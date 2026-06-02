---
id: file:include/ui/ReducedMotion.h
type: header
path: include/ui/ReducedMotion.h
domain: ui
bucket: 
loc: 51
classes: []
sources: ["include/ui/ReducedMotion.h"]
---
# `ReducedMotion.h`

> **一句定位**：View 與 MessageView 共用的純動畫閘門——三個 constexpr 輔助函式，讓「減少動畫」無障礙偏好在單一處關閉所有連續時間動畫，不影響存檔的決定性。

## 職責

本標頭把「減少動畫（`reducedMotion`）」無障礙偏好的控制邏輯集中到三個 `constexpr` 純函式中，防止各消費端各自硬寫 `if (reducedMotion) ...` 造成邏輯漂移。

三個函式各自負責一種動畫：
- `InterludeMarkerPhaseStep(dt, reduced)`：幕間出口地面標記的掃動相位累加量；`reduced=true` 時回傳 0（凍結在相位 0 處），`reduced=false` 時回傳 `dt * 30.0f`（正常掃動速度）。
- `EndingFadeAlphaStep(currentAlpha, dt, reduced)`：結局字卡淡入的 alpha 步進；`reduced=true` 時直接回傳 1.0（首幀即全不透明），`reduced=false` 時回傳 `std::min(1.0f, currentAlpha + dt)`（約一秒內漸升）。
- `HudToastFadeT(remaining, fade, reduced)`：HUD 提示框末段淡出係數 [0,1]；`reduced=true` 時恆回傳 1.0（不淡出，到 TTL 邊界硬切），`reduced=false` 時計算 `remaining / fade` 並夾限到 [0,1]（線性淡出）。

所有函式均為 `constexpr noexcept`，不依賴 raylib，可無頭單元測試（`tests/ui/test_reduced_motion.cpp`）。注解強調「預設行為（`reduced=false`）與先前的內聯算式逐位元等價」，確保接上這些 helper 不會擾動自動跑流程在「旗標關閉」預設情形下的決定性結果。

## 關鍵內容（類別 / 函式 / 資料）

- `InterludeMarkerPhaseStep(dt, reduced) -> float`（`constexpr noexcept`）：幕間標記掃動相位步進；reduced 凍結，否則 `dt * 30`。
- `EndingFadeAlphaStep(currentAlpha, dt, reduced) -> float`（`constexpr noexcept`）：結局淡入步進；reduced 直接 1.0，否則 `min(1.0f, currentAlpha + dt)`。
- `HudToastFadeT(remaining, fade, reduced) -> float`（`constexpr noexcept`）：HUD 淡出係數 [0,1]；reduced 恆 1.0，否則 `clamp(remaining/fade, 0, 1)`。

## 相依與在架構中的位置

- **#include（往外）**：`<algorithm>`（`std::min`）；無其他依賴。
- **被誰使用（往內）**：`src/ui/MessageView.cpp`（`HudToastFadeT` 計算提示框淡出係數）；`src/ui/View.cpp`（`InterludeMarkerPhaseStep` 更新標記動畫、`EndingFadeAlphaStep` 更新結局淡入）；`tests/ui/test_reduced_motion.cpp`（三個函式的單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層工具函式，在 `View::Draw()` 的各渲染階段每幀呼叫，純計算無副作用，不涉及模型或控制器。

## OO 概念與設計重點

本標頭的核心設計動機是 **單一真實來源（Single Source of Truth）** 的無障礙邏輯：把散落在多處的 `if (reducedMotion)` 判斷聚合到三個有明確語義的 helper 中，使未來調整「減少動畫」行為只需改此一處，且測試覆蓋率集中在此。`constexpr` 讓這些 helper 在任何常數表達式環境中都可用，且保留了「`reduced=false` 路徑與原始內聯算式逐位元等價」的硬性要求——這是確保重構不破壞存檔決定性的明確設計契約。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/ReducedMotion.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/ReducedMotion.h) · [← 全檔索引](../files-index.md)
