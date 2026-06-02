---
id: "file:tests/ui/test_reduced_motion.cpp"
type: test
path: tests/ui/test_reduced_motion.cpp
domain: tests
bucket: ui
loc: 108
classes: []
sources: ["tests/ui/test_reduced_motion.cpp"]
---
# `test_reduced_motion.cpp`

> **一句定位**：驗證「減少動畫」無障礙偏好的預設值、setter 雙向切換、三個動畫閘函式的行為，以及環境變數 `UMBRELLA_REDUCED_MOTION` 的接線。

## 職責

本測試固定 `World::ReducedMotion()` 旗標及三個對應的動畫閘函式：

**預設值與 setter**：清除環境變數後建立 `World`，確認 `ReducedMotion()` 預設為 false；`SetReducedMotion(true)` 開啟；`SetReducedMotion(false)` 關回（雙向切換）。

**三個動畫閘函式的開關行為**（均以 dt = 1/60 測試）：
- `InterludeMarkerPhaseStep(dt, reducedMotion)`：正常時 > 0.0（虛線前進）；減少動畫時 = 0.0（原地停止）。
- `EndingFadeAlphaStep(current, dt, reducedMotion)`：正常時回傳介於 0 到 1 的漸增值；減少動畫時（無論 current 為何）立即回傳 1.0（跳至全亮）。
- `HudToastFadeT(progress, fadeWindow, reducedMotion)`：正常時淡出途中係數約 0.5；減少動畫時全程維持 1.0（到 TTL 才由呼叫端硬切）。

**環境變數接線**：`UMBRELLA_REDUCED_MOTION=1` → `ReadWorldOptionsFromEnv()` 解析出 `opts.reducedMotion=true` → World 建構子遵循；`"0"` 或未設定維持 false（只有字面 `"1"` 才啟用）。

## 關鍵內容（類別 / 函式 / 資料）

- `World::ReducedMotion()` / `World::SetReducedMotion(bool)` — 被測。
- `EndingFadeAlphaStep(float current, float dt, bool reduced)` — 被測：結局卡淡入閘。
- `HudToastFadeT(float progress, float fadeWindow, bool reduced)` — 被測：HUD 淡出閘。
- `InterludeMarkerPhaseStep(float dt, bool reduced)` — 被測：幕間地標流動閘。
- `nccu::ReadWorldOptionsFromEnv()` / `nccu::WorldOptions` — 被測環境變數解析器。

## 相依與在架構中的位置

- **#include（往外）**：`ui/ReducedMotion.h`（三個動畫閘函式）、`game/world/World.h`（旗標與選項建構）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層旗標測試 + View 層動畫閘函式測試。

## OO 概念與設計重點

三個動畫閘函式是純函式（無狀態），設計上刻意可測試：以 `bool reducedMotion` 參數控制路徑，無需 mock World。環境變數解析獨立在 `ReadWorldOptionsFromEnv()` 中，使 `World` 建構子相對引數為純函式，測試兩半可分開驗證。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_reduced_motion.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_reduced_motion.cpp) · [← 全檔索引](../files-index.md)
