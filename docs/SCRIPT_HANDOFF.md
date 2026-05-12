# 腳本端交付清單（給 implementation session）

腳本撰寫階段已完成。本檔說明**已落地的內容**、**鎖定的設計決策**、**程式端要接的契約**、**還沒拍板的旋鈕**。所有腳本檔在 `docs/content/`。

---

## 一、已交付的 9 份腳本檔

| 檔案 | 行數 | 用途 |
|---|---|---|
| `docs/content/voice_bible.md` | 363 | 5 NPC 人設 / 口頭禪 / 四章 voice arc / Karma 高低語氣差。其他章節對白要對齊本檔。 |
| `docs/content/chapter1.md` | 260 | Ch1「加退選之亂」全部對白 + 場景旁白 + 章節結尾 4 種傘分支 |
| `docs/content/interlude_market.md` | 390 | 過場市集：經濟系統、3 消耗品、Vendor 模板、10 攤位完整對白、`MapManager::ApplyStateOverlay()` 介面草稿 |
| `docs/content/chapter2.md` | 387 | Ch2「圖書館期中考」+ 圖書館管理員（新 NPC）+ Ch1 旗標 callback |
| `docs/content/chapter3.md` | 356 | Ch3「校慶運動會」+ 物物交換鏈 3 個新 NPC（A/B/C 系攤主）+ Ch1+Ch2 旗標 callback |
| `docs/content/chapter4.md` | 402 | Ch4「期末考終焉」+ 5 NPC peak emotional intensity + 三結局觸發點 |
| `docs/content/ending_a.md` | 219 | 真相大白（karma>80 + TrueUmbrella + 助教體諒） |
| `docs/content/ending_b.md` | 202 | 詛咒冰封（karma<0 / CursedUmbrella） |
| `docs/content/ending_c.md` | 166 | 破財消災（集英樓便利商店買醜綠傘） |

**全部 2745 行**。已通過禁忌字檢查（內容中沒有任何外部工具或服務名稱）。

---

## 二、鎖定的設計決策（不要再改）

### Karma 系統
- **起始 karma = 50**（與 `Player` ctor 預設要對齊）
- **刻度：×0.6 累積型**——`±3 / ±5 / ±10`（小事），`-15 / -30`（大事）
- **Ending A 門檻 karma > 80**、**Ending B 門檻 karma < 0**
- 設計意圖：單一選擇不鎖結局，玩家需累積多個善/惡行為才到結局門檻

### Player 金錢
- **起始 money = 100 元**
- 三條賺錢管道：NPC 打賞（quest 完成）+ 地圖 pickup（CashPickup : Item）+ 市集小遊戲

### 死亡懲罰
- rainMeter ≥ 100% → **傳送回正門結算區**（不是宿舍——目前無宿舍場景）
- 時間推進半天，**不扣 karma**

### 對白格式
- **每行 ≤ 28 全形字**（dialog box 寬度限制）
- 每個 NPC 在每章按 `(a) / (b) / (c)` 子狀態分組，對應 `dialogLines[0..N]`
- Karma 變化用 markdown 註解 `// karma +N` 或 `// karma -N` 標出，方便程式端對照

### Ending C 觸發點
- 在 **集英樓便利商店**（不是樂活小舖）
- Ch1 福利社阿姨只是 Ending C 的「種子」——讓玩家認識醜綠傘的存在
- Ch2/Ch3 各自累積 `Flag_BoughtUglyUmbrella` 計數
- Ch4 集英樓便利商店「再次購買」才最終觸發

---

## 三、程式端要接的契約

### NPC dialog 注入

```cpp
// 每個 NPC 在每章載入時：
NPC* tutor = factory->CreateNPC("tutor");
tutor->SetDialogLines(state, subState, lines);
// state    = SemesterState (Chapter1_AddDrop / Interlude_Market / ...)
// subState = (a) / (b) / (c) / (d)  --- enum 或 int
// lines    = std::vector<std::string>，對應腳本檔 markdown 的 - "..." 條目
```

腳本檔內每個 `### (a) / (b) / (c)` 區塊下的 `- "..."` 條目，**直接對應 `dialogLines[0..N]`**。

### Karma 變化注入

```cpp
// 腳本檔的 // karma +5 註解 → 程式端 hook 點：
void NPC::OnInteract(Player& p, int subState) {
    // ... 對白播放完
    p.AddKarma(this->karmaDeltaForSubState(subState));
}
```

### 旗標系統（Flag_xxx）

腳本檔出現的 `Flag_xxx` 全部是 `bool` 旗標，建議用 `std::unordered_map<std::string, bool>` 或 enum-keyed bitset。完整旗標清單：

**Ch1 設立**：
- `Flag_HelpedSenior` / `Flag_ScoldedSenior` — 玩家對西裝學長的反應
- `Flag_HasProfessorTrap` — 持有 ProfessorTrapUmbrella
- `Flag_TookCursedUmbrella` — 拿 CursedUmbrella
- `Flag_HelpedTA_Ch1` — 完成助教跑腿 quest
- `Flag_PromisedVictim` / `Flag_SawVictim_Ch1` — 對苦主的反應
- `Flag_BoughtUglyUmbrella` — 在福利社阿姨處買綠傘
- `Flag_BoughtCoffeeForAuntie_Ch1` — voice bible 引入，建議在 Ch1 補一個觸發點

**Ch2 設立**：
- `Flag_BookwormRecovered` — Ch2 用提神飲料喚醒學霸、換回傘

**Ch3 設立**：
- 主要重複利用 Ch1+Ch2 旗標，少量新旗標可在實作時定

### 必要 class（GoF 對齊）

| 類別 | 父 | 用途 | 來源 |
|---|---|---|---|
| `ConsumableItem` | `Item` | 抽象基類，與 `TransparentUmbrella` 並列 | interlude_market.md |
| `HotPack` / `WaterproofSpray` / `EnergyDrink` | `ConsumableItem` | 3 種消耗品，價格/效果見 interlude_market.md | interlude_market.md |
| `CashPickup` | `Item` | 地圖上的金錢拾取物（5/10/20 元） | interlude_market.md |
| `Vendor` | `NPC` | 1 個 class + 10 份 `VendorConfig` 跑 10 攤位 | interlude_market.md（已有 struct 草稿） |

`GameObjectFactory` 至少要有：`CreateNPC` / `CreateUmbrella` / `CreateConsumable` / `CreateVendor` / `CreateCashPickup`。

### `SemesterStateMachine` 五個狀態

```
Chapter1_AddDrop → Interlude_Market → Chapter2_Midterms → Chapter3_SportsDay → Chapter4_Finals
                                                                                        ↓
                                                                  Ending_A | Ending_B | Ending_C
```

切態邏輯：
- 每個 chapter 結尾的「分支提示」section 寫了切態條件（拿到哪把傘 / karma 條件）
- `MapManager::ApplyStateOverlay(state)` 接 Observer mode：每次切態 MapManager 與 UIManager 自動更新

---

## 四、新 NPC 清單（5 主 NPC 之外）

腳本中引入了主 5 NPC 之外的角色，程式端要為他們生成 sprite + 加進 NPC roster：

| NPC | 章節 | 功能 |
|---|---|---|
| 圖書館管理員 | Ch2 | 純資訊節點，不給 karma |
| A 系烤香腸攤主 | Ch3 | 物物交換鏈第一棒 |
| B 系大聲公持有者 | Ch3 | 物物交換鏈第二棒 |
| C 系學姊 | Ch3 | 物物交換鏈第三棒、給情報 |
| 10 個 Vendor（市集） | Interlude | 用 Vendor class + VendorConfig 跑 |
| 公告板 NPC | Interlude | 場景物件、不是人類 NPC、做為 Interlude 出口 |

---

## 五、還沒拍板的旋鈕（implementation session 可決定）

腳本端列出但尚未確認的設計選項，整理在各檔末尾的「待覆核 / 自行補的設定」section。摘要如下：

### voice_bible.md
1. 5 NPC 的系所/年級全部是腳本端推斷（西裝學長財金大四 / 學霸法律大三 / 助教資工碩二 / 阿姨在職十年 / 苦主大一下）
2. `Flag_BoughtCoffeeForAuntie_Ch1` 的觸發條件需在 Ch1 程式端決定（建議在福利社阿姨對話加一個分支）

### interlude_market.md
1. **金錢上限**（建議 999 防超量）
2. **市集是否做存檔點**（建議在公告板 NPC 加存檔選項）
3. **章魚燒攤的下章劇情提示具體內容**（目前是泛指圖書館，待 Ch2 對白定稿後微調）
4. **Vendor sprite 共用策略**（10 個攤主可共用 1 base sprite + 帽子/圍裙顏色變化，省生圖預算）

### chapter2.md
1. **學霸 Ch2 初始位置 = 圖書館三樓靠窗**（voice bible 提到「三樓靠窗」是他習慣坐的地方）
2. **圖書館管理員借 FragileUmbrella** 給玩家當戶外追人的緩衝
3. **EnergyDrink 替代取得路徑**：圖書館地下室自販機 35 元（玩家沒在 Interlude 買時的容錯）

### chapter3.md
1. **苦主 Ch3 在角落擺手工飾品攤**——維持存在感但不介入主線
2. **A 系攤主前置條件**：要玩家「先幫她問 B 系要衛生紙」（強化物物交換鏈連鎖感）
3. **C 系學姊指定道具箱位置**「第三個，從左邊算」——給玩家具體行動目標

### chapter4.md
1. **Ending B 在 Ch4 研究大樓走廊新設「傘架場景」**——讓首次玩家也能本地觸發 Ending B，不依賴 Ch1 舊旗標
2. **`Flag_HelpedTA_Ch1` + `Flag_HasProfessorTrap` 同時成立時** (b) 優先、(c) 的 -15 以系統訊息獨立補扣

---

## 六、建議的程式端 implementation 順序

1. **基礎類別**：`GameObject` → `Character`/`Item` → `Player` / `NPC` / `TransparentUmbrella` 抽象
2. **Player 內部欄位**：karma、money、rainMeter、Inventory、HasFlag(name)
3. **GameObjectFactory**：先做 `CreateUmbrella` / `CreateNPC`（Assignment 5 評分必要）
4. **4 個 TransparentUmbrella 子類**（True / Fragile / ProfessorTrap / Cursed）+ `beClaimed()` polymorphism
5. **3 個 ConsumableItem 子類** + `Consume()` polymorphism
6. **Vendor class + VendorConfig**（用同一 class 跑 10 攤位）
7. **SemesterStateMachine**（State pattern）+ `ApplyStateOverlay` Observer
8. **NPC dialog 載入**：把 `docs/content/*.md` 的對白人工或 script 轉成 `SetDialogLines()` 呼叫
9. **EventBus / IObserver mixin**（karma / rainMeter 變化廣播給 UI）
10. **MapManager 物件生命週期**：deferred deletion（marked-then-swept），絕不在迭代中刪
