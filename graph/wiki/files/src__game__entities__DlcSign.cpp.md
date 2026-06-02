---
id: file:src/game/entities/DlcSign.cpp
type: source
path: src/game/entities/DlcSign.cpp
domain: game
bucket: entities
loc: 57
classes: []
sources: ["src/game/entities/DlcSign.cpp"]
---
# `DlcSign.cpp`

> **一句定位**：DLC 預告告示牌的實作——以矩形組合繪製粗體「？」字符，互動時可重複發布預告訊息。

## 職責

`DlcSign` 繼承自 `WithRoles<DlcSign, GameObject>`，是世界中固定擺放的裝飾性互動物件，無任何玩法副作用。

**建構**：碰撞盒 28×28（略大於 16×16 拾取物，確保 E 探測距離膨脹後可輕鬆觸及），`message_` 預燒 `"DLC開發中\n敬請期待"` 兩行字串（`\n` 被 `MessageView::WrapCjk` 採納，使兩行置中對齊）。

**Render**：以 `DrawRect` 組合繪製：先鋪一塊深色半透明底板（RGBA 30/32/40/220），再以比例座標（`rc` lambda，`fx/fy/fw/fh` 相對於碰撞盒）繪出金色（`Colors::Gold`）的粗體「？」筆畫（上橫 + 右側下降 + 彎入弧 + 垂直主幹 + 底部一點，共 5 個矩形）。遵守 `Item`/`GameObject` 架構規則：純 `DrawRect`，不含 raylib、不呼叫 `DrawText`。

**Interact**：可重複閱讀，每次均發布 `ShowMessage` 帶入 `message_`，但「不」設 `isActive_=false`——告示牌留在世界、可再次閱讀。`initiator` 參數未使用（標記為 `/*initiator*/`）。

## 關鍵內容（類別 / 函式 / 資料）

- `DlcSign(Vec2 position)`：建構，28×28 碰撞盒，`message_` 預燒兩行預告。
- `void Render(IRenderer&) const`：底板 + 5 個矩形組成「？」字符，比例座標設計使其隨碰撞盒縮放。
- `void Interact(Player*)`：發布 `ShowMessage`，可重複觸發，不使 `isActive_=false`。
- `message_`（string）：預燒的兩行預告文字。
- `rc` lambda：比例座標輔助函式（`fx * width + x`）。

## 相依與在架構中的位置

- **#include（往外）**：`DlcSign.h`、`EventBus.h` / `EventSink.h`（發布 ShowMessage）、`Color.h` / `Rect.h` / `IRenderer.h`（渲染）。
- **被誰使用（往內）**：—（葉節點；由世界初始化或 Factory 擺放）。
- **繼承 / 實作 / 體現**：繼承 `WithRoles<DlcSign, GameObject>`；實作 `IDrawable`（`Render`）、`IInteractable`（`Interact`）角色。
- **每幀管線 / MVC 角色**：Model 層被動實體；不影響 Survival / Movement / Collision 管線，僅在 E 互動時觸發。

## OO 概念與設計重點

[CRTP mixin `WithRoles`](../concepts/oo-crtp.md) 使 `DlcSign` 在編譯期掛載 `IDrawable` 和 `IInteractable` 而無 `dynamic_cast` 開銷。[ISP 角色介面](../concepts/oo-isp-roles.md) 確保告示牌只引入它確實實作的角色（無 `IMortal`，不能被移除）。[Observer](../concepts/pat-observer.md) 體現於 `ShowMessage` 事件——`DlcSign` 對 HUD 訂閱者一無所知。比例座標 `rc` lambda 使圖形與碰撞盒尺寸脫耦，是資料驅動繪製的輕量形式。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/DlcSign.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/DlcSign.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [ISP 角色介面](../concepts/oo-isp-roles.md) · [Observer](../concepts/pat-observer.md)
