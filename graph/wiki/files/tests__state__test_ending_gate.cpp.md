---
id: file:tests/state/test_ending_gate.cpp
type: test
path: tests/state/test_ending_gate.cpp
domain: tests
bucket: state
loc: 318
classes: []
sources: ["tests/state/test_ending_gate.cpp"]
---
# `test_ending_gate.cpp`

> **一句定位**：最完整的結局判定測試：四種結局 A/B/C/D 的觸發條件、優先順序、僅限 Ch4 生效、對話延後語義，以及「做出最終選擇後判定必為完全（不卡關）」的窮舉証明。

## 職責

此測試檔（318 行）是整個結局系統的核心測試，通過 `CheckEndingGates` 函式驗證四種結局路徑及其優先順序，並包含一個窮舉証明（`ResolveCh4` 輔助 + 巢狀 for 迴圈）確保判定的完全性。

**測試群組如下：**

1. **Ch4 以外不觸發**：遍歷 Ch1/幕間/Ch2/Ch3，即使設下所有結局旗標也不轉場。

2. **Ending A 路徑**：karma>80 + `kFlagHasTrueUmbrella` + `kFlagConsoledTA` 三條件全滿 → A；缺任一條件的三個 SUBCASE（無體諒 → 停 Ch4；有體諒無傘 → D；有體諒無足夠 karma → D）。

3. **Ending B**：詛咒傘或 karma<0 的兩個 SUBCASE。

4. **Ending C**：`kFlagBoughtUglyUmbrella`（Ch4 集英樓商人設下）；Ch1 阿姨橋段不觸發 C（敘事鋪陳，無旗標）。

5. **優先順序 A > B > C**：三旗標同時成立時 A 勝出。

6. **對話延後語義**：`d.Open(...)` 後呼叫 `CheckEndingGates` 無效，`d.Close()` 後才觸發 C。

7. **完全判定（防卡關）**：五個 SUBCASE 驗證各種「`kFlagTaFinaleChoiceMade`已設但條件不足 A」的情境都落到某個結局（B/C/D），不卡在 Ch4。

8. **`A->B->D->C` 判定樹對照表**：`ResolveCh4` helper + 9 個 `CHECK` 逐條驗證（含 karma 邊界 80）。

9. **窮舉完全性証明**：在 `finaleMade=true` 下，6 個 karma 值 × 2 × 2 × 2 × 2 = 96 種組合，每種都 `CHECK(r != Chapter4_Finals)` 且 `CHECK(IsEndingState(r))`。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::CheckEndingGates(EventBus::Instance(), p, m, d)`：被測的結局輪詢函式。
- `nccu::IsEndingState(SemesterState)`（來自 `EndingView.h`）：判斷是否為結局狀態。
- `ResolveCh4(karma, trueUmb, consoled, finaleMade, cursed, boughtUgly)`：本地 helper，構建 Ch4 場景並回傳落到的狀態。
- 旗標：`kFlagHasTrueUmbrella`、`kFlagConsoledTA`、`kFlagTaFinaleChoiceMade`、`kFlagTookCursedUmbrella`、`kFlagBoughtUglyUmbrella`。

## 相依與在架構中的位置

- **#include（往外）**：`EventBus.h`、`Flags.h`、`EndingGate.h`、`SemesterStateMachine.h`、`DialogState.h`、`Player.h`、`EndingView.h`（`IsEndingState`）、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試每幀管線末端（Sweep 之前）的 `CheckEndingGates` 輪詢，這是 Model 層的狀態轉場判定點。

## OO 概念與設計重點

「完全判定不卡關」是此測試最重要的業務規則。曾有真實 bug：質問路徑設下 `kFlagTaFinaleChoiceMade` 但缺乏任何結局條件，導致玩家永久卡在 Ch4。窮舉証明（96 種組合）確保此 bug 不再回來。判定樹 A→B→D→C 的優先順序體現了敘事設計意圖（道德選擇勝過購物決定）。[State 模式](../concepts/pat-state.md)的 `SemesterStateMachine` 在此為被測主體。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_ending_gate.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_ending_gate.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md)
