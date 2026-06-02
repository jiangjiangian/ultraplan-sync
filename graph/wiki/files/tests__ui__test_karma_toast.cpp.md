---
id: "file:tests/ui/test_karma_toast.cpp"
type: test
path: tests/ui/test_karma_toast.cpp
domain: tests
bucket: ui
loc: 147
classes: [KarmaCapture]
sources: ["tests/ui/test_karma_toast.cpp"]
---
# `test_karma_toast.cpp`

> **一句定位**：驗證業力變動的三層事件管線：`AddKarma` 發佈帶正負號差值的 `KarmaChanged`、`WireKarmaToastSubscriber` 轉成 HUD 提示，以及詛咒傘拾取不重複發佈。

## 職責

本測試固定業力 HUD 橫幅功能的三個不變量：

1. **`AddKarma` 格式**：每次呼叫 `AddKarma(n)` 發佈恰好一個 `KarmaChanged` 事件，`e.text` 格式為 `%+d`（正差值帶明確 `+`，如 `"+5"`、`"-3"`）；`decreaseKarma(10)` 轉發到 `AddKarma(-10)`，也只發一個事件，固定「不重複發佈」不變量。

2. **端到端接線（`WireKarmaToastSubscriber`）**：正式接線 `AddKarma → KarmaChanged → WireKarmaToastSubscriber → ShowMessage → WireHudMessageSubscriber → World.HudMessage()`。驗證正差值產生含「業力」前綴與「+N」的 HUD 訊息；負差值含「業力」與「-N」；`AddKarma(0)` 不產生任何 HUD 提示（防止無意義的「業力 +0」佔用橫幅）。

3. **詛咒傘拾取語意修正**：`CursedUmbrella::BeClaimed` 不再於拾取時調整業力（業力代價已移到 `ApplyCursedTaintDecay`，在每個章節邊界觸發）。測試驗證拾取時 `KarmaChanged` 為 0 個、`GetKarma()` 不變；後續 `ApplyCursedTaintDecay()` 發佈一次 `"-5"`，業力從 50 降到 45。

`KarmaCapture` 用 `ScopedSubscribe` RAII 訂閱 `KarmaChanged` 事件，讓訂閱自動在解構時取消。

## 關鍵內容（類別 / 函式 / 資料）

- `KarmaCapture`：包含 `deltas`（string vector）和 RAII `Subscription`，透過 `EventBus::ScopedSubscribe` 擷取 `KarmaChanged`。
- `CaptureKarma()`：建立並返回 `KarmaCapture` 的工廠函式。
- `Player::AddKarma(int)` — 被測：格式化差值並發佈 `KarmaChanged`。
- `Player::decreaseKarma(int)` — 被測：確認只透過 `AddKarma` 轉發，不重複。
- `CursedUmbrella::BeClaimed(Player*)` — 被測：確認不再影響業力。
- `Player::ApplyCursedTaintDecay()` — 被測：觸發 `-5` 業力衰減並發佈。
- `WireKarmaToastSubscriber(EventBus&)` / `WireHudMessageSubscriber(EventBus&, World&)` — 被測接線函式。

## 相依與在架構中的位置

- **#include（往外）**：`game/entities/CursedUmbrella.h`、`engine/events/EventBus.h`、`game/controller/EventWiring.h`、`game/entities/Player.h`、`game/world/World.h`、`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model + Controller 層測試，驗證 EventBus [Observer 模式](../concepts/pat-observer.md)從 Player 到 HUD 的端到端接線。

## OO 概念與設計重點

`ScopedSubscribe` 返回 RAII `Subscription`（[RAII](../concepts/oo-raii.md)），讓測試訂閱自動隨 `KarmaCapture` 析構取消。測試本身體現了 [Observer 模式](../concepts/pat-observer.md)的可測試性——直接訂閱事件，無需 mock 整個 World。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_karma_toast.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_karma_toast.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
