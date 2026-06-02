---
id: file:src/game/controller/screens/InventoryScreen.cpp
type: source
path: src/game/controller/screens/InventoryScreen.cpp
domain: game
bucket: controller
loc: 73
classes: []
sources: ["src/game/controller/screens/InventoryScreen.cpp"]
---
# `InventoryScreen.cpp`

> **一句定位**：背包畫面的輸入處理器——Tab 開關、方向鍵移游標/翻頁、E/Enter 使用消耗品，開啟期間凍結一般模擬。

## 職責

`InventoryScreen.cpp` 實作自由函式 `HandleInventory(EventBus& bus, World& world)`，是 Controller 層每幀插入結局/暫停處理之後、移動互動之前的背包攔截器。

**Tab 開關**：每幀先以 `Input::IsPressed(Key::Tab)` 翻轉 `world.InventoryOpen()`，使 Tab 成為唯一的開關鍵。背包開啟後函式在尾端回傳 `true`，凍結後續的移動與互動；關閉後回傳 `false`，讓輸入正常穿透。

**游標與翻頁**：每幀從 `world.GetPlayer()` 取玩家，呼叫 `BuildInventoryRows(*player)` 重建當前列表快照，以方向鍵 Up/Down 環形捲動游標，Left/Right 一次跳 `kInventoryRowsPerPage` 列作為快速翻頁（夾限不繞回），再以 `world.SetInventoryCursor(cur)` 寫回。游標若超出列表範圍自動夾至合法區間。

**使用消耗品**：按 E 或 Enter 時，若當前選取列的 `usable` 位元為 true、`IsUsableConsumable(sel.itemId)` 成立、且玩家還有庫存，則先呼叫 `ApplyConsumableEffect(bus, *player, sel.itemId)` 套用效果，再呼叫 `player->ConsumeOne(sel.itemId)` 扣減數量。刻意先效果後扣減，使語意一目了然（使用→沒了）。

## 關鍵內容（類別 / 函式 / 資料）

- `HandleInventory(EventBus& bus, World& world) -> bool`：唯一入口；`true` 表示背包開啟中，輸入已消耗。
- `BuildInventoryRows(Player&) -> std::vector<InventoryRow>`：每幀從玩家資料重建列表快照。
- `IsUsableConsumable(itemId)`：判斷道具可直接使用。
- `ApplyConsumableEffect(EventBus&, Player&, itemId)`：套用消耗效果並發布 ShowMessage。
- `player->ConsumeOne(itemId)`：扣減一份庫存。
- `nccu::kInventoryRowsPerPage`：Left/Right 跳頁步長常數。

## 相依與在架構中的位置

- **#include（往外）**：`InventoryScreen.h`、`World.h`、`Player.h`（取玩家、游標、UseConsumable）、`ItemCatalog.h`（`IsUsableConsumable` / `ApplyConsumableEffect`）、`InventoryPaging.h`（`kInventoryRowsPerPage` 翻頁步長）、`Input.h` / `Key.h`。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 的每幀輸入分派呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：Controller 層的輸入分派第二道凍結器，位於結局/暫停之後、一般移動之前。

## OO 概念與設計重點

本檔屬 [MVC](../concepts/arch-mvc.md) 的 **Controller** 層。設計上刻意與 View（`BuildInventoryRows`）分離：每幀重建快照而非快取，使「使用後數量歸零→列消失」這個語意由資料驅動、無需手動通知。`HandleInventory` 回傳 `bool` 參與呼叫端的 Chain-of-Responsibility 凍結鏈，與 `HandleEndingMenu` / `HandlePauseMenu` 保持一致的介面契約。

## 連結

[🕸 圖譜節點](../../index.html#node=file:src/game/controller/screens/InventoryScreen.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/InventoryScreen.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
