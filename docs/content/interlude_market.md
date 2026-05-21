# Interlude：四維道常態市集

## 章節 metadata

- SemesterState: `Interlude_Market`
- 觸發: 玩家完成 Ch1 章節結算（任意通關路徑）後自動切換
- 結束: 玩家走出市集南端的離開觸發區（player.y ≥ 1900 自動 set `Flag_LeaveInterlude`）→ 進入 `Chapter2_Midterms`
  - 設計變更紀錄: 公告板 NPC 對話 → 靜默 trigger zone（配合 `include/InterludeExit.h:13-19` 與 `src/GameController.cpp:511`，玩家從南端走出即觸發 `Flag_LeaveInterlude`；不再有人類型 NPC、不再有「離開」對話選項）
- 場景: 四維道（與 Ch1 共用同一張 worldmap，差別由 `MapManager::ApplyStateOverlay()` 處理）
- **特殊規則**:
  - 雨粒子降到 10% 強度（小雨而非暴雨）
  - `rainMeter` 累積率歸零，本章節不淋雨
  - 沒有「找傘」主線——這是喘息點

---

## 經濟系統

### Player 金錢欄位

```cpp
class Player : public Character {
    int money = 100;  // 學期開始家長轉帳的零用錢
    // ...
};
```

- **起始值: 100 元**（GDD 未明訂；本檔釘在 100，方便所有售價對齊）
- **顯示位置**: HUD 右上角，緊鄰 rainMeter

### 三條賺錢管道（玩家可在 Interlude 之前/之中累積）

| 管道 | 觸發 | 額度 | 程式對應 |
|---|---|---|---|
| **NPC 打賞** | 完成 karma+ quest 後，部分 NPC 額外給錢 | 助教跑腿 +30 元、苦主感謝 +20 元（karma 高才解鎖） | `NPC::OnQuestComplete()` 內呼叫 `player.AddMoney(amount)` |
| **地圖 pickup** | 地圖上散落小額銅板/紅包 | 5/10/20 元 不等，總量約 50 元 | `CashPickup : Item`（Factory Method 多一條生產線） |
| **市集小遊戲** | 套圈圈攤 5 元/次、二手書攤賣道具回收 | 套圈圈期望值 -1.7 元/次（賭博）、二手書回收約 30~60 元 | 攤位內建小遊戲 sub-state |

> 設計意圖: 三條管道分別代表「德行報償」、「探索獎勵」、「策略選擇」。對應 `karma > 80` 走完整路線的玩家進 Interlude 時應有約 150~200 元，剛好能挑兩件消耗品；karma 低的玩家進 Interlude 約只剩 100 元，必須割捨。

---

## 三種消耗品（GDD 指定 2 + 連動 Ch2 加 1）

| Item | 售價 | 效果 | Ch2/Ch3 連動 |
|---|---|---|---|
| **暖暖包** `HotPack` | 25 元 | 使用後 60 秒內 `rainMeter` 累積率 ×0.5 | 體育館場景額外 +5 體溫值 |
| **防水噴霧** `WaterproofSpray` | 50 元 | 使用後 90 秒內 `rainMeter` 完全凍結 | 噴在書包上：圖書館場景 quest 道具加分 |
| **提神飲料** `EnergyDrink` | 35 元 | 使用後 30 秒內移動速度 +30%；可給 NPC | **Ch2 學霸 quest 道具**（不買就只能在圖書館自販機找） |

### Class 結構（與 `TransparentUmbrella` 並列，共用 Item Factory）

```cpp
class Item;  // abstract

class TransparentUmbrella : public Item { /* 已有 */ };

class ConsumableItem : public Item {
public:
    virtual void Consume(Player& player) = 0;  // Template Method 變體
    int  price;
    bool isStackable = true;
    int  quantity = 1;
protected:
    float effectDurationSec;
};

class HotPack         : public ConsumableItem { void Consume(Player&) override; };
class WaterproofSpray : public ConsumableItem { void Consume(Player&) override; };
class EnergyDrink     : public ConsumableItem { void Consume(Player&) override; };
```

`GameObjectFactory` 多一個 `CreateConsumable(ItemType)` overload——直接讓 Assignment 5 的 Factory Method 評分多一個具體例證。

---

## Vendor NPC 模板（10 攤位用同一個 class）

### 設計關鍵: **不寫 10 個子類**

- `Vendor` 繼承自 `NPC`，行為由「stock 資料」驅動而非繼承層
- 10 個攤位 = 1 個 `Vendor` class + 10 份 `VendorConfig` 結構（資料）
- `GameObjectFactory::CreateVendor(VendorConfig)` 創建出每個攤位 instance
- 對話腳本由 `VendorConfig::dialogLines` 提供，攤主名與商品擺設由配置決定

### 介面草稿

```cpp
struct StallSlot {
    Item* item;          // ConsumableItem* 或 flavor item
    int   price;         // 元
    int   stockLeft;     // -1 = 無限
};

struct VendorConfig {
    std::string vendorName;       // 例: "炸物阿姨"
    std::string stallSign;        // 例: "熱騰騰雞排攤"
    std::vector<StallSlot> stock;
    std::vector<std::string> greeting;
    std::vector<std::string> onPurchase;
    std::vector<std::string> onLeave;
    int   karmaOnInteract = 0;   // 例: 募款攤 +1
    Vector2 worldPos;
};

class Vendor : public NPC {
public:
    void Configure(const VendorConfig& cfg);
    void OnInteract(Player& p) override;  // 開購買 UI
private:
    VendorConfig config_;
};
```

---

## 10 攤位 lineup

> 命名原則: 攤主稱謂中性、避免將特定外型/性別/職業綁定到負面刻板。「阿姨/伯伯」是親切稱呼；「學長/學姊/學弟妹」表學生身分；單獨的「攤主」用於不需要人格化的點。

> 解析格式（`LoadInterludeVendors()` 讀取，`## NPC：` 的兄弟）：每攤一段
> `## 攤位：<攤名>`，緊接 `>` 註記：`> 攤主：<人>`、0..n
> `> 商品：<itemId> = <價>`、`> 機制：<buy|donate|sell|game|flavor>`、
> 選用 `> tier：<N>`、`> karma：<±N>`、`> stock：<N>`。對白以
> `### greeting` / `### onPurchase` / `### onLeave` 子標題分段，每行
> `- "…"`。變體括註（如 `### onPurchase（陷阱傘殘骸）`）原樣保留；
> Phase 2 取同類首塊為通用，其餘記錄不行使。itemId 為英文正規名，
> 對應 ConsumableItem 子類（Tier 2 風味食物無 Consume 效果，純金錢
> sink，回復 buff 屬 Tier-3 不在本輪）。

## 攤位：熱騰騰雞排攤
> 攤主：炸物阿姨
> 商品：HotPack = 25
> 機制：buy
> tier：1

### greeting
- "同學淋這麼濕，吃塊雞排暖一下。"
- "今天順便帶包暖暖包，二十五塊一包。"
- "阿姨自己都囤好幾包了，山下風大。"

### onPurchase
- "對嘛，年輕人就是要照顧好自己。"
- "雞排趁熱吃，暖暖包記得搓開。"

### onLeave
- "下雨天要小心啊，慢走。"

## 攤位：校友手沖咖啡
> 攤主：手沖咖啡學長
> 商品：EnergyDrink = 35
> 機制：buy
> tier：1

### greeting
- "手沖咖啡，現點現做，三十五塊。"
- "畢業前最後一次擺攤，喝完保證撐過下禮拜。"
- "選豆子我花了三天，不誇張。"
- "你看這個色澤——好啦我知道你趕時間。"

### onPurchase
- "你眼光不錯，這支衣索比亞日曬很少人懂。"
- "雨天喝熱的，效果最好，慢慢走。"

### onLeave
- "期中加油，有緣再見。"

## 攤位：文創手作攤
> 攤主：手作攤主
> 商品：WaterproofSpray = 50
> 機制：buy
> tier：1

### greeting
- "防水噴霧，五十塊，自己調配的配方。"
- "效果比便利商店那種好，我試驗過書包、外套。"
- "這幾天雨這麼大，噴了才出門比較安心。"

### onPurchase
- "噴之前先把東西擦乾，這樣附著力比較好。"
- "如果不夠再來找我，我還有存貨。"

### onLeave
- "下次帶朋友來，我不收介紹費啦。"

## 攤位：古早味雞蛋糕
> 攤主：雞蛋糕伯伯
> 商品：EggCake = 10
> 機制：buy
> tier：2

### greeting
- "雞蛋糕，剛出爐，十塊錢四個。"
- "吃了暖胃，雨天走路也有力氣。"
- "以前在校門口擺，現在搬進來躲雨。"
- "小朋友，你是哪個系的？讀書辛苦了。"

### onPurchase
- "趁熱吃，外皮最脆就是現在。"
- "裡頭的蛋香才是重點，感受一下。"

### onLeave
- "讀書讀累了記得吃東西，慢走喔。"

## 攤位：茶藝社花茶攤
> 攤主：茶藝社社員
> 商品：FlowerTea = 15
> 機制：buy
> tier：2

### greeting
- "花茶，熱的，十五塊一杯。"
- "今天是桂花烏龍，自己社上烘的。"
- "喝了腳步會輕一點——真的，不是廣告詞。"

### onPurchase
- "慢慢喝，別燙到。"
- "加社的表單我可以順便給你，不強迫。"

### onLeave
- "下次記得來找我們，社費很便宜的。"

## 攤位：三色章魚燒
> 攤主：章魚燒攤主
> 商品：Takoyaki = 20
> 機制：buy
> tier：2

### greeting
- "章魚燒，二十塊六顆，三種口味。"
- "原味、辣醬、起司，要哪種？"
- "雨天吃熱食才對味，吃完身體都暖了。"

### onPurchase
- "等一下，剛起鍋燙，小心手。"
- "對了，聽說圖書館那邊最近很熱鬧，"
- "很多人搶位子準備期中——你有規劃嗎？"

### onLeave
- "別光顧著吃，書還是要讀的啦。"

## 攤位：學生會募款箱
> 攤主：學生會幹部
> 商品：Donation = 10
> 機制：donate
> tier：3
> karma：+1
> stock：5

### greeting
- "學生會募款，幫弱勢同學繳活動費。"
- "十塊一次，上限五次，自己決定要不要。"
- "不捐也沒關係，聊一下也歡迎。"

### onDonate
- "謝謝你，每一份都有記錄的。"
- "你的善意會傳到需要的人那裡。"

### onChat（未捐款）
- "沒關係，知道有這件事就好。"
- "有需要幫忙的話也可以來找我們。"

### onLeave
- "加油，期中考別太緊繃。"

## 攤位：畢業生二手書攤
> 攤主：畢業學姊
> 機制：sell
> tier：3

### greeting
- "二手書、舊講義、不要的雜物都收。"
- "價格看狀況，一般物資三十到六十塊。"
- "快畢業了，能少帶走一樣是一樣。"

### onPurchase（陷阱傘殘骸）
- "這把傘骨架怎麼這樣……算了，我拆材料用。"
- "給你二十，這傘留著也是麻煩。"

### onPurchase（其他物資）
- "品相還可以，我給你四十。"
- "這種東西留著佔位子，換現金比較實在。"

### onLeave
- "有多的再拿來，我這幾天都在。"

## 攤位：套圈圈遊戲攤
> 攤主：系學會幹事
> 機制：game
> tier：3

### greeting
- "套圈圈，五塊一次，套中拿三十塊。"
- "規則簡單，靠手感，不靠運氣——或許吧。"
- "試試看？反正五塊而已。"

### onPurchase（中獎）
- "哇，真的套進去了，恭喜！"
- "運氣不錯，拿好，三十塊。"

### onPurchase（未中獎）
- "差一點，角度問題，下次調一下力道。"
- "再試一次？五塊，機率三分之一。"

### onLeave
- "沒關係，手氣留著下次用。"

## 攤位：新生招生宣傳攤
> 攤主：招生小組學長姐
> 機制：flavor
> tier：4

### greeting
- "學弟妹好！我們在辦系所說明，來了解一下嗎？"
- "有小點心，免費試吃，不用加社不用報名。"
- "就算只是躲雨也歡迎，傘架在那邊。"

### onAccept
- "好，這個是我們自製的小手冊，帶回去翻翻。"
- "對了最近圖書館那棟三樓在整修，繞道注意一下。"
- "多認識校園之後，很多地方你會發現都有故事的。"

### onLeave
- "有問題隨時來找我們，加油！"

---

## 南端離開觸發區（出 Interlude 的觸發點）

- 位置: 四維道南端，跨越走道全寬（`x ∈ [150, 1950]`、`y ∈ [1900, 2048]`）
- 角色: 靜默 trigger zone——**沒有** NPC、**沒有** 對話選單、**沒有** 互動按鍵
- 機制: 玩家中心點走進該區即同幀 set `Flag_LeaveInterlude = true`；`ChapterGate` 在下一輪 tick 消費此 flag 並 `Transition` 到 `Chapter2_Midterms`
- 對應實作:
  - `include/InterludeExit.h:22-35`（`kInterludeEntry`、`kInterludeExit*` 常數、`InInterludeExitZone()` 純幾何判定）
  - `src/GameController.cpp:511`（每 tick 偵測 player 中心點，arm flag）
- 設計變更紀錄: 原 GDD 草案的「公告板 NPC + 對話選項『離開』」於實作期改為資料驅動的觸發區（`InterludeExit.h` 註記 "F.1-board=C"），原因：
  - 公告板不需要人物互動敘事，純導引功能改為走入即離開更直觀
  - 省一個 `## NPC：` content section、省一張 sprite、省一輪對話腳本
  - 與「逛完往南離開」`QuestObjective`（`include/QuestObjective.h:29-30`）的引導文字直接對齊
- 玩家提示: HUD 上方目標條顯示「目標：在羅馬廣場市集向攤販按 E 採買，逛完往南（校門口方向）離開」，足以引導玩家走到南端離開區

---

## 狀態切換 / MapManager 行為

```cpp
void MapManager::ApplyStateOverlay(SemesterState s) {
    switch (s) {
        case Interlude_Market:
            spawnStalls(stallConfigs_);   // 10 個 Vendor instance
            setRainParticleStrength(0.1f);
            setRainMeterAccumulation(0.0f);
            spawnNoticeBoard();
            break;
        case Chapter2_Midterms:
            despawnStalls();
            setRainParticleStrength(1.0f);
            setRainMeterAccumulation(1.0f);
            // ...
    }
}
```

> Observer 模式應用點: `SemesterStateMachine` 是 Subject，`MapManager` 與 `UIManager` 是 Observer。狀態切換時自動推播。

---

## Karma 連動（×0.6 累積型刻度）

| 行為 | Δkarma | Δmoney |
|---|---|---|
| 募款攤每次捐款 | +1 | -10 |
| 募款 5 次封頂 | +5 | -50 |
| 套圈圈中獎時對攤主說「謝謝」 | +0 | +25 |
| 套圈圈未中卻強行抱怨 | -1 | +0 |
| 跟招生攤位學姐多聊一段 | +1 | +0 |

---

## 待覆核 / 待後續決定的項目

1. **賺錢上限**: 目前三條管道理論上可累積到 ~250 元（起始 100 + NPC 打賞 50 + Pickup 50 + 二手書 60）。是否要設「金錢上限 999 防超量」？
2. **市集是否提供存檔點**: GDD 沒寫；目前無存檔系統。若需要，可在 Interlude 進入時自動寫入 `resources/save/save.json`，或於南端離開觸發區同步寫入。
3. **章魚燒攤的「下章劇情提示」具體要寫什麼**: 等 Ch2 腳本起稿時回填。
4. ~~**公告板 NPC 視覺**~~：已關閉——公告板 NPC 整個取消，改為靜默南端觸發區（見上方「南端離開觸發區」節）。
5. **Vendor sprite**: 10 個攤主沒有獨立全身像時，可用「同一 vendor 站立 sprite + 不同帽子/圍裙顏色覆蓋」風格化處理，省一輪生圖預算。
6. **離開觸發區視覺指示**: 目前完全靜默；考慮在 `View` 加南端地面標記（黃線/路牌 sprite）或進入觸發區前一格時 `ShowMessage("準備離開市集...")`——見 cycle9-ux-diagnosis §5 H3 候選工作項。

---

## 與 Ch1 / Ch2 / Ending C 的銜接

- **Ch1 的 ProfessorTrap 路線**: 進 Interlude 時若 `Flag_HasProfessorTrap == true`，由二手書攤（畢業學姊）`onPurchase（陷阱傘殘骸）` 對話接手，「拆材料用」回收 +20 元——此 mechanic 目前未實作（見 audit "Ch1 ProfessorTrap 回收 +20" 條目）；先當設計意圖記錄。
- **Ch1 的 CursedUmbrella 路線**: 攤主對話會出現「看你那把傘怪怪的」flavor，karma 偏低的玩家會被部分攤主拒絕交易
- **Ending C 的醜綠傘**: **不在這裡賣**——保持 GDD「集英樓便利商店買綠傘」的結局觸發點不變。市集只是 Ch1 福利社阿姨之外的「物資補給站」，與綠傘無直接關係
