---
id: file:include/engine/events/HudSlot.h
type: header
path: include/engine/events/HudSlot.h
domain: engine
bucket: events
loc: 31
classes: []
sources: ["include/engine/events/HudSlot.h"]
---
# `HudSlot.h`

> **一句定位**：HUD 訊息雙頻道列舉，將章節 / 結局提示（Top）與一般訊息（Bottom）分到兩條獨立通道，解決同幀覆蓋問題。

## 職責

此標頭定義一個極簡的 `nccu::HudSlot` enum class，僅含 `Top` 和 `Bottom` 兩個值。

它解決了一個具體的 bug：先前 `World` 只有單一訊息槽，章節提示在切章時僅存活 0.02 秒（一幀）就被下一幀發布的抵達訊息蓋掉。單純調整發布順序只能解決部分競爭；把頻道一分為二才徹底解決「抵達提示覆蓋章節提示」的問題。

`Event::slot` 欄位預設為 `HudSlot::Bottom`，使既有所有發布者（拾取訊息、karma 增減、抵達提示、攤販購買文字、離場準備）行為完全不變。只有少數高優先提示（章節切換、結局關卡）改用 `HudSlot::Top`，兩條頻道在畫面上各自佔一行，約 25 px 間距，同時清晰可讀。

設計上刻意獨立為一個小標頭（不含 raylib、不做渲染），使 `World / Event / MessageView / ChapterToast / EventWiring` 等不同層的型別都可以單獨引入此定義，而無需拉進整個事件系統或渲染層。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::HudSlot`**（`enum class`）：
  - `Top`：章節 / 結局等重大進度提示頻道，畫在 Bottom 訊息帶上方。
  - `Bottom`：其餘一切（拾取、karma 增減、抵達提示等），`Event::slot` 的預設值。

## 相依與在架構中的位置

- **#include（往外）**：無（不依賴任何標頭）——最底層純資料型別，可被所有層安全引入。
- **被誰使用（往內）**：`include/engine/events/EventBus.h`（`Event::slot` 欄位型別）、`include/game/world/World.h`（HUD 狀態儲存）、`include/ui/MessageView.h`（HUD 渲染分頻道）、部分測試檔（`test_chapter_transitions / test_two_hud_channels`）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/events 層的純資料型別；不參與任何管線邏輯，僅作為頻道路由的鍵值。

## OO 概念與設計重點

純資料 enum class，體現「最小知識」原則：只定義頻道語意，不含任何邏輯。獨立標頭避免了循環依賴（`EventBus.h` include 它、`World.h` include 它、`MessageView.h` include 它，三者互不相依），是 C++ 分離關注點（Separation of Concerns）的最小單位實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/events/HudSlot.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/HudSlot.h) · [← 全檔索引](../files-index.md)
