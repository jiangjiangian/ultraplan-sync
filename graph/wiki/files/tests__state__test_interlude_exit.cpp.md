---
id: file:tests/state/test_interlude_exit.cpp
type: test
path: tests/state/test_interlude_exit.cpp
domain: tests
bucket: state
loc: 66
classes: []
sources: ["tests/state/test_interlude_exit.cpp"]
---
# `test_interlude_exit.cpp`

> **一句定位**：驗證幕間市集南側出口觸發區的判定幾何、防卡關不變量（入口點不在區內），以及踏入觸發區後依 `returnTo` 轉場回指定章節的流程。

## 職責

此測試檔鎖定 `nccu::InInterludeExitZone` 的幾何語義與 `CheckChapterGates` 在幕間的行為，共三個 TEST_CASE：

1. **幾何判定**：y=2000（深入南側帶）→ 在區內；x=1800, y=1905 → 在區內；y=1899（北緣以上）→ 在區外；y=1280（雨傘區）→ 在區外；x=50（走廊以外）→ 在區外。

2. **防卡關不變量**：`nccu::kInterludeEntry`（市集入口點）不在出口區內——如果一進市集就觸發離開帶，玩家會被立刻彈回而略過整個市集。

3. **端到端觸發流程**：設 `InterludeReturnTo(Chapter3_SportsDay)`，切換到幕間；在入口點（不在區內）呼叫 `CheckChapterGates` 無效；設 `kFlagLeaveInterlude` 後呼叫 `CheckChapterGates` → 轉到 `Chapter3_SportsDay`，旗標被消耗（`!HasFlag(kFlagLeaveInterlude)`）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::InInterludeExitZone(Vec2)`：幾何判定函式。
- `nccu::kInterludeEntry`：市集入口點常數（Vec2）。
- `nccu::CheckChapterGates(EventBus, p, m, d)`：在幕間消耗離開旗標並轉場。
- `TEST_CASE("InterludeExit：南側帶狀區內的點視為在區內，北邊視為在區外")`。
- `TEST_CASE("InterludeExit：市集入口點不可落在出口區內（防卡關）")`。
- `TEST_CASE("InterludeExit：踏入南側觸發區後轉移到 returnTo 章節")`。

## 相依與在架構中的位置

- **#include（往外）**：`EventBus.h`、`Flags.h`（`kFlagLeaveInterlude`）、`InterludeExit.h`（`InInterludeExitZone`、`kInterludeEntry`）、`ChapterGate.h`（`CheckChapterGates`）、`SemesterStateMachine.h`、`DialogState.h`、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試每幀中 `GameController` 呼叫 `InInterludeExitZone` 後設旗標、再由 `CheckChapterGates` 消耗的兩步流程。

## OO 概念與設計重點

「觸發區幾何固定」是此測試的核心價值：`kInterludeExitMinY`（帶狀區北緣）等常數若調整，三個測試必須同步更新，防止地圖調整讓出口觸發點偏移到不預期的位置。防卡關不變量（入口點不在區內）體現了「後置條件守門」設計。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_interlude_exit.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_interlude_exit.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
