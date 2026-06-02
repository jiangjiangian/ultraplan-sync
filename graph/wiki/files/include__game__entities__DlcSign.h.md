---
id: file:include/game/entities/DlcSign.h
type: header
path: include/game/entities/DlcSign.h
domain: game
bucket: entities
loc: 55
classes: [DlcSign]
sources: ["include/game/entities/DlcSign.h"]
---
# `DlcSign.h`

> **一句定位**：立於風雩走廊的可重複閱讀彩蛋告示牌，E 互動發 DLC 預告訊息、永不失效、無任何玩法效果。

## 職責

`DlcSign` 是 Chapter4_Finals 的裝飾性點綴物件。玩家按 E 互動時發布 `ShowMessage`（「DLC開發中\n敬請期待」），但「不」設 `isActive_ = false`，故告示牌持續存在、可重複閱讀。它沒有旗標、無業力、無金錢、無任務鉤子，是純粹的非功能性彩蛋。

設計邊界：告示牌刻意「不」繼承 `Item`——`Item` 帶有 `OnPickup` 語意與 `isPickable_` 旗標，E 互動掃描預期 Item 為一次性收取。告示牌是可對話的固定佈景，故直接落在 `GameObject` 之下，搭配 CRTP `WithRoles<DlcSign, GameObject>`。

ISP 設計：`DlcSign` 扮演 `IDrawable`（繪出大型「？」字符，只用矩形圖元，不呼叫 DrawText）與 `IInteractable`（`Interact` 發出預告），但「不」扮演 `IUpdatable`（告示牌不需逐幀更新）。控制器的 E 互動掃描因 `NpcId()` 為空、`IsVendor()` 為 false，導向通用的 `AsInteractable()->Interact()` 分支，不走 NPC 對話或攤販購買路徑。

## 關鍵內容（類別 / 函式 / 資料）

- **`DlcSign(position)`**：建構子，定義於 `.cpp`，世界座標決定告示牌位置。
- **`void Render(IRenderer&) const override`**：用矩形圖元繪出大型「？」字符，符合 Item 不呼叫 DrawText/DrawTexture 的架構紅線。
- **`void Interact(Player* initiator) override`**：發布 `ShowMessage` 預告文字；「不」設 `isActive_ = false`；initiator 未被使用（不觸碰 Player 狀態）。
- **`std::string message_`**（private）：預告訊息文字。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/core/GameObject.h`（基底提供位置、碰撞盒、`isActive_`）、`include/engine/math/Vec2.h`（建構子座標參數）；`<string>`（`message_`）。
- **被誰使用（往內）**：`src/game/entities/DlcSign.cpp`（實作）、`src/game/world/World.cpp`（持有）、`src/game/world/WorldSpawn.cpp`（生成）、`tests/entities/test_dlc_sign.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<DlcSign, GameObject>`、實作 `IDrawable`、`IInteractable`（來自 `Roles.h`）。
- **每幀管線 / MVC 角色**：Model 層，World 持有此物件於 `Objects()` 中。View 的繪製迴圈呼叫其 `Render`；Controller 的 E 互動掃描呼叫其 `Interact`。因永不設 `isActive_=false`，Sweep 不移除它。

## OO 概念與設計重點

`DlcSign` 是 [ISP（介面隔離原則）](../concepts/oo-isp-roles.md) 的精確體現：只扮演實際需要的角色（`IDrawable` + `IInteractable`），捨棄 `IUpdatable` 的空殼覆寫。它不繼承 `Item` 的決定則遵守了 Liskov 替換原則——若它是 `Item`，拾取語意會使 E 互動掃描將其當成一次性收取物，與設計意圖衝突。

`WithRoles<DlcSign, GameObject>` 的 [CRTP mixin](../concepts/oo-crtp.md) 設計讓控制器可以在無 `dynamic_cast` 的情況下，以 `AsInteractable()` 取得其 `IInteractable` 介面指標。整體維持 Model 乾淨（不呼叫 raylib 繪製函式、不讀輸入）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/DlcSign.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/DlcSign.h) · [← 全檔索引](../files-index.md) · 相關概念：[ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
