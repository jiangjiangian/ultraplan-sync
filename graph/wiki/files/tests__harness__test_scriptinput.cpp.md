---
id: file:tests/harness/test_scriptinput.cpp
type: test
path: tests/harness/test_scriptinput.cpp
domain: tests
bucket: harness
loc: 95
classes: []
sources: ["tests/harness/test_scriptinput.cpp"]
---
# `test_scriptinput.cpp`

> **一句定位**：驗證確定性輸入驅動器 `ScriptInput` 的腳本解析與 edge 語意——`down/up` 的按住與邊緣、`press` 單格 tap、`quit` 精確格觸發，以及具名鍵可解析而垃圾行被靜默略過。

## 職責

此測試以四個 TEST_CASE 規格化 `ScriptInput` 的全部指令語意，重點在確保其 edge 語意與 raylib 的 `IsKeyPressed/Released` 完全對應——任何偏差都會讓所有腳本化執行悄悄失準。

`ScriptInput` 解析文字腳本（`<frame> <verb> [key]`）並精確重放輸入序列。其核心契約：
- `down K` 在指定格啟動按住，`IsDown` + `IsPressed` 在第一格均為 true，之後 `IsDown` 維持 true 但 `IsPressed` 歸零。
- `up K` 在指定格產生 released edge（`IsReleased`），之後歸零。
- `press K` 為單格 tap：按住一格、下一格自動 released edge，不留殘餘。
- `quit` 在指定格設定 `WantsQuit()`，之前不設。
- 未知動詞（`wiggle`）或未知鍵 token（`ZZZ`）靜默略過；`#` 開頭的行被當作注釋略過。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ScriptInput：down/up 的按住與 edge 語意對應 raylib")`：`0 down D` / `3 up D`，逐格驗證 IsDown/IsPressed/IsReleased，確認第 4 格不留殘餘 released 狀態。
- `TEST_CASE("ScriptInput：press 是單格 tap，下一格自動放開")`：`5 press E`，前 5 格均不按；第 5 格 IsDown+IsPressed；第 6 格 IsReleased 且不再 IsDown；第 7 格無殘餘。
- `TEST_CASE("ScriptInput：quit 指令在其指定格才設定 WantsQuit，之前不會")`：`2 quit`，格 0、1 均 false，格 2 才 true。
- `TEST_CASE("ScriptInput：具名鍵與字母可解析，垃圾行被略過")`：確認 `Space` 和 `Enter` 解析成功，`ZZZ`（未知鍵）和 `wiggle`（未知動詞）被靜默略過。

## 相依與在架構中的位置

- **#include（往外）**：`engine/platform/ScriptInput.h`（受測），`engine/input/Key.h`（鍵枚舉）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（測試 harness 核心元件，不在遊戲管線內）

## OO 概念與設計重點

[harness 架構](../concepts/arch-harness.md) 的基礎測試：`ScriptInput` 是 `LiveInput` 的確定性替代品，使測試與正常遊玩的輸入路徑完全相同。edge 語意的精確測試（逐格驗證 pressed/released 的一次性）防止了「tap 永遠按住」或「release 無限殘留」等靜默 bug，這些 bug 在整合測試中非常難察覺。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/harness/test_scriptinput.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/harness/test_scriptinput.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
