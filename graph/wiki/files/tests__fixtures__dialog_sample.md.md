---
id: file:tests/fixtures/dialog_sample.md
type: other
path: tests/fixtures/dialog_sample.md
domain: tests
bucket: fixtures
loc: 39
classes: []
sources: ["tests/fixtures/dialog_sample.md"]
---
# `dialog_sample.md`（DialogLoader 測試資料）

> **一句定位**：餵給 `DialogLoader` 解析器的共用測試 fixture——刻意鋪滿各種邊界，驗證 Markdown→對白的解析正確性。

## 職責
這不是 `TEST_CASE`，而是被 `tests/dialog/` 諸測試載入的**樣本對白檔**。它以一份「小而全」的 Markdown
劇本，把 `DialogLoader` 解析器需要正確處理的各種情況都塞進同一個檔案，讓測試能對照預期結果斷言。

## 內容（這份 fixture 涵蓋哪些解析邊界）
- **章節 metadata 區塊**（`SemesterState: TestState`）位於第一個 `## NPC：` 之前，**應被忽略**。
- **場景旁白** narration（不屬任何 NPC）**應被忽略**。
- **`## NPC：學長`**：含 `>` 場景註與多個 substate `### (a) 初次接觸`、`### (b) 二次接觸`，各帶數行台詞。
- **`## NPC：學妹`**：刻意使用 **CJK 全形引號**（「」/ “”）以驗證 parser 對引號變體的容忍；含
  `### (c) 沒台詞的 substate`（驗證空 substate 不致命）。
- **章節結尾** 的尾段 narration **應被忽略**。

換言之，fixture 同時測「該抓的台詞有抓到」與「該忽略的雜訊有忽略」，以及 substate 標籤 (a)/(b)/(c) 與
CJK 引號、空段落等邊界。

## 相依與在架構中的位置
測試資料，無程式相依。被 `tests/dialog/test_dialog_loader.cpp` 等載入；對應的解析器見
[`DialogLoader.h`](include__game__dialog__DialogLoader.h.md) / [`DialogLoader.cpp`](src__game__dialog__DialogLoader.cpp.md)。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/fixtures/dialog_sample.md) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/fixtures/dialog_sample.md) · [← 全檔索引](../files-index.md)
