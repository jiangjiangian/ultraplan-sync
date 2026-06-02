#!/usr/bin/env python3
"""build_graph.py — 《尋傘記》repo 的知識圖譜萃取器。

把整個 repo（`git ls-files` 取得的每一個版控檔案，一個都不漏）轉成一張
可互動的知識圖譜：節點 = 檔案 / OO 概念 / 設計模式 / 領域分層，邊 =
`#include` 相依、類別繼承、模式落點、領域依賴方向。純靜態分析，只用
標準函式庫，無需 LLM 或外部套件（與 `tools/docs_graph.py` 同精神）。

產物（全部落在 `graph/` 底下）：
    graph/data/graph.json      Cytoscape elements（nodes + edges）+ meta
    graph/data/graph-data.js   同一份資料，包成 window.GRAPH_DATA（支援 file://）
    graph/data/files.json      扁平的「每檔一列」索引（給工具 / agent 直接讀）
    graph/wiki/files-index.md   人類可讀的全檔索引（每個檔案都有，含深連結）

用法：
    python3 graph/build_graph.py            # 重建上述資料
    python3 graph/build_graph.py --check    # 只驗證「節點數 == 檔案數」，不寫檔
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path
from urllib.parse import quote


def enc(node_id: str) -> str:
    """把 node id 編成可安全放進 markdown 連結 / URL hash 的字串（空白、括號…）。"""
    return quote(node_id, safe="/:._-")

# --------------------------------------------------------------------------
# 0. 常數 / 路徑
# --------------------------------------------------------------------------
ROOT = Path(
    subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
)
GRAPH = ROOT / "graph"
DATA = GRAPH / "data"
WIKI = GRAPH / "wiki"
REPO = "jiangjiangian/ultraplan-sync"
BRANCH = "main"
BLOB = f"https://github.com/{REPO}/blob/{BRANCH}/"

# include 解析的根（對應 CMakeLists 的 target_include_directories）
INCLUDE_ROOTS = ["include", "include/game"]

# 領域分層的依賴方向（紅線：engine 不反向相依 game/ui）
DOMAIN_DEPS = [
    ("app", "game"), ("app", "ui"), ("app", "engine"),
    ("game", "engine"), ("ui", "engine"), ("ui", "game"),
]
DOMAIN_LABEL = {
    "app": "app · 組裝根", "engine": "engine · 引擎層",
    "game": "game · 遊戲邏輯（Model）", "ui": "ui · 視圖層（View）",
    "tests": "tests · 測試", "docs": "docs · 設計文件",
    "tools": "tools · 離線管線", "resources": "resources · 執行期資產",
    "root": "root · 專案根",
}

# --------------------------------------------------------------------------
# 1. 檔案清單 + 分類
# --------------------------------------------------------------------------
def git_files() -> list[str]:
    """以 NUL 分隔取得每一個版控檔案（避免全形/空白路徑被引號包起來）。"""
    out = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT,
        capture_output=True, text=True, check=True,
    ).stdout
    return [p for p in out.split("\0") if p]


CODE_EXT = {".cpp", ".h", ".hpp", ".cc", ".cxx"}
ASSET_EXT = {".png", ".jpg", ".jpeg", ".ttf", ".otf", ".wav", ".ogg", ".gif"}


def classify(path: str) -> dict:
    """把一個檔案路徑分類成 (kind, ntype, domain, bucket)。

    kind  : file（一律 file；概念/模式/領域節點另外加）
    ntype : header|source|test|doc|content|asset|tool|build|config|other
    domain: app|engine|game|ui|tests|docs|tools|resources|root
    bucket: 次層資料夾（例如 entities / render / UML / content），無則 ""
    """
    p = path
    parts = p.split("/")
    ext = ("." + p.rsplit(".", 1)[1].lower()) if "." in parts[-1] else ""

    domain, bucket, ntype = "root", "", "other"

    if parts[0] in ("include", "src"):
        if len(parts) > 2:
            domain = parts[1]                 # include/<domain>/...
            if len(parts) >= 4:
                bucket = parts[2]             # include/<domain>/<bucket>/File.h
        else:
            domain = "root"                   # include/README.md、src/README.md — 樹層級說明
        ntype = "header" if parts[0] == "include" else "source"
        if ext == ".md":
            ntype = "doc"
    elif parts[0] == "tests":
        domain = "tests"
        bucket = parts[1] if len(parts) > 2 else ""
        ntype = "test" if ext in CODE_EXT else ("doc" if ext == ".md" else "other")
    elif parts[0] == "docs":
        domain = "docs"
        if len(parts) >= 3:
            bucket = parts[1]            # UML / content
        ntype = "content" if "/content/" in "/" + p else "doc"
    elif parts[0] == "tools":
        domain = "tools"
        ntype = "tool" if ext == ".py" else ("doc" if ext == ".md" else "other")
    elif parts[0] == "resources":
        domain = "resources"
        bucket = parts[1] if len(parts) > 2 else ""
        ntype = "asset" if ext in ASSET_EXT else "other"
    else:
        domain = "root"
        if parts[-1] in ("CMakeLists.txt", "Doxyfile"):
            ntype = "build"
        elif parts[-1].startswith(".") or parts[-1] in (".gitignore",):
            ntype = "config"
        elif ext == ".md":
            ntype = "doc"
        elif ext in ASSET_EXT:
            ntype = "asset"

    return {"ntype": ntype, "domain": domain, "bucket": bucket, "ext": ext}


# --------------------------------------------------------------------------
# 2. 程式碼解析：#include 與 類別/繼承
# --------------------------------------------------------------------------
INC_RX = re.compile(r'#\s*include\s+"([^"]+)"')
SYS_INC_RX = re.compile(r"#\s*include\s+<([^>]+)>")
# class/struct + 可選 final + 可選 base-spec，必須以 { 收尾（排除前向宣告 `class X;`）
CLASS_RX = re.compile(
    r"(?<!enum )\b(class|struct)\s+([A-Za-z_]\w*)\b(?:\s+final\b)?\s*"
    r"(?::\s*([^{};]+?))?\s*\{",
    re.MULTILINE,
)
BASE_TOKEN_RX = re.compile(r"[A-Za-z_]\w*")


def base_names(spec: str) -> list[str]:
    """從 `: public A, private ns::B<T>` 取出 base 類別的素名（A, B）。"""
    names = []
    for chunk in spec.split(","):
        c = re.sub(r"<[^>]*>", "", chunk)          # 去模板引數
        c = c.replace("virtual", " ")
        for kw in ("public", "protected", "private"):
            c = re.sub(rf"\b{kw}\b", " ", c)
        toks = BASE_TOKEN_RX.findall(c.split("::")[-1])  # 去命名空間，取最後一段
        if toks:
            names.append(toks[-1])
    return names


def parse_code(text: str) -> dict:
    """解析一個 .h/.cpp，回傳 includes / 系統 include / 類別與其 base。"""
    includes = sorted(set(INC_RX.findall(text)))
    sys_inc = sorted(set(SYS_INC_RX.findall(text)))
    classes = []
    for _kw, name, spec in CLASS_RX.findall(text):
        classes.append({"name": name, "bases": base_names(spec) if spec else []})
    loc = text.count("\n") + 1
    return {"includes": includes, "sys": sys_inc, "classes": classes, "loc": loc}


def resolve_include(inc: str, src_path: str, fileset: set[str]) -> str | None:
    """把 `#include "X"` 的 X 解析成 repo 內實際檔案路徑（命中才回傳）。"""
    cands = [f"{root}/{inc}" for root in INCLUDE_ROOTS]
    cands.append(str(Path(src_path).parent / inc).replace("\\", "/"))
    for c in cands:
        c = re.sub(r"/\./", "/", c)
        if c in fileset:
            return c
    return None


# --------------------------------------------------------------------------
# 3. 策展的 OO 概念地圖（以「類別名」連結到檔案，對路徑差異穩健）
#    來源：docs/UML/{1..8}.md — 與設計文件一致。
# --------------------------------------------------------------------------
CONCEPTS = [
    # --- GoF 設計模式 ---
    dict(id="pat-factory", label="Factory Method", kind="pattern",
         summary="GameObjectFactory::Create(ObjectType) 由列舉動態產生 12 種具體 GameObject。",
         classes=["GameObjectFactory"]),
    dict(id="pat-template", label="Template Method", kind="pattern",
         summary="TransparentUmbrella::BeClaimed 與 ConsumableItem::Consume 為純虛擬骨架，子類別填行為。",
         classes=["TransparentUmbrella", "ConsumableItem"]),
    dict(id="pat-observer", label="Observer", kind="pattern",
         summary="EventBus Subscribe/Publish 解耦事件；Subscription 為 RAII 退訂 token。",
         classes=["EventBus", "Subscription"]),
    dict(id="pat-state", label="State", kind="pattern",
         summary="SemesterStateMachine 持有 IChapterState；五個章節狀態切換，四結局以哨兵記錄。",
         classes=["SemesterStateMachine", "IChapterState"]),
    dict(id="pat-strategy", label="Strategy / Pipeline", kind="pattern",
         summary="ISystem::Run 的五個 stage（Survival/Movement/Collision/Spawn/Sweep）由 Controller 依序執行。",
         classes=["ISystem"]),
    dict(id="pat-singleton", label="Singleton", kind="pattern",
         summary="EventBus::Instance() 全域事件匯流排（shared_mutex 僅護 handler list）。",
         classes=["EventBus"]),
    dict(id="pat-command", label="Command / Table（資料化）", kind="pattern",
         summary="QuestHookTable 把 ~14 個內嵌互動鉤子改為一張有序、自我把關的資料表（OCP）。",
         classes=["QuestHookTable", "QuestHook"]),
    # --- 進階 OO 技法 ---
    dict(id="oo-crtp", label="CRTP static mixin", kind="principle",
         summary="WithRoles<Derived,Base> 以 std::derived_from + if constexpr 在編譯期實作 As*() 能力查詢，取代 dynamic_cast。",
         classes=["WithRoles"]),
    dict(id="oo-isp-roles", label="角色介面（ISP）", kind="principle",
         summary="Roles.h 把胖介面拆成 IUpdatable / IDrawable / IInteractable / IMortal，葉類別只實作扮演的角色。",
         classes=["IUpdatable", "IDrawable", "IInteractable", "IMortal"]),
    dict(id="oo-raii", label="RAII / 記憶體安全", kind="principle",
         summary="物件以 unique_ptr 持有；Texture/Font/Subscription 皆 RAII；移除採 isActive 旗標 + 幀末 Sweep（mark-then-sweep）。",
         classes=["Texture", "Subscription"]),
    # --- 架構 ---
    dict(id="arch-mvc", label="MVC 核心", kind="architecture",
         summary="World＝純資料 Model、View＝只讀模型繪圖、GameController＝收輸入＋跑模擬＋接事件。",
         classes=["World", "View", "GameController"]),
    dict(id="arch-isystem", label="ISystem 模擬管線", kind="architecture",
         summary="每幀邏輯拆成可插拔的 ISystem stage，透過 SimContext 串接；god-method 解體。",
         classes=["ISystem", "SimContext"]),
    dict(id="arch-dip-renderer", label="DIP：IRenderer", kind="architecture",
         summary="所有 raylib Draw* 關在 IRenderer 後；Model 端只認 IRenderer&，永不 include raylib（架構紅線）。",
         classes=["IRenderer", "RaylibRenderer"]),
    dict(id="arch-harness", label="決定性 autoplay（Harness）", kind="architecture",
         summary="InputSource 抽象（LiveInput/ScriptInput）＋ Time 固定 1/60 步 ⇒ 同腳本 byte-identical state.jsonl；正常遊玩旁路。",
         classes=["InputSource", "ScriptInput", "Harness", "Time"]),
]

# 每個概念的 wiki 內文（是什麼／怎麼運作）、相關概念、來源文件。
# 與 docs/UML/{1..8}.md 一致；落點檔案表由 realizes 邊自動產生（不手寫路徑）。
CONCEPT_PROSE = {
    "pat-factory": dict(related=["pat-template", "oo-isp-roles", "arch-isystem"],
        sources=["docs/UML/7-gof.md", "docs/UML/1-entities.md"],
        body="`GameObjectFactory::Create(ObjectType, Vec2)` 是單一進入點，依 `ObjectType` 列舉"
             "把『要生哪種物件』與『誰來 new』解耦。呼叫端（spawn 設定、`SpawnSystem`、章節 roster）"
             "只給一個列舉值與座標，工廠回傳 `unique_ptr<GameObject>`；12 種具體型別（4 把傘、3 種"
             "消耗品、Vendor、3 種金幣、Player…）對呼叫端不可見。新增一種物件＝工廠加一個 case + "
             "列舉加一項，呼叫端不動（OCP）。"),
    "pat-template": dict(related=["pat-factory", "pat-observer"],
        sources=["docs/UML/7-gof.md", "docs/UML/1-entities.md", "docs/UML/6-sequence.md"],
        body="`TransparentUmbrella::BeClaimed(Player*)` 與 `ConsumableItem::Consume(Player*)` 是純"
             "虛擬的『骨架步驟』：基底定義互動的固定流程（偵測 → 入袋/撐傘 → 廣播事件），把『被拾取"
             "後究竟發生什麼』留給子類別覆寫。4 把傘給 4 種後果（真傘完成章節、詛咒傘扣業力…），"
             "3 種消耗品給 3 種使用效果。基底不認得任何具體子類別。"),
    "pat-observer": dict(related=["pat-singleton", "oo-raii", "arch-mvc"],
        sources=["docs/UML/7-gof.md", "docs/UML/3-mvc-isystem.md", "docs/UML/6-sequence.md"],
        body="`EventBus`（單例）讓事件的『發出方』與『反應方』完全解耦。Model 端（傘、Vendor、"
             "Player）只 `Publish(Event)`，不知道誰在聽；UI/HUD/Toast/harness 用 `Subscribe` / "
             "`ScopedSubscribe` 註冊 handler。事件有 `UmbrellaClaimed`、`KarmaChanged`、`ShowMessage`、"
             "`EnteredBuilding`、`PickupAcquired`。`Publish` 對 handler 的 snapshot 廣播（避免廣播中"
             "改訂閱造成迭代失效）。`ScopedSubscribe` 回傳 `Subscription`（RAII token），出作用域自動退訂。"),
    "pat-state": dict(related=["pat-command", "arch-isystem"],
        sources=["docs/UML/2-state-machine.md", "docs/UML/7-gof.md"],
        body="學期進程是一台 `SemesterStateMachine`，目前章節是一個 `IChapterState`（`Enter/Exit/"
             "Update`）。`Transition(next)` 析構舊狀態、建構新的具體章節狀態（Chapter1AddDrop / "
             "InterludeMarket / Chapter2Midterms / Chapter3SportsDay / Chapter4Finals）。`Interlude_"
             "Market` 是共用轉運站，被重複進出三次，由 `InterludeReturnTo` 決定下一站。四個結局"
             "（A→B→D→C）**不是** `IChapterState` 子類別，而以 `ending_`/`inEnding_` 哨兵記錄，判定"
             "集中在自由函式 `CheckEndingGates()`，每個非對話幀輪詢一次。"),
    "pat-strategy": dict(related=["arch-isystem", "pat-state", "oo-isp-roles"],
        sources=["docs/UML/3-mvc-isystem.md", "docs/UML/6-sequence.md", "docs/UML/7-gof.md"],
        body="原本約 793 行的 `GameController::Update()` god-method 被拆成一條 `ISystem` 管線："
             "每個 stage（`SurvivalSystem`、`MovementSystem`、`CollisionSystem`、`SpawnSystem`、"
             "`SweepSystem`）只做一件事、實作同一個 `Run(SimContext&, dt)`，由 Controller 以固定順序"
             "執行、透過 `SimContext` 串接。stage 可組合、可重排、可單獨測試，也是 Assignment #6 生存"
             "玩法（敵人 spawner、碰撞傷害）的直接擴充點。"),
    "pat-singleton": dict(related=["pat-observer"],
        sources=["docs/UML/7-gof.md", "docs/UML/3-mvc-isystem.md"],
        body="`EventBus::Instance()` 提供全域唯一的事件匯流排，讓散落各處的 publisher / subscriber "
             "共用同一條通道而不必彼此持有參照。內部以 `shared_mutex` 保護 handler list（注意：只護 "
             "list，handler 本體仍須單執行緒呼叫，因 GL 單執行緒——見技術債 H1）。"),
    "pat-command": dict(related=["pat-state", "arch-mvc"],
        sources=["docs/UML/3-mvc-isystem.md", "docs/UML/7-gof.md"],
        body="E 互動的任務副作用原本是約 14 個內嵌 `TryXxx()` 呼叫；改成一張資料化的 `QuestHookTable`："
             "每個鉤子是一筆 `QuestHook`（條件 + 動作），由 `RunInteractHooks(player, npcId, state, "
             "returnTo)` 依序、自我把關地執行。新增章節劇情＝`RegisterHook` 加一行，不動既有控制流（OCP）。"),
    "oo-crtp": dict(related=["oo-isp-roles", "arch-mvc"],
        sources=["docs/UML/1-entities.md", "docs/UML/8-solid.md"],
        body="`WithRoles<Derived, Base>` 是插在 `Character`/`Item` 與葉類別之間的 CRTP mixin。它在"
             "**編譯期**用 `std::derived_from` + `if constexpr` 判斷 `Derived` 扮演哪些角色介面，產生 "
             "`AsUpdatable()`/`AsDrawable()`/`AsInteractable()`/`AsMortal()`：命中回傳 typed pointer、"
             "未扮演回傳 `nullptr`——全程**無 `dynamic_cast`**、無 RTTI 成本。場景容器只持 "
             "`GameObject*`，靠這些 `As*()` 做多型分派（LSP）。"),
    "oo-isp-roles": dict(related=["oo-crtp", "pat-strategy"],
        sources=["docs/UML/1-entities.md", "docs/UML/8-solid.md"],
        body="`Roles.h` 把『一個 GameObject 該會什麼』從一個胖介面拆成四個小角色介面：`IUpdatable`"
             "（會動）、`IDrawable`（會畫）、`IInteractable`（可互動）、`IMortal`（有血量/會死）。"
             "道具不必實作 `Update`、看板不必 `IMortal`——只實作自己扮演的角色（ISP）。`IMortal` 是 "
             "Assignment #6 戰鬥的鋪路，目前只有 `Player` 扮演。"),
    "oo-raii": dict(related=["pat-observer", "arch-isystem"],
        sources=["docs/UML/4-gfx.md", "docs/UML/8-solid.md", "docs/UML/6-sequence.md"],
        body="資源生命週期一律綁在物件上：GameObject 以 `unique_ptr` 持有；`Texture`/`Font` 是 "
             "move-only RAII 包裝、GL 資源在 `~Window`/`CloseWindow` 前顯式釋放；"
             "`EventBus::Subscription` 是 RAII 退訂 token。物件移除**不在迭代中 `delete`**——改標記 "
             "`isActive_=false`，由 `SweepSystem`→`World::Sweep()` 於幀末 mark-then-sweep 統一 "
             "erase-remove（`objects_.front()` 恆為 Player）。"),
    "arch-mvc": dict(related=["arch-isystem", "arch-dip-renderer", "pat-observer"],
        sources=["docs/UML/0-layer-map.md", "docs/UML/3-mvc-isystem.md", "docs/UML/8-solid.md"],
        body="三權分立：`World` 是**純資料 Model**（擁有所有 GameObject、學期 FSM、碰撞遮罩、對話/"
             "HUD/背包狀態），不認得 raylib、不讀輸入；`View` **只讀** `const World&` 把畫面畫出來；"
             "`GameController` 收輸入、跑 `ISystem` 模擬、接 `EventBus` 事件、改 World。`main.cpp` 是"
             "薄薄的組裝根。"),
    "arch-isystem": dict(related=["pat-strategy", "arch-mvc", "oo-raii"],
        sources=["docs/UML/3-mvc-isystem.md", "docs/UML/6-sequence.md"],
        body="每幀的 model 推進是一條有序的 `ISystem` 管線，透過 `SimContext`（world、世界尺寸、本幀 "
             "collider、上一幀玩家座標）串接。鐵律：`ISystem` 只動 model（`World&`/`Player&`），不讀"
             "輸入、不呼叫 raylib、不繪圖。順序：Survival → Movement → Collision → Spawn →（互動/結局"
             "判定）→ Sweep。"),
    "arch-dip-renderer": dict(related=["arch-mvc", "oo-raii"],
        sources=["docs/UML/4-gfx.md", "docs/UML/8-solid.md"],
        body="所有 raylib `::Draw*` 都關在 `IRenderer` 介面後，`RaylibRenderer` 是唯一具體實作。Model "
             "端寫 `Render(IRenderer&)`，永遠不 `#include` raylib（架構紅線＃2）——這就是依賴反轉："
             "高層 Model 不依賴低層繪圖細節，兩者都依賴抽象。材質經 process-lifetime 的 `TextureCache` "
             "只上傳一次。"),
    "arch-harness": dict(related=["arch-mvc", "oo-raii"],
        sources=["docs/UML/5-harness.md", "docs/UML/8-solid.md"],
        body="感知＋致動的縫合層，預設關閉（無 `UMBRELLA_SCRIPT` 環境變數時旁路，正常遊玩 bit-for-bit "
             "不變）。啟用時把輸入換成 deterministic 的 `ScriptInput`（經 `InputSource` 抽象，對應 "
             "`LiveInput`）、時間換成 `Time` 固定 1/60 步，每幀輸出一行 JSON 狀態 ⇒ 同腳本產生 "
             "byte-identical `state.jsonl`，讓自動遊玩可重播、可回歸測試。"),
}

# 四個程式碼領域的導言（檔案表由 build 自動帶出，故只寫『為什麼』）。
DOMAIN_INTRO = {
    "app": "組裝根與場景框架。`main.cpp`（composition root）把 Window / Font / Loading / 主迴圈"
           "組起來；`IScene` / `SceneManager` / 各 scene 提供場景切換骨架。相依方向最外層："
           "**app → game / ui / engine**。",
    "engine": "與遊戲內容無關、可重用的引擎層：`core`（GameObject / Roles 基礎型別）、`events`"
              "（EventBus）、`render`（IRenderer / Raylib 包裝）、`input`、`math`、`audio`、"
              "`platform`（harness / Time）。**鐵律：engine 不反向相依 game / ui。**",
    "game": "遊戲邏輯（MVC 的 Model 端）：`entities`（GameObject 家族）、`world`（World / 碰撞）、"
            "`state`（學期狀態機 / 結局）、`quest`（鉤子表 / ripple / spawn）、`controller`"
            "（GameController / Factory / ISystem 管線）、`dialog`、`vendor`、`gfx`。",
    "ui": "視圖層（MVC 的 View 端）：`View` 主繪圖器，加上 `hud` / `overlay` / `world` 子視圖與扁平"
          "視圖（標題、角色選擇、結局卡、說明頁、背包、訊息）。只讀 `const World&`，經 `IRenderer` 輸出。",
}


# --------------------------------------------------------------------------
# 4. 主流程：建圖
# --------------------------------------------------------------------------
def build() -> dict:
    files = git_files()
    fileset = set(files)

    nodes: list[dict] = []
    edges: list[dict] = []
    file_meta: dict[str, dict] = {}
    class_to_file: dict[str, str] = {}
    seen_edge: set[tuple] = set()

    def add_edge(src: str, tgt: str, etype: str):
        key = (src, tgt, etype)
        if src == tgt or key in seen_edge:
            return
        seen_edge.add(key)
        edges.append({"data": {
            "id": f"e{len(edges)}", "source": src, "target": tgt, "etype": etype}})

    # --- 4.1 領域 + bucket 容器節點 ---
    domains_seen: set[str] = set()
    buckets_seen: set[str] = set()

    # --- 4.2 逐檔：建檔案節點 + 解析程式碼 ---
    for path in files:
        cls = classify(path)
        nid = f"file:{path}"
        meta = {
            "id": nid, "label": path.rsplit("/", 1)[-1], "kind": "file",
            "path": path, **cls, "loc": 0, "classes": [],
            "github": BLOB + quote(path, safe="/"),
        }
        if cls["ext"] in CODE_EXT:
            try:
                text = (ROOT / path).read_text(encoding="utf-8", errors="replace")
                parsed = parse_code(text)
                meta["loc"] = parsed["loc"]
                meta["classes"] = [c["name"] for c in parsed["classes"]]
                meta["_includes"] = parsed["includes"]
                meta["_class_objs"] = parsed["classes"]
                # 只在 header 註冊 class→file（宣告處），避免 .cpp 重複定義搶走
                if cls["ntype"] in ("header",):
                    for c in parsed["classes"]:
                        class_to_file.setdefault(c["name"], path)
            except Exception as exc:  # noqa: BLE001 — 萃取器要韌性，壞檔不致命
                meta["_error"] = str(exc)
        file_meta[path] = meta
        domains_seen.add(cls["domain"])
        if cls["bucket"]:
            buckets_seen.add(f'{cls["domain"]}/{cls["bucket"]}')

    # tests 的 class→file 不該蓋掉正式宣告：第二輪補上 src/ 內的 class（若 header 沒登記）
    for path, meta in file_meta.items():
        if meta.get("ntype") == "source":
            for c in meta.get("_class_objs", []):
                class_to_file.setdefault(c["name"], path)

    # --- 4.3 容器節點（domain / bucket）---
    for d in sorted(domains_seen):
        nodes.append({"data": {
            "id": f"domain:{d}", "label": DOMAIN_LABEL.get(d, d),
            "kind": "domain", "domain": d}})
    for b in sorted(buckets_seen):
        d, name = b.split("/", 1)
        nodes.append({"data": {
            "id": f"bucket:{b}", "label": f"{d}/{name}", "kind": "bucket",
            "domain": d, "bucket": name}})

    # --- 4.4 概念 / 模式節點 ---
    for c in CONCEPTS:
        nodes.append({"data": {
            "id": c["id"], "label": c["label"], "kind": c["kind"],
            "summary": c["summary"],
            "wiki": f"wiki/concepts/{c['id']}.md"}})

    # --- 4.5 檔案節點落地 + 容器歸屬邊 ---
    for path, meta in file_meta.items():
        clean = {k: v for k, v in meta.items() if not k.startswith("_")}
        if "wiki" not in clean and meta["ntype"] in ("header", "source"):
            clean["wiki"] = None
        nodes.append({"data": clean})
        d, b = meta["domain"], meta["bucket"]
        if b:
            add_edge(f"file:{path}", f"bucket:{d}/{b}", "in-bucket")
            add_edge(f"bucket:{d}/{b}", f"domain:{d}", "in-domain")
        else:
            add_edge(f"file:{path}", f"domain:{d}", "in-domain")

    # --- 4.6 include 相依邊 ---
    for path, meta in file_meta.items():
        for inc in meta.get("_includes", []):
            tgt = resolve_include(inc, path, fileset)
            if tgt:
                add_edge(f"file:{path}", f"file:{tgt}", "includes")

    # --- 4.7 繼承邊 ---
    for path, meta in file_meta.items():
        for c in meta.get("_class_objs", []):
            for base in c["bases"]:
                bfile = class_to_file.get(base)
                if bfile and bfile != path:
                    add_edge(f"file:{path}", f"file:{bfile}", "inherits")

    # --- 4.8 概念 → 落點檔案 邊（以類別名連結）---
    for c in CONCEPTS:
        for cname in c.get("classes", []):
            f = class_to_file.get(cname)
            if f:
                add_edge(c["id"], f"file:{f}", "realizes")

    # --- 4.9 領域依賴方向 邊 ---
    for a, b in DOMAIN_DEPS:
        if a in domains_seen and b in domains_seen:
            add_edge(f"domain:{a}", f"domain:{b}", "depends")

    # --- 4.10 測試 → 受測 bucket 邊（同名 bucket）---
    for path, meta in file_meta.items():
        if meta["domain"] == "tests" and meta["bucket"]:
            for d in ("game", "engine", "ui", "app"):
                tb = f"bucket:{d}/{meta['bucket']}"
                if any(n["data"]["id"] == tb for n in nodes):
                    add_edge(f"file:{path}", tb, "tests")
                    break

    meta_block = {
        "repo": REPO, "branch": BRANCH, "generated_by": "graph/build_graph.py",
        "counts": {
            "files": len(files),
            "file_nodes": sum(1 for n in nodes if n["data"]["kind"] == "file"),
            "concept_nodes": len(CONCEPTS),
            "domain_nodes": len(domains_seen),
            "bucket_nodes": len(buckets_seen),
            "nodes_total": len(nodes), "edges_total": len(edges),
        },
        "edge_types": sorted({e["data"]["etype"] for e in edges}),
        "domains": sorted(domains_seen),
    }
    return {"elements": {"nodes": nodes, "edges": edges}, "meta": meta_block,
            "_file_meta": file_meta}


# --------------------------------------------------------------------------
# 5. 輸出
# --------------------------------------------------------------------------
def _src_links(sources: list[str], up: str = "../../../") -> str:
    return " · ".join(f"[`{s}`]({up}{s})" for s in sources)


def write_wiki(graph: dict) -> None:
    """產生 llm_wiki 風格的概念頁與領域頁（含 frontmatter、wikilink、落點表、來源）。"""
    (WIKI / "concepts").mkdir(parents=True, exist_ok=True)
    (WIKI / "domains").mkdir(parents=True, exist_ok=True)
    nodes = {n["data"]["id"]: n["data"] for n in graph["elements"]["nodes"]}
    realizes: dict[str, list[str]] = defaultdict(list)
    for e in graph["elements"]["edges"]:
        d = e["data"]
        if d["etype"] == "realizes":
            realizes[d["source"]].append(d["target"])
    label_of = {c["id"]: c["label"] for c in CONCEPTS}
    kind_zh = {"pattern": "設計模式 (GoF)", "principle": "OO 原則 / 技法",
               "architecture": "架構元件"}

    # ---- 概念頁 ----
    for c in CONCEPTS:
        pr = CONCEPT_PROSE.get(c["id"], {})
        L = ["---", f"id: {c['id']}", f"type: {c['kind']}", f"title: {c['label']}",
             f"sources: [{', '.join(pr.get('sources', []))}]", "---", "",
             f"# {c['label']} · {kind_zh.get(c['kind'], c['kind'])}", "",
             f"> {c['summary']}", "", "## 是什麼 / 怎麼運作", "",
             pr.get("body", c["summary"]), ""]
        targets = sorted(realizes.get(c["id"], []))
        if targets:
            L += ["## 落點（程式碼）", "", "| 檔案 | 類別 | 連結 |", "|---|---|---|"]
            for t in targets:
                nd = nodes.get(t, {})
                classes = ", ".join(f"`{x}`" for x in nd.get("classes", [])) or "—"
                L.append(f"| `{nd.get('path', '?')}` | {classes} | "
                         f"[node](../../index.html#node={enc(t)}) · [src]({nd.get('github', '#')}) |")
            L.append("")
        rel = pr.get("related", [])
        if rel:
            L += ["## 相關概念", "",
                  " · ".join(f"[{label_of.get(r, r)}]({r}.md)" for r in rel), ""]
        if pr.get("sources"):
            L += ["## 來源（設計文件）", "", _src_links(pr["sources"]), ""]
        L += ["---",
              f"[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node={c['id']})"]
        (WIKI / "concepts" / f"{c['id']}.md").write_text("\n".join(L), encoding="utf-8")

    # ---- 領域頁（檔案表自動帶出，保證與真實檔案同步）----
    files_by_dom: dict[str, dict[str, list]] = defaultdict(lambda: defaultdict(list))
    for nd in nodes.values():
        if nd["kind"] == "file":
            files_by_dom[nd["domain"]][nd.get("bucket") or "(根)"].append(nd)
    for dom, intro in DOMAIN_INTRO.items():
        buckets = files_by_dom.get(dom, {})
        total = sum(len(v) for v in buckets.values())
        L = ["---", f"id: domain-{dom}", "type: domain", f"title: {dom}", "---", "",
             f"# 領域：{DOMAIN_LABEL.get(dom, dom)}", "", f"> {intro}", "",
             f"共 **{total}** 個檔案，分 {len(buckets)} 個 bucket。"
             f"[在互動圖譜中檢視 →](../../index.html#node=domain:{dom})", ""]
        for bucket in sorted(buckets):
            fs = sorted(buckets[bucket], key=lambda x: x["path"])
            L += [f"## {dom}/{bucket}  ({len(fs)})", "",
                  "| 檔案 | 類別 | 連結 |", "|---|---|---|"]
            for nd in fs:
                classes = ", ".join(f"`{x}`" for x in nd.get("classes", [])) or "—"
                L.append(f"| `{nd['path']}` | {classes} | "
                         f"[node](../../index.html#node={enc(nd['id'])}) · [src]({nd['github']}) |")
            L.append("")
        L += ["---", "[← wiki 索引](../index.md)"]
        (WIKI / "domains" / f"{dom}.md").write_text("\n".join(L), encoding="utf-8")


def write_outputs(graph: dict) -> None:
    DATA.mkdir(parents=True, exist_ok=True)
    WIKI.mkdir(parents=True, exist_ok=True)

    file_meta = graph.pop("_file_meta")
    payload = json.dumps(graph, ensure_ascii=False, indent=1)
    (DATA / "graph.json").write_text(payload + "\n", encoding="utf-8")
    (DATA / "graph-data.js").write_text(
        "// 自動產生 by graph/build_graph.py — 請勿手改。\n"
        "window.GRAPH_DATA = " + payload + ";\n", encoding="utf-8")

    # files.json — 扁平每檔索引
    flat = []
    for path, m in sorted(file_meta.items()):
        flat.append({k: v for k, v in m.items() if not k.startswith("_")}
                    | {"includes": m.get("_includes", []),
                       "include_count": len(m.get("_includes", []))})
    (DATA / "files.json").write_text(
        json.dumps(flat, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")

    # files-index.md — 每個檔案都有一列（完整、含深連結）
    by_dom: dict[str, list[dict]] = defaultdict(list)
    for m in flat:
        by_dom[m["domain"]].append(m)
    lines = ["# 全檔索引 — 《尋傘記》知識圖譜",
             "",
             f"> 自動產生 by `graph/build_graph.py`。共 **{len(flat)}** 個版控檔案，"
             "一個都不漏。點 `node` 連結會在互動圖譜中聚焦該檔。",
             ""]
    for dom in sorted(by_dom):
        lines += [f"## {DOMAIN_LABEL.get(dom, dom)}  ({len(by_dom[dom])})", "",
                  "| 檔案 | 類型 | bucket | LOC | 類別 | 連結 |",
                  "|---|---|---|---|---|---|"]
        for m in sorted(by_dom[dom], key=lambda x: x["path"]):
            classes = ", ".join(f"`{c}`" for c in m.get("classes", [])) or "—"
            link = (f"[node](../index.html#node={enc(m['id'])}) · "
                    f"[src]({m['github']})")
            lines.append(
                f"| `{m['path']}` | {m['ntype']} | {m['bucket'] or '—'} | "
                f"{m['loc'] or '—'} | {classes} | {link} |")
        lines.append("")
    (WIKI / "files-index.md").write_text("\n".join(lines), encoding="utf-8")

    write_wiki(graph)


def main(argv: list[str]) -> int:
    graph = build()
    c = graph["meta"]["counts"]
    # 完整性硬性檢查：每個版控檔案都必須是一個 file 節點
    assert c["file_nodes"] == c["files"], (
        f"完整性檢查失敗：file_nodes={c['file_nodes']} != files={c['files']}")
    if "--check" in argv:
        print(f"OK — {c['files']} 檔案全數成為節點；"
              f"nodes={c['nodes_total']} edges={c['edges_total']}")
        return 0
    write_outputs(graph)
    print(f"wrote graph/data/*.json + graph/wiki/files-index.md")
    print(f"  files={c['files']}  nodes={c['nodes_total']}  edges={c['edges_total']}  "
          f"concepts={c['concept_nodes']}  buckets={c['bucket_nodes']}")
    print(f"  edge types: {', '.join(graph['meta']['edge_types'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
