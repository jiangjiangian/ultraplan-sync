---
id: "file:tests/ui/test_two_hud_channels.cpp"
type: test
path: tests/ui/test_two_hud_channels.cpp
domain: tests
bucket: ui
loc: 232
classes: []
sources: ["tests/ui/test_two_hud_channels.cpp"]
---
# `test_two_hud_channels.cpp`

> **一句定位**：驗證雙 HUD 通道（Top / Bottom）的路由隔離、共存、章節→幕間轉場樣式、過期不外洩，以及 `DismissHud` 的單通道/全通道語意。

## 職責

本測試修正了一個可見性問題：章節→幕間轉場時，「章節清關」提示（Top）與「抵達市集」提示（Bottom）幾乎同一幀發佈，舊的單一 HUD 欄位讓後者覆寫前者，前者只可見約 0.02 秒。通道拆分後兩行同時可見。

測試固定六個不變量：

1. **路由隔離**：`SetHudMessage(Top, text)` 只落在 Top；Bottom 維持空（反之亦然）。向後相容：無 slot 的 `SetHudMessage(text)` 等同寫入 Bottom；無 slot 的 `HudMessage()` / `HudAge()` 讀取器也指向 Bottom。

2. **同幀共存**：先 Top 後 Bottom，或先 Bottom 後 Top，最終兩通道都可見（發佈順序不影響結果）。

3. **章節轉場樣式**：`PublishChapterTransitionToast(bus, Interlude_Market)` + Bottom 抵達提示；`TickHud(1/60)` 後兩個通道的訊息與存活時間都正確，均未過期。

4. **過期不外洩**：Top 過期後，往 Bottom 的寫入乾淨落定；`HudMessage(Top)` 仍保留字串（View 淡出動畫需要），但 `HudExpired(Top)` 為 true。

5. **DismissHud 語意**：`DismissHud(Top)` 只讓 Top 過期；`DismissHud(Bottom)` 只讓 Bottom 過期；`DismissHud()`（無參數）兩者皆過期。

6. **章節提示的 slot 契約**：`PublishChapterTransitionToast` 發佈的 `ShowMessage` 事件帶有 `HudSlot::Top`；`Ending_A` 的轉場提示也帶有 Top。

## 關鍵內容（類別 / 函式 / 資料）

- `World::SetHudMessage(HudSlot, text)` / `World::SetHudMessage(text)` — 被測（雙多載）。
- `World::HudMessage(HudSlot)` / `World::HudAge(HudSlot)` / `World::HudExpired(HudSlot)` — 被測（帶 slot 參數）。
- `World::DismissHud(HudSlot)` / `World::DismissHud()` — 被測。
- `World::TickHud(float dt)` — 同時推進兩個通道的存活時間。
- `nccu::PublishChapterTransitionToast(EventBus&, SemesterState)` — 被測：發佈帶 `HudSlot::Top` 的轉場提示。
- `nccu::kInterludeArrivalHint` — 幕間抵達提示字串常數。
- `nccu::WireHudMessageSubscriber(EventBus&, World&)` — 連接 ShowMessage → SetHudMessage 的接線函式。
- `HudSlot::Top` / `HudSlot::Bottom` — 通道枚舉。

## 相依與在架構中的位置

- **#include（往外）**：`game/state/ChapterToast.h`、`engine/events/EventBus.h`、`game/controller/EventWiring.h`、`engine/events/HudSlot.h`、`ui/MessageView.h`（kHudTtl）、`game/entities/Player.h`、`game/state/SemesterState.h`、`game/state/SemesterStateMachine.h`、`game/world/World.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model + Controller 層測試，驗證 HUD 雙通道設計在 EventBus [Observer](../concepts/pat-observer.md) 接線下的行為。

## OO 概念與設計重點

雙通道設計是對單一欄位的 **ISP** 擴展：按用途（章節轉場 vs 其他提示）分通道，使兩類 HUD 的 TTL 各自獨立計時，不互相覆寫。`ScopedSubscribe` 的 RAII 訂閱確保測試探針自動在 case 邊界清理。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_two_hud_channels.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_two_hud_channels.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
