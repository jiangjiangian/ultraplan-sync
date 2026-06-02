---
id: file:tests/entities/test_quest_pickup_render.cpp
type: test
path: tests/entities/test_quest_pickup_render.cpp
domain: tests
bucket: entities
loc: 80
classes: [CountingRenderer, RectCall]
sources: ["tests/entities/test_quest_pickup_render.cpp"]
---
# `test_quest_pickup_render.cpp`

> **一句定位**：透過間諜 `IRenderer` 驗證 `QuestFlagPickup::Render` 的型別感知算繪——紙張類任務物件畫白色，苦主的透明傘畫成藍色真傘輪廓，且兩者均不呼叫 DrawSprite 或 DrawText。

## 職責

此測試在無 GL context 的環境下驗證 `QuestFlagPickup` 的算繪行為。其核心觀察是：任務拾取物的外觀依旗標名稱而異——申請書類（`kFlagFoundForm`）畫出白色紙張（不再是舊版的 Yellow），而苦主的透明傘（`kFlagHasVictimUmbrella`）則使用 `UmbrellaGlyph` 提供的共用藍色雨傘圖形，確保它在地圖上不會被誤認為普通的黃色方塊。

兩個 TEST_CASE 都確立「任務物件只畫矩形，絕不畫 sprite 或文字」的架構規則。這是 MVC 紀律的一部分：View 層不知道 `QuestFlagPickup` 的型別，但物件本身的 `Render` 依自身資料（旗標名稱）選擇繪圖方式。

## 關鍵內容（類別 / 函式 / 資料）

- `CountingRenderer`：實作 `IRenderer` 的間諜，繼承並覆寫 `DrawRect/DrawSprite/DrawText`；用 `std::vector<RectCall> rects` 記錄所有矩形繪製呼叫，以及整數計數器 `sprites`、`texts`。
- `RectCall`：儲存單次 `DrawRect` 呼叫的矩形與顏色，用於事後斷言。
- `HasColor()`：在 `CountingRenderer.rects` 中搜尋指定顏色是否出現。
- `TEST_CASE("QuestFlagPickup::Render：紙張類任務物件畫白色紙張")`：以 `kFlagFoundForm` 建構，斷言至少一個矩形是白色（`Colors::White`），且不出現 Yellow；sprites==0、texts==0。
- `TEST_CASE("QuestFlagPickup::Render：苦主的透明傘畫成藍色雨傘圖形")`：以 `kFlagHasVictimUmbrella` 建構，斷言矩形數量 ≥ 3（完整雨傘輪廓），且顏色等於 `UmbrellaLookColor(UmbrellaLook::TrueBlue)`，不出現 Yellow。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/IRenderer.h`（間諜基底），`game/entities/QuestFlagPickup.h`（受測），`game/entities/Player.h`，`game/gfx/UmbrellaGlyph.h`（共用外觀色），`game/quest/Chapter1Quest.h`，`game/quest/Flags.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`CountingRenderer` 繼承 `IRenderer`（`include/engine/render/IRenderer.h`）。
- **每幀管線 / MVC 角色**：—（測試 View 行為，不在執行期管線內）

## OO 概念與設計重點

`CountingRenderer` 是典型的「測試間諜」模式：在無視窗環境下注入 `IRenderer` 介面，讓 Render() 的多型呼叫路徑可觀察。這體現了 [DIP Renderer 架構](../concepts/arch-dip-renderer.md)——所有繪圖指令都通過 `IRenderer` 抽象，測試因此得以在不依賴 raylib 的情況下驗證算繪邏輯。

`QuestFlagPickup` 的型別感知算繪（依旗標名稱分支）體現了「行為由資料驅動」的設計：物件自帶足夠的資訊（旗標名稱），無需 `dynamic_cast` 或外部分派，在 `Render()` 內部決定外觀。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_quest_pickup_render.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_quest_pickup_render.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
