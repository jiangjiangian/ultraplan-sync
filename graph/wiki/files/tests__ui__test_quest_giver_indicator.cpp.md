---
id: "file:tests/ui/test_quest_giver_indicator.cpp"
type: test
path: tests/ui/test_quest_giver_indicator.cpp
domain: tests
bucket: ui
loc: 145
classes: [Spy, RectCall, TextCall, StubObj]
sources: ["tests/ui/test_quest_giver_indicator.cpp"]
---
# `test_quest_giver_indicator.cpp`

> **一句定位**：驗證任務發派者頭上「!」指示器的繪圖原語、幾何佈局、世界座標跟隨行為，以及 `IsQuestGiver()` 的虛擬分派正確性。

## 職責

本測試修正了一個回歸缺陷：`IsQuestGiver()` 旗標在生成端設定，但 View 端完全忽略它，任何 NPC 的「!」標記都不出現。

`Spy` 記錄所有繪圖原語的完整資訊（矩形含顏色、文字含位置與大小），使幾何斷言不依賴 GL。

四個測試案例：

1. **繪製原語**：`DrawQuestGiverIndicator(spy, hitbox)` 恰好畫 2 個矩形（陰影 + 金色面板）和 1 個「!」文字，0 個精靈。面板顏色為 `{255, 200, 61, 255}`（raylib 金色 #FFC83D）；陰影為深色半透明（`a < 255`）。

2. **佈局浮於頭頂上方**：`LayoutQuestGiverIndicator(hitbox)` 返回的 `panel` 底部嚴格低於 `spriteTopY`（`hitbox.y + height - 32`），且間距 ≥ 10px；面板水平對齊 hitbox 中心；面板大小恰好 16×16。

3. **世界座標跟隨**：hitbox 從 (100,200) 移到 (160,260)，`panel.x`、`panel.y`、`textPos.x`、`textPos.y` 分別各位移 60（同等量）。

4. **`IsQuestGiver()` 虛擬分派**：透過 `GameObject&` 基底參考呼叫，標記為 `isQuestGiver=true` 的 NPC 回傳 true；`isQuestGiver=false` 的 NPC 回傳 false。`StubObj`（空的 `GameObject` 子類別）預設 `IsQuestGiver()` 為 false，確認基底類別預設值正確。

## 關鍵內容（類別 / 函式 / 資料）

- `Spy`（含 `RectCall`、`TextCall` 巢狀結構）：實作 `IRenderer`，完整記錄所有繪圖原語及參數。
- `StubObj`：最小的 `GameObject` 子類別，用於驗證基底預設值。
- `DrawQuestGiverIndicator(IRenderer&, Rect hitbox)` — 被測繪圖函式。
- `LayoutQuestGiverIndicator(Rect hitbox)` — 被測佈局函式，回傳含 `panel`（Rect）與 `textPos`（Vec2）的結構。
- `NPC::IsQuestGiver()` / `GameObject::IsQuestGiver()` — 被測虛擬函式。

## 相依與在架構中的位置

- **#include（往外）**：`ui/QuestGiverIndicator.h`（受測主體）、`game/entities/NPC.h`（NPC 建構與 IsQuestGiver）、`engine/render/IRenderer.h`（Spy 基底）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 實作 `include/engine/render/IRenderer.h`；`StubObj` 繼承 `include/engine/core/GameObject.h`。
- **每幀管線 / MVC 角色**：View 層測試，對應 View 在 CameraScope 內遍歷物件並呼叫 `DrawQuestGiverIndicator` 的路徑。

## OO 概念與設計重點

以 [DIP IRenderer](../concepts/arch-dip-renderer.md) 的 Spy 達成無頭繪圖測試。`IsQuestGiver()` 的虛擬分派測試對應 [OO 多型](../concepts/oo-isp-roles.md)：View 走訪 `GameObject&` 基底，透過虛擬分派取得子類別的行為，無需 `dynamic_cast`。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_quest_giver_indicator.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_quest_giver_indicator.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [ISP Roles](../concepts/oo-isp-roles.md)
