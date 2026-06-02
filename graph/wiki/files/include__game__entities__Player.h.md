---
id: file:include/game/entities/Player.h
type: header
path: include/game/entities/Player.h
domain: game
bucket: entities
loc: 303
classes: [Player]
sources: ["include/game/entities/Player.h"]
---
# `Player.h`

> **一句定位**：玩家角色的完整宣告，作為整個 Model 的核心狀態持有者：業力、金錢、雨量、持傘種類、旗標、消耗品背包、生命值與動畫狀態。

## 職責

`Player` 是遊戲 Model 中狀態最複雜的類別，繼承自 `Character`（可移動）並扮演三個角色：`IUpdatable`（每幀輸入 + 動畫）、`IDrawable`（Pipoya sprite + 人設色調）、`IMortal`（生命值骨架）。刻意捨棄 `IInteractable`（玩家不回應其他玩家），E 互動掃描以 `ForEachActiveExcept` 略過他。

**雨量系統**：`ApplyRain`（戶外無傘，+5/sec，滿格傳送回正門）、`ApplyRainSheltered`（戶外持傘，+1.5/sec 緩慢累積）、`DrainRain`（建築內，-10/sec 完全回復）、`DrainRainBy`（消耗品減免，固定量）。四條路徑讓 `GameController` 依情境精確控制，不作一刀切設計。

**業力系統**：`AddKarma(delta)` 夾制於 [-100,100] 並發出 `KarmaChanged` 事件；`decreaseKarma` 為向下相容包裝。詛咒污點（`cursedTaint_`）透過 `IncCursedTaint()` + `ApplyCursedTaintDecay()` 實現每章過場的滑動懲罰。

**持傘系統**：`HeldUmbrella` 列舉（None/True/Cursed/Ugly/Victim/Fragile/ProfessorTrap）追蹤「即時」握傘種類，與持久結局旗標（`Flag_HasTrueUmbrella` 等）分離。`SetHasUmbrella(bool)` 切換遮蔽、`SetHeldUmbrella(kind)` 設定種類（非 None 同時啟用遮蔽）。

**背包系統**：`consumables_`（`unordered_map<string, int>`）純計數消耗品；`AddConsumable`、`ConsumableCount`、`ConsumeOne`、`ClearConsumables`。金錢 `AddMoney` 夾制於 `kMoneySoftCap = 300`（循環經濟防囤積）。旗標表 `flags_`（`unordered_map<string, bool>`）的 `SetFlag`／`ClearFlag`／`HasFlag`。

所有變更器皆回傳 `*this` 支援鏈式呼叫。

## 關鍵內容（類別 / 函式 / 資料）

- **`enum class HeldUmbrella { None, True, Cursed, Ugly, Victim, Fragile, ProfessorTrap }`**：即時握傘種類。
- **`void Update(float deltaTime) override`**：每幀輸入 + 行走動畫推進。
- **`void Render(IRenderer&) const override`**：依 `lastFacing_` / `animStep_` 切 Pipoya 格，以 `tint_` 調色繪製。
- **`void HandleInput(float deltaTime)`**：讀方向鍵並呼叫 `Move()`。
- **`static constexpr int kMaxHp = 100`**：生命值上限（IMortal 骨架，目前無敵人傷害）。
- **`Player& AddKarma(int delta)`**：夾制並發 `KarmaChanged`。
- **`Player& IncCursedTaint() noexcept`**：遞增詛咒污點計數。
- **`Player& ApplyCursedTaintDecay()`**：扣 5×cursedTaint_ 業力（每章過場呼叫）。
- **`Player& SetHasUmbrella(bool v) noexcept`**：切換遮蔽；false 同時清 `heldUmbrella_`。
- **`Player& SetHeldUmbrella(HeldUmbrella kind) noexcept`**：設種類並同步遮蔽旗標。
- **`HeldUmbrella HeldUmbrellaKind() const noexcept`**：查即時握傘。
- **`static constexpr int kMoneySoftCap = 300`**：金錢軟上限。
- **`Player& AddMoney(int amount) noexcept`**：加錢夾制軟上限。
- **`bool DeductMoney(int amount) noexcept`**：扣款，不足回傳 false（無副作用）。
- **`Player& SetFlag / ClearFlag / bool HasFlag`**：任務旗標操作。
- **`Player& AddConsumable(const string& itemId)`**：背包計數加一。
- **`int ConsumableCount(const string& itemId) const`**：查持有數。
- **`bool ConsumeOne(const string& itemId)`**：消耗一個；未持有為 false。
- **`Player& ClearConsumables() noexcept`**：清空背包（換章時呼叫）。
- **`const unordered_map<string,int>& Consumables() const noexcept`**：背包唯讀視圖（UI 用）。
- **`void LoadSprite(path)` / `Player& SetTint(Color)`**：sprite 載入與人設色調。
- **`Player& ApplyRain / DrainRain / DrainRainBy / ApplyRainSheltered`**：雨量四條路徑。
- 私有：`rainMeter_`、`karma_`、`hasUmbrella_`、`heldUmbrella_`、`money_`、`cursedTaint_{0}`、`hp_{kMaxHp}`、`flags_`、`consumables_`、`sprite_`、`tint_`、`lastFacing_`、`animTimer_`、`animStep_`。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/Character.h`（可移動基底）、`include/engine/math/Color.h`（tint）、`include/engine/render/Texture.h`（sprite）、`include/engine/math/Vec2.h`（lastFacing_）。
- **被誰使用（往內）**：幾乎整個代碼庫（超過 50 個檔案），從 World、Controller、各 Entity、Quest、UI、測試一路引用。Player.h 是整個 Model 的中樞標頭。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<Player, Character>`，實作 `IUpdatable`、`IDrawable`、`IMortal`（`Roles.h`）。`Player` 為 `final`。
- **每幀管線 / MVC 角色**：Model 核心。Survival System 呼叫雨量四條路徑；Movement System 呼叫 `HandleInput` + `Move`；Collision System 推回座標；View 呼叫 `Render`；所有 Quest/Dialog/Vendor 邏輯讀寫 Player 狀態。

## OO 概念與設計重點

`Player` 是「Model = World 的核心狀態持有者」的最佳例證，體現 [MVC](../concepts/arch-mvc.md) 架構：所有遊戲狀態（業力、金錢、傘、旗標、雨量）集中於此，View 只讀 `const Player&`，Controller 透過方法改寫狀態，無 raylib 符號滲入 Model。

`HeldUmbrella` 列舉與結局旗標分離是精妙的設計：結局旗標（`Flag_HasTrueUmbrella`）一旦設立就不清除，但「即時握傘」在換章時可重置（`SetHasUmbrella(false)` 清 `heldUmbrella_`）。這解決了「Ch4 傘再度失蹤」後，舊旗標不該讓背包仍顯示持傘的問題。鏈式 `*this` 回傳設計讓測試碼清晰表達複合狀態設置。`cursedTaint_` 永不清除（道德污點永久），`hp_` 不被序列化（不影響確定性重播）——兩個精確的不變式設計。

[ISP / Roles](../concepts/oo-isp-roles.md) 體現在捨棄 `IInteractable`（玩家不被其他玩家互動），以及 [CRTP](../concepts/oo-crtp.md) 的 `WithRoles<Player, Character>` 使 `Player` 為 final 時仍能做零成本角色轉型。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/Player.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Player.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
