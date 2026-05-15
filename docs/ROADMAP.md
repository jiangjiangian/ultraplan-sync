# ROADMAP — 從 MVC 骨架到可從頭玩到尾的完整遊戲

本檔是《尋傘記：政大山下篇》後續開發的單一事實來源（SoT）。
上位設計文件仍為 `遊戲企劃與敘事架構.md`（劇情/數值/結局）、
`系統架構與UML分析：尋傘記.md`（class 圖/狀態機）、`docs/SCRIPT_HANDOFF.md`
（腳本端↔程式端契約）。本檔只負責「還沒接的東西，依什麼順序接」。

範圍決議：**Tier 2（Phase 0 + 1 + 2）= 完整劇情**。對話系統採
**設定期 codegen**。Tier 3 打磨項列於文末，屬延伸，不在本輪承諾內。

核心體驗定義（明確假設）：俯視角探索 + 與 NPC 對話推進任務 + 撿道具 +
任務閘門 + 世界隨學期章節變化。沿用現有 `worldmap.png` + camera-follow
的世界幾何；**不**新增小地圖 UI 疊層。

---

## 一、現況盤點（2026-05-15，以實際程式為準）

專案說明檔內的「程式未開始」進度快照已過時，以本節為準。

### 已完成

- 完整 class 樹：GameObject → Character/Item → Player/NPC、
  TransparentUmbrella + 4 子類、ConsumableItem + 3 子類、CashPickup、
  Vendor、GameObjectFactory。
- 四個 GoF pattern 皆有具體實作；MVC 已純化（World=純資料、
  View=渲染+鏡頭、GameController=輸入/模擬/事件接線、main=40 行組裝根）。
- Player 欄位齊全：`rainMeter_`、`karma_`(起始 50)、`money_`(起始 100)、
  `hasUmbrella_`、`flags_`（`SetFlag`/`HasFlag`/`ClearFlag`）。
- SemesterState 八態：Chapter1_AddDrop / Interlude_Market /
  Chapter2_Midterms / Chapter3_SportsDay / Chapter4_Finals /
  Ending_A / Ending_B / Ending_C；State pattern 已就位。
- 玩家 + 5 主 NPC sprite 已存在；檔名契約鎖在 `NpcSpawns.h`、
  `CharacterSelect.cpp`（`sprites/npc/*.png`、`sprites/school_uniform_3/`）。
- 測試 19 檔 / 93 case / 290 斷言全綠。

### 缺口（依「能不能玩」排序）

1. **`docs/content/*.md`（2745 行劇本）零接入** — 全部 NPC 對白、
   四章劇情、三結局都還在 markdown，遊戲內一句都沒有。
2. **無對話框 UI 與對話流程** — NPC 有 `dialogLines` 資料容器，
   但沒有 DialogBox 渲染、推進、分支選擇。
3. **rainMeter 只累積不懲罰** — 50%/80% 降速、室內/撐傘止累、
   暗角特效、昏倒傳送正門皆未進模擬（`Player::Update` 無條件 +5/dt）。
4. **章節推進是假的** — 目前「走進建築 = 下一章」(`enterTrigger_`)，
   非 GDD 的任務閘門（取得 TrueUmbrella 過關、漣漪 flag → Ch4 callback）。
5. 三結局無演出；無 Tab 物品欄；`ProfessorTrapUmbrella` 助教追逐
   為 `// simulated` TODO；圖書館強制慢走未做；無存檔。

---

## 二、對話系統架構（codegen 方案）

### 設定期管線

新增 `tools/gen_dialog.py`：解析 `docs/content/chapter{1..4}.md`、
`interlude_market.md`、`ending_{a,b,c}.md`、`voice_bible.md`，輸出
`include/DialogData.h`（純靜態 `constexpr`/`inline` 陣列，風格對齊既有
`Buildings.h`/`NpcSpawns.h`），不重編無法改劇本，與本專案 data-as-header
慣例一致。

腳本檔內每個 `### (a)/(b)/(c)` 區塊下的 `- "…"` 條目，依序對應
`dialogLines[0..N]`。Key = `(npcId, SemesterState, subState)`。
`// karma +N` / `// karma -N` 註解解析成該 subState 的 karma delta；
`Flag_xxx` 解析成該分支要 `SetFlag` 的旗標名。

`DialogData.h` 對外介面（草案）：

```cpp
namespace nccu::dialog {
struct Line  { std::string_view npcId; int subState;
               std::span<const std::string_view> lines;
               int karmaDelta; std::string_view setsFlag; };
std::span<const Line> For(std::string_view npcId, SemesterState s);
}
```

`NPC` 載入時呼叫既有 `SetDialogLines(state, subState, lines)` 把資料灌入。

### 執行期配套

- **View**：新增 `DialogBox` 元件（先用 `IRenderer::DrawRect` +
  `DrawText` 佔位框，`resources/assets/ui/` 圖一落地即換貼圖）。
  每行 ≤ 28 全形字（`SCRIPT_HANDOFF` 鎖定的對話框寬度）。
- **GameController**：新增 `DialogSession`（當前 NPC、行游標、
  是否選擇模式）。E 鍵推進；有選項時上下鍵移游標、E 確認，
  選定分支 → `Player::SetFlag(...)` + `AddKarma(delta)`。
- 對話進行中凍結移動/碰撞模擬，只走對話輸入。

### 旗標清單（取自 SCRIPT_HANDOFF，型別 `std::unordered_map<string,bool>`）

Ch1：`Flag_HelpedSenior` / `Flag_ScoldedSenior` / `Flag_HasProfessorTrap`
/ `Flag_TookCursedUmbrella` / `Flag_HelpedTA_Ch1` / `Flag_PromisedVictim`
/ `Flag_SawVictim_Ch1` / `Flag_BoughtUglyUmbrella` /
`Flag_BoughtCoffeeForAuntie_Ch1`。Ch2：`Flag_BookwormRecovered`。
Ch3 主要重用前述旗標。

鎖定數值（不再改）：karma 起始 50、刻度 ±3/±5/±10、大事 -15/-30、
Ending A 門檻 >80、Ending B 門檻 <0；money 起始 100；rainMeter ≥100%
→ 傳送正門結算區、時間推進半天、不扣 karma；Ending C 觸發點在
集英樓便利商店。

---

## 三、分階段路線圖

### Phase 0 — 對話資料管線（前置，所有東西的根）

交付：`tools/gen_dialog.py`、`include/DialogData.h`（產出物，入 git）、
codegen 在 `cmake -B build` 前或 CMake 自訂 target 內可重跑、
`NPC::SetDialogLines` 接線、parser 單元測試（餵一段 markdown 斷言陣列）。

驗收：`DialogData.h` 編得過；測試含對白計數/karma delta/flag 解析；
全測試仍綠。

### Phase 1 — 可玩脊椎（Chapter 1 端到端）

交付：

- `DialogBox` 元件 + `DialogSession`；E 推進、選項 → flag/karma。
- Ch1 任務鏈接線：綜院苦主對話 → 得知西裝學長 → 冒雨往集英樓 →
  便利店店員跑腿（四維道找帳單）→ 集英樓 2F 找到學長 → 漣漪抉擇
  （A 溫和+請咖啡 `Flag_HelpedSenior`／B 斥責搶傘 `Flag_ScoldedSenior`）。
- 章節閘門：取得 `TrueUmbrella` → `SemesterStateMachine` 推進
  Ch1 → Interlude（取代現行「走進建築就跳章」的 `enterTrigger_`）。
- 一個結局畫面模板（全螢幕字卡 + 淡出），先接 Ending C 走通流程。

驗收：可從 CharacterSelect 一路對話、跑腿、抉擇、過 Ch1，
旗標確實寫入且 `karma` 隨抉擇變動；窗存活 ≥5s 無崩潰。

### Phase 2 — 全劇情（= 完整遊戲）

交付：

- Interlude 市集：10 Vendor（1 class + 10 VendorConfig）+ 消耗品購買
  （HotPack/WaterproofSpray/EnergyDrink）+ Tab 物品欄 UI + 消耗動作；
  公告板出口推進至 Ch2。
- Ch2 圖書館期中考：圖書館管理員 NPC、收集筆記/提神飲料、
  羅馬廣場喚醒學霸換傘、`Flag_BookwormRecovered`。
- Ch3 校慶運動會：物物交換鏈三 NPC（A 香腸攤主 → B 大聲公 →
  C 學姊情報）→ 體育館後台道具箱取傘。
- Ch4 期末考終焉：地圖全開探索；**漣漪 callback** — 若
  `Flag_HelpedSenior` 則西裝學長現身行政大樓給線索，否則敷衍；
  追到研究室崩潰助教。
- 三結局完整演出：A（karma>80 + TrueUmbrella + 體諒助教）、
  B（karma<0 / CursedUmbrella，畫面永久灰暗字卡）、
  C（集英樓便利商店買醜綠傘，花光金錢）。
- 全章 NPC 對白由 Phase 0 管線注入。

驗收：四章 + 過場 + 任一結局可連續跑完；漣漪效應在 Ch4 對
`Flag_HelpedSenior` 有可觀察的對白差異；三結局皆可由對應路徑觸達。

---

## 四、與並行美術管線的協調

並行的圖片生成工作只動 `resources/assets/`（對話框/HUD/雨量計外框、
追加 sprite）。程式工作只動 `include/`、`src/`、`tools/` — 不同檔案樹，
無合併衝突。

- DialogBox/物品欄/結局字卡先用畫框佔位，`resources/assets/ui/`
  圖一落地即換貼圖，互不阻塞。
- sprite 檔名契約已鎖在 `NpcSpawns.h`、`CharacterSelect.cpp`；
  美術產出須照既有檔名，不得另立。
- **推送前**：先 `git pull --rebase origin main` 併入美術的
  `resources/` commit，再由使用者自行 `git push origin main`。

---

## 五、每階段共同驗收 gate

1. `cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5` 後
   `cmake --build build`，專案碼零警告（`-Wall -Wextra`）。
2. 既有測試全綠 + 該階段新增測試綠。
3. 二進位開窗 ≥5s 不崩、該階段新功能可實際操作。
4. 不在迭代中刪 GameObject（沿用 end-of-frame sweep）。
5. 推送內容不得出現外部工具/服務名稱（沿用既有禁忌字檢查）。
6. 不 `git push`（由使用者執行）；不 `rm -rf build`（信任增量編譯）。

---

## 六、Tier 3 打磨（延伸，不在本輪承諾）

rainMeter 50%/80% 降速 + 邊緣暗角 + 室內/撐傘止累、Shift 奔跑
（雨中奔跑淋雨加倍）、圖書館強制慢走 70% + 安靜 BGM、Ch3 市集帳篷
動態場景 overlay（`ApplyStateOverlay`）、`ProfessorTrapUmbrella`
真生成追逐助教 AI、音效/BGM、存檔（Interlude 公告板存檔點、旗標持久化）。
