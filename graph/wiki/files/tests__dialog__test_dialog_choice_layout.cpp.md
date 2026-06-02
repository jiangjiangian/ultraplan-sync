---
id: "file:tests/dialog/test_dialog_choice_layout.cpp"
type: test
path: tests/dialog/test_dialog_choice_layout.cpp
domain: tests
bucket: dialog
loc: 128
classes: [PosSpy, T]
sources: ["tests/dialog/test_dialog_choice_layout.cpp"]
---
# `test_dialog_choice_layout.cpp`

> **一句定位**：以位置感知的 `PosSpy` IRenderer 驗證寬對話框（`kBoxCells`）下的選項排版：每個選項一列、游標隨 `MoveChoice` 移動、選項間距非等距，以及三選項能容納於 110px 面板。

## 職責

本檔針對 `DrawDialog` 在選項模式下的垂直排版邏輯，特別驗證 Ch4 終局選單（三選項：「體諒助教的辛勞」/ 「質問／強硬索回」/ `kDialogExitLabel "我再想想…"`）在寬對話框下的正確呈現。

**`PosSpy`**：繼承 `IRenderer`，記錄每次 `DrawText` 的文字與 y 座標（`struct T { std::string text; float y; }`），讓 case 能驗證排版的垂直間距與游標位置。

**`OpenFinaleMenu()`**：helper，建構真正的 Ch4 終局選單形狀（3 選項），呼叫 `d.Advance()` 越過開場白進入選項模式。

五個 `TEST_CASE`：
1. **每個選項算繪成一列**：驗證三個標籤在 `kBoxCells` 下都短於框寬（`CellWidth <= kBoxCells`），`s.texts.size() == 3`，且各含正確關鍵字。
2. **恰好一個 `"> "` 標記**：`marked == 1`、`blank == 2`，游標初始在選項 0。
3. **游標隨 `MoveChoice(1)` 移動**：選項 1 帶 `"> "`，選項 0/2 帶 `"  "`。
4. **選項間距非等距（`> kBoxLineH`）**：`pitch01 > kBoxLineH`、`pitch12 > kBoxLineH`，且 `pitch01 ≈ pitch12`（選項間距一致）。
5. **容納於 110px 面板**：最後一列底部 `maxY + kBoxLineH <= kBoxY + kBoxH`，確認不溢出面板。

## 關鍵內容（類別 / 函式 / 資料）

- `struct PosSpy : IRenderer`：記錄 `(text, y)` 的算繪間諜。
- `struct T`：`PosSpy` 的記錄單元。
- `OpenFinaleMenu()`：建構 Ch4 終局三選項選單並推進到選項模式。
- `dialog::kBoxCells`、`kBoxTextY`、`kBoxLineH`、`kBoxH`、`kBoxY`：對話框佈局常數。
- `nccu::kDialogExitLabel`：婉拒選項的標籤常數（`"我再想想…"`）。
- `nccu::dialog::CellWidth`：計算字串的 cell 寬度。

## 相依與在架構中的位置
- **#include（往外）**：`include/engine/render/IRenderer.h`、`include/game/dialog/DialogLayout.h`、`include/game/dialog/DialogState.h`、`include/game/dialog/DialogView.h`、`include/game/quest/Flags.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`PosSpy` 繼承 `IRenderer`
- **每幀管線 / MVC 角色**：驗證 View 層的選項排版邏輯

## OO 概念與設計重點

`PosSpy` 擴充了 `test_dialog_box_render.cpp` 的 `Spy` 模式，加上 y 座標記錄，讓排版的垂直間距可被數值驗證。這釘住了「非等距選項間隔」的設計（選項間距 `> kBoxLineH`），防止未來的排版重構將選單改回等距造成視覺退化。測試以佈局常數（`kBoxCells` 等）推導期望值，而非寫死像素數字，對任何對話框尺寸調整都保持適應性。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_choice_layout.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_choice_layout.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-dip-renderer](../concepts/arch-dip-renderer.md)
