# 《尋傘記：政大山下篇》— 期末專案口頭報告

> C++20 / Raylib 5.5 俯視角敘事 RPG ｜ NCCU OOP Assignment #5→#6
> 作者：iansu（蘇畇亦）

## 目錄

- [一、專案整體理解](#一專案整體理解)
- [二、OOP 架構與責任分離](#二oop-架構與責任分離)
- [三、遊戲核心機制](#三遊戲核心機制)
- [四、程式碼細節與除錯](#四程式碼細節與除錯)
- [五、表達與誠實程度／心路歷程](#五表達與誠實程度心路歷程)
- [附：建置與執行](#附建置與執行)

---

## 一、專案整體理解

> ***前言***  
> 一開始想做這個遊戲，有三個原因。第一，政大一直下雨，身邊一定要帶把傘，沒有傘很難在這裡生存；第二，我常在校版上看到同學 Po 文說「傘又不見了」、「我相信你只是拿錯傘」之類的（所以我都把傘帶在身邊、盡量不離手，目前還沒掉過，非常慶幸～）；第三，這學期的市集實在太多了（真的太多了），當下我心裡冒出的念頭是：「這蠻像薩爾達的驛站、或是各種地圖上的市集，感覺可以拿來當素材～」。總之，一陣腦力激盪之後，點子像雨後春筍般冒出來，我趕緊去問 Gemini 這究竟該怎麼具體實作、有沒有什麼參考資料和論文。最早讀到的一篇是 [WHAT-IF: Exploring Branching Narratives by Meta-Prompting Large Language Models](https://arxiv.org/abs/2412.10582)：作者提出一個叫 WHAT-IF 的系統，用零樣本元提示（zero-shot meta-prompting）把既有的線性故事自動轉成具備多重選擇的分支劇情（branching narratives），再用圖形結構（graph）確保故事的連貫與邏輯。聽起來好像不難（？）。後來 Gemini 幫我寫了第一份文件 [遊戲企劃與敘事架構](../遊戲企劃與敘事架構.md)，我看完覺得很不錯，裡面很多都正是我想要的；再加上學過的 C++ 語法（類別 → 動態多型、模板 → 靜態多型）與 modern C++（`std::move()`、`std::unique_ptr<T>` 等智慧指標），這些都很值得真正用進遊戲實作裡。
>
> 不過，一開始把需求講給 AI 聽時，它其實都能做到；可是當 codebase 越長越大、檔案一多，context window 裝不下那麼多輸入，它就開始「失憶」。中間一度有點失控（到處亂改、生出一堆不必要的東西），我很清楚這是我自己「駕馭」（harness）沒做好。於是我上網查了不少和 AI 協作的正確方法，大致領悟到三件事：  
> 1. 對於一個專案（尤其大型專案），一開始一定要有非常清楚的「計畫」，以及建立一個 [graph](https://jiangjiangian.github.io/ultraplan-sync/) 做檔案索引
> 2. 真正開始前一定要建立一個「團隊」，打造一隻專屬於這個專案的各種「專家」，各自負責什麼
> 3. 出 bug 時一定要人類具體指出來，並且用條列式的「詳細」文字敘述，請他們找出哪一份檔案與相依的檔案受到影響  

> 後來照著這些原則做，一切就拉回了正軌。這也給了我一個很重要的啟發：要和 AI 協作，不只需要足夠的 domain knowledge，更需要一種「帶團隊」的心態——得具體而密集地討論分工細節，才能持續把專案往前推；否則就只是放任它們像無頭蒼蠅般打轉。我想，未來和 AI 一起工作，會越來越像是和一位同等地位的「夥伴」共事（最終拍板的當然還是人類），由人與 AI 一起組成一支團隊。
 
在終年下著冬雨的政大山下校區，一把透明傘的失蹤，引發一整學期的蝴蝶效應；玩家要在加退選、期中考、運動會、期末考四個節點間穿梭，靠「業力」與「選擇」尋回屬於自己的傘，走向四種結局之一。

- 類型 / 主題：16-bit 像素風、俯視角、敘事驅動的小品 RPG。核心不是戰鬥，而是
  「淋雨存活 × 道德選擇 × 跨章節漣漪」——第一章對學長的態度，會在第四章才兌現。
- 技術堆疊：C++20（`if constexpr`、CRTP、`std::derived_from`、`std::unique_ptr`、
  `std::move`）＋ Raylib 5.5（以 `FetchContent` 自動取得）。測試用 doctest，建置用
  CMake 3.14+，另備 AddressSanitizer 版本。
- 規模：`include/` 144 個標頭、`src/` 80 個 `.cpp`、`tests/` 569 個測試案例 /
  16,991 個斷言（全數通過）。劇情對白以 9 份 Markdown 放在 `docs/content/`，執行期載入。

倉庫採領域分層（domain-layered）：`include/` 與 `src/` 兩棵樹平行鏡射，頂層皆切成
app / engine / game / ui 四個領域，相依方向單向 `app → game / ui → engine`。

| 領域 | 職責 |
| :--- | :--- |
| `app/` | 組裝根（main.cpp、SceneManager、各 Scene） |
| `engine/` | 與遊戲無關的服務（render 抽象、events、math、audio、input、platform） |
| `game/` | 遊戲模型與控制（world / entities / state / quest / controller / dialog / vendor / gfx） |
| `ui/` | View 與 HUD / overlay / 世界座標提示 |

這個專案最想證明的，是用一套乾淨的 OOP 架構把一個會成長的敘事遊戲撐起來，
而不是把所有東西塞進一個 `main` 迴圈。

---

## 二、OOP 架構與責任分離

### 2.1 一條鐵律：相依方向單向

引擎不認得遊戲。實證：`include/engine/` 底下沒有任何 `#include "game/..."`；遊戲模型
（`World`、各 entity）也不 `#include "raylib.h"`——全宇宙只有 `RaylibRenderer` 這一個
轉接器碰得到 raylib。

### 2.2 GameObject + 角色介面 + CRTP（不用 dynamic_cast）

最初 `GameObject` 是一個肥大介面：每個物件都被迫實作 `Update / Render / Interact`，
即使是空殼。重構後拆成四個單方法角色介面（ISP）：

```cpp
struct IUpdatable    { virtual void Update(float dt) = 0; };
struct IDrawable     { virtual void Render(IRenderer&) const = 0; };
struct IInteractable { virtual void Interact(Player*) = 0; };
struct IMortal       { virtual void TakeDamage(int) noexcept = 0; /* … */ };
```

各類別只扮演自己需要的角色：消耗品只有 `IInteractable`（不動、不自繪）；雨傘沒有
`IMortal`；玩家沒有 `IInteractable`。

關鍵是 CRTP 中介層 `WithRoles<Derived, Base>`，用編譯期 `if constexpr +
std::derived_from` 決定某型別扮演哪些角色，把「查角色」變成零成本的 `static_cast`，
完全不用 RTTI / dynamic_cast：

```cpp
template <class Derived, class Base>
class WithRoles : public Base {
    IUpdatable* AsUpdatable() noexcept override {
        if constexpr (std::derived_from<Derived, IUpdatable>)
            return static_cast<Derived*>(this);   // 編譯期決定
        else return nullptr;
    }
    // …AsDrawable / AsInteractable / AsMortal 同理
};
```

呼叫端寫 `if (auto* u = o.AsUpdatable()) u->Update(dt);`——異質容器裡誰能更新誰就
更新，安全且零開銷。

### 2.3 四個 GoF 設計模式

| 模式 | 落點 | 一句話 |
| :--- | :--- | :--- |
| State | `SemesterStateMachine` + `IChapterState` | 五個章節各是一個狀態；`Transition()` 切換並呼叫 `Exit()/Enter()`。 |
| Factory Method | `GameObjectFactory::Create(ObjectType, pos)` | 12 種物件集中在一個 switch 生成；新增種類只改這裡（OCP）。 |
| Template Method | 傘家族 `BeClaimed()` | `Interact/OnPickup` 先過任務閘，再呼叫純虛 `BeClaimed`；五把傘各自覆寫「被撿走會發生什麼」。 |
| Observer | `EventBus`（Singleton） | `UmbrellaClaimed / KarmaChanged / ShowMessage …` 發布訂閱；`Publish` 先快照 handler 再廣播，避免迭代中失效。 |

### 2.4 MVC + ISystem 模擬管線

- Model（`World`）：擁有一切（物件、FSM、碰撞遮罩、對話/HUD 狀態），無 raylib、無輸入。
- View（`View::Draw(const World&)`）：全程 const 讀模型，只輸出像素，不查輸入、不改狀態。
- Controller（`GameController`）：收輸入、跑模擬、接事件，只變動 `World`，絕不渲染。

最大的改動，是把原本約 793 行的 `Update()` 神之方法拆成一條 `ISystem` 管線——
`SurvivalSystem → MovementSystem → CollisionSystem → SpawnSystem → SweepSystem`，
每個 stage 只做一件事，透過 `SimContext` 串接：

```cpp
SimContext ctx{world_, worldSize_, playerSize_, frameColliders_, {}};
for (const std::unique_ptr<ISystem>& sys : advanceSystems_) sys->Run(ctx, dt);
```

### 2.5 SOLID 對照

| 簡稱 | 全稱 | 中文名稱 | 核心概念 | 對應實作 |
| :--- | :--- | :--- | :--- | :--- |
| S | Single Responsibility | 單一職責原則 | 一個類別只該有一個被修改的理由。 | 神之方法拆成 5 個 system + 4 個輸入處理器 + `QuestHookTable` + `SceneRouter`。 |
| O | Open/Closed | 開放封閉原則 | 對擴展開放，對修改封閉。 | 新增物件＝改 `GameObjectFactory`；新增任務＝在 `QuestHookTable` 加一行，不動控制器。 |
| L | Liskov Substitution | 里氏替換原則 | 子類別必須能完全替換父類別而不破壞功能。 | 五個 `IChapterState`、`Vendor : NPC` 皆可代換；異質容器以角色存取安全。 |
| I | Interface Segregation | 介面隔離原則 | 不強迫客戶端依賴用不到的介面。 | 四個單方法角色介面，物件只實作真正需要的。 |
| D | Dependency Inversion | 依賴反轉原則 | 高低階模組都依賴抽象，而非具體實現。 | `Render` 只認 `IRenderer&`；輸入經 `InputSource` 抽象，可在真鍵盤與腳本輸入間切換。 |

---

## 三、遊戲核心機制

### 3.1 雨量表＝這款遊戲的「HP」

玩家不會被打死，而是淋太久被沖回正門。雨量 `rainMeter_ ∈ [0,100]`，由
`SurvivalSystem` 每幀三選一：

```
室內            → DrainRain          (-10/s，完全回復)
室外 + 撐傘      → ApplyRainSheltered  (+1.5/s，緩慢淋濕)
室外 + 無傘      → ApplyRain          (+5/s，滿格傳送回正門)
```

設計含義是：傘換來的是「時間」而非「免疫」。撐傘仍會慢慢濕，玩家得不時躲進建築
完全回復。背包道具（防水噴霧 −35／暖暖包 −25／飲料・小吃 −15）則是離散的即時扣量。

### 3.2 業力與「詛咒污點」

`AddKarma(delta)` 夾制在 `[-100,100]` 並發 `KarmaChanged`（初始 50）。詛咒傘不是
一次性扣分，而是 `IncCursedTaint()`：每進一個編號章節扣 `5 × 污點數`，讓「拿詛咒傘」
像一條跨整場滑下去的道德代價（污點為 0 完全不觸發，乾淨流程逐位元一致）。

### 3.3 循環經濟：賺 → 花，每章歸零

- `Vendor::TryBuy` 三道閘：售罄 → 餘額不足（`DeductMoney` 把關，失敗無副作用）→
  成交（發提示＋`PickupAcquired`，可選 `setsFlag`）。
- 金錢軟上限 300、跨章保留；消耗品每次進市集清空（`ClearConsumables`），逼玩家每章
  重新做「為下一章採買什麼」的決策——這就是循環的張力來源。

### 3.4 章節脊柱與四種結局（優先序 A → B → D → C）

狀態順序：`Ch1_加退選 → 幕間市集 → Ch2_期中考 → 幕間市集 → Ch3_運動會 → 幕間市集 →
Ch4_期末考 → 結局`。`EndingGate` 只在 Ch4 且對話關閉時判定，順序就是優先序：

| 結局 | 條件（簡述） | 一句話 |
| :--- | :--- | :--- |
| A 雨過天晴 | `karma > 80` ＋ 持真傘 ＋ 體諒助教 | 誠實高業力的真結局 |
| B 屠龍者終成惡龍 | 拿過詛咒傘／`karma < 0`／冷漠終局 | 墮落路線 |
| D 風雨同行 | 體諒助教，但未達 A（業力不夠或沒真傘） | 苦甜：心善，但傘已被整學期風雨磨穿 |
| C 破財消災 | 到集英樓買醜綠傘（或兜底） | 務實、平穩的預設 |

D 排在 C 之前，確保「選擇體諒」的玩家不會被「買醜傘」蓋過。（此為四結局；先前
in-game 說明只列三種，已修正對齊。）

### 3.5 旗標驅動 × OCP 任務分派

- `Flags.h` 是唯一事實來源：所有 `SetFlag/HasFlag` 都用具名常數，`src/`、`include/`
  內不准出現裸 `"Flag_X"` 字面值。
- E 互動的任務副作用全部走資料化的 `RunInteractHooks`（每個 hook 各自以
  `(state, npcId)` 自我守門），新增任務只要加一行登記，不改控制器。
- NPC 頭上的「!」由 `QuestIndicatorVisible` 依章節規則點亮（如 Ch2：管理員 → 學霸 →
  缺飲料才亮的販賣機 → 學霸）。

---

## 四、程式碼細節與除錯

### 4.1 測試與「確定性重播」

569 個 doctest 案例、16,991 個斷言，依 domain 分目錄（quest 最多）。最特別的是
Harness / ScriptInput 確定性重播：把輸入換成腳本來源、時間固定為 1/60，於是同一腳本
跑兩次，每一幀的 `{x, y, dialog, cursor, npc}` 逐位元相同——`test_scriptinput_plan`
直接 `CHECK(identical)`。預設關閉時對正常遊玩逐位元無影響。

### 4.2 除錯戰記（五則）

1. 字型圖集啟動崩潰（「進去選角 crash」）。CWD 不在資產目錄時，退路想烘入整段約
   兩萬個 CJK 字（`U+4E00..9FFF`），圖集寬度爆掉 Intel／舊 Mac 的
   `GL_MAX_TEXTURE_SIZE`，第一次 `DrawTextEx` 就崩潰。
   修法：新增 `EnsureAssetWorkingDir()` 先 chdir 到執行檔旁；`CollectCodepoints` 退路
   只烘標點並硬性上限 4096 字。

2. 攤販「十個一模一樣的分身」。舊版所有攤位都用字面值 `"vendor"` 當選圖鍵，雜湊全撞、
   退路又全用同一張 `shop_auntie.png`。
   修法：改用「生成索引」對精選清單取模，保證十攤各異（附回歸測試
   `test_vendor_centred_cluster`）。

3. Ch2 借傘蓋過真傘、背包洩漏。借傘佔了 `heldUmbrella_` 槽，換回真傘時互相覆蓋。
   修法：借傘改為純旗標驅動（`Flag_LibrarianUmbrella` ＋ `SetHasUmbrella(true)`），
   不佔槽，於是真傘與借傘並存、各列一行；歸還借傘時以 `keepShelter` 保住真傘。

4. Ch2「!」順序錯亂。學霸在見過管理員前就亮、跳過了引導。
   修法：`Ch2IndicatorVisible` 改以 `npcId` 精確狀態驅動——管理員見過即熄、學霸接手、
   販賣機只在「缺飲料喚醒」時當中繼。

5. 助教跨章節旗標只讀、不寫的死線索。`Flag_HelpedTA_Ch1` 在 Ch2/Ch3/Ch4 的對白與
   結局名冊共五處被讀取，卻沒有任何程式路徑設過它，整條助教支線形同虛設。
   修法：在 Ch1 交還申請書時設旗標（karma +5），並補上 Ch3 兌現的 +5 漣漪；附一個
   釘住「設置→兌現」的回歸測試。

> 這五則的共通方法論：先寫一個會失敗的測試重現問題 → 找根因 → 最小修正 →
> 全套 569 測試綠燈。

---

## 五、表達與誠實程度／心路歷程

坦白說，這個專案的每一行程式碼幾乎都不是我親手敲出來的——但它的每一個設計決策都
經過我點頭。如果要給自己一個定位，我是這個專案的架構師與導演：我決定方向、把關品質；
AI 是我手上一個非常強的工具，負責把想法落地成 569 個測試都綠燈的程式。

我帶進來的，不是一句「幫我做個遊戲」，而是三樣具體的東西：

1. 一套 OOP 架構的方向感。我要求分層、要求責任分離、要求「Model 不准認得 raylib」
   這種鐵律，並且選定了 GoF 四個模式各自的落點——State 管章節、Factory 生物件、
   Template Method 給五把傘各自的 `BeClaimed`、Observer 用 `EventBus` 解耦。
2. 我自己理解、而且是我點名要用的 C++ 技術。`std::move` 搭配 `std::unique_ptr<T>`
   做所有權轉移與 RAII，讓物件生命週期乾淨、不漏記憶體；還有一招是用 CRTP 靜態多型
   （`WithRoles`）取代 `dynamic_cast`，把「查角色」變成編譯期就決定的零成本操作。
   這不是 AI 自己冒出來的，是我希望它「要快、而且不要用 RTTI」的想法。
3. 敘事與機制的連貫性。四種結局的優先序、業力與詛咒污點怎麼跨章節滑下去、雨量表
   為什麼是「給時間而非免疫」、循環經濟為什麼每章清空——這些「為什麼這樣設計」的
   邏輯是我給的，AI 把它翻成程式與旗標。

AI 做的是最重的活：把我的規格變成能編譯、能跑、有測試的程式，補我沒想到的邊界情況；
當我說「這裡太亂、拆掉那個 793 行的 god method」時，它真的把它拆成一條 `ISystem`
管線。它的價值不在於「會寫 code」，而在於能承接一個有架構主張的人的決策，並穩定落地。

我們的合作方式，是我親自試玩、回報 bug（進選角的 segfault、十個一樣的攤販、Ch2
借傘蓋過真傘、感嘆號順序），有時提供 backtrace，然後盯著它走「先寫一個會失敗的測試
重現 → 找根因 → 最小修正 → 全套測試綠燈」這條路。我不接受「看起來會動」，我要看到
全部測試通過。

我學到的是在過程裡把 move 語意、CRTP、MVC 的責任邊界、
為什麼不要 `dynamic_cast` 這些東西，從「聽過」變成「能對它的實作提出意見」。對我來說，
這才是這份作業最大的收穫——重點不是我會不會背 C++ 語法，而是我能不能判斷一個架構
好不好、並要求把它做到位。
 
這個專案的品味、那條「什麼叫做好」的標準、以及把它一路修到對齊的耐心甚是重要。在 AI 時代，domain knowledge 是不可或缺的，一定要有最底層的知識與概念，AI 與人合作的能力才會凸顯價值出來。

---

## 附：建置與執行

```bash
cmake -B build && cmake --build build   # 設定並編譯（含遊戲與測試）
./build/umbrella                        # 執行遊戲
ctest --test-dir build                  # 跑測試（CTest，目標名 unit_tests）
./build/umbrella_test                   # 或直接執行 doctest 測試二進位
```

> 附註：`umbrella` / `umbrella_test` 是實際產生的執行檔；`OOP_Raylib_Lab` 只是
> CMake 內部的目標名稱，不是可執行檔。
