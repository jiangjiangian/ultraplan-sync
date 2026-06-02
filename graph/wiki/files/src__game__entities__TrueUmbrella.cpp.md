---
id: file:src/game/entities/TrueUmbrella.cpp
type: source
path: src/game/entities/TrueUmbrella.cpp
domain: game
bucket: entities
loc: 26
classes: []
sources: ["src/game/entities/TrueUmbrella.cpp"]
---
# `TrueUmbrella.cpp`

> **一句定位**：真傘的認領覆寫——設下結局 A 精確旗標，並以嚴格事件順序確保章節通關提示正確顯示在 HUD 前景。

## 職責

`TrueUmbrella::BeClaimed(Player*)` 是傘家族中最重要的版本，覆蓋 Ending A 的核心前提。

冪等守衛後，呼叫 `player->SetHeldUmbrella(HeldUmbrella::True)` 記錄背包欄位並設 `hasUmbrella_=true`。設下 `kFlagHasTrueUmbrella`——此旗標「只」由 `TrueUmbrella::BeClaimed` 設立，Chapter4 進場時被清除（傘再次失蹤），故在 Ch4 它精確表示「已認領 Ch4 的 TrueUmbrella」，是 Ending A 的最終條件，不會被 Fragile / ProfTrap 的早期認領污染。

**事件順序（關鍵正確性）**：`ShowMessage`（「你撿到了 TrueUmbrella，雨停了。」）**先**發布，`UmbrellaClaimed("TrueUmbrella")` **後**發布。兩則訊息都落在同一個單槽 HUD 頻道，後發布者會覆蓋前者。因此 `UmbrellaClaimed` 的訂閱者（發出章節通關的 ShowMessage）成為 HUD 可見的橫幅，而拾取字幕僅短暫顯示後被覆蓋。對調此兩行將重現「傘字串遮蔽章節提示」的回歸缺陷，由章節過場測試固定。

## 關鍵內容（類別 / 函式 / 資料）

- `TrueUmbrella::BeClaimed(Player*)`：冪等守衛 + `SetHeldUmbrella(True)` + `SetFlag(kFlagHasTrueUmbrella)` + 失活 + ShowMessage 先 + UmbrellaClaimed 後。
- `kFlagHasTrueUmbrella`（引自 `Flags.h`）：Ending A 精確旗標，Ch4 進場清除。
- 事件發布順序：`ShowMessage → UmbrellaClaimed`，順序不可顛倒。

## 相依與在架構中的位置

- **#include（往外）**：`TrueUmbrella.h`、`Player.h`、`EventBus.h` / `EventSink.h`、`Flags.h`。
- **被誰使用（往內）**：—（葉節點；`TransparentUmbrella::BeClaimed` 的具體終端）。
- **繼承 / 實作 / 體現**：`TrueUmbrella` → `TransparentUmbrella` → `Item`；覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：認領在 Collision / 互動階段執行，觸發 UmbrellaClaimed 進而驅動章節通關。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md) 的最高價值節點：`BeClaimed` 的事件發布順序是刻意設計的正確性保證，且由回歸測試固定。這說明「鉤子」方法不只是簡單的填空，而是要求子類別對整個系統的副作用順序負責。[Observer](../concepts/pat-observer.md)：`UmbrellaClaimed` 接線到章節通關 ShowMessage；這是雙重 ShowMessage 覆蓋問題的根因與解法所在。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/TrueUmbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/TrueUmbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [Observer](../concepts/pat-observer.md)
