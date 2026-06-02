---
id: file:tests/ui/test_accessibility_contrast.cpp
type: test
path: tests/ui/test_accessibility_contrast.cpp
domain: tests
bucket: ui
loc: 91
classes: []
sources: ["tests/ui/test_accessibility_contrast.cpp"]
---
# `test_accessibility_contrast.cpp`

> **一句定位**：以 WCAG 2.x sRGB 相對亮度公式驗證暫停選單與說明遮罩的提示文字色（180 灰）在深色面板上達到 SC 1.4.3 AA 4.5:1 門檻，並固定舊 DarkGray 配色確實不及格。

## 職責

此測試檔屬於 `tests/ui/` 桶，是遊戲中唯一的無障礙對比度測試。它不呼叫 raylib，完全以純計算方式驗證顏色常數符合 WCAG 規範。

背景：暫停選單與說明遮罩的提示文字原本使用 `Colors::DarkGray` (80,80,80) 畫在 `Color{20,22,30,*}` 的深色面板上，對比僅約 1.05:1（正文 AA 門檻為 4.5:1）；改用 `{180,180,180,255}` 後約達 7:1。

四個 TEST_CASE：
1. **提示色不是 DarkGray**：`kPauseHintColor != Colors::DarkGray`，且 r/g/b > 128（比中灰更亮）。
2. **暫停面板達 AA**：`Ratio(kPauseHintColor, kPausePanel) >= 4.5`（且 `>= 6.5` 確認約 7:1）。
3. **說明面板達 AA**：同上，底色改為 `kHelpPanel{18,20,28,245}`。
4. **舊 DarkGray 配色不及格**：`Ratio(Colors::DarkGray, kPausePanel) < 4.5`——若此 CHECK 哪天看到 `>= 4.5`，代表亮度計算被改寫或 DarkGray 被重新定義。

本地 `Channel/Luminance/Ratio` 函式精確實作 IEC 61966 sRGB → 相對亮度的 WCAG 演算法，不依賴任何外部庫。

## 關鍵內容（類別 / 函式 / 資料）

- `Channel(double c)`：IEC 61966 sRGB 線性化（低於 0.03928 線性；否則 gamma 2.4 還原）。
- `Luminance(Color c)`：WCAG 相對亮度（0.2126R + 0.7152G + 0.0722B 的線性權重）。
- `Ratio(Color fg, Color bg)`：對比比率 `(hi + 0.05) / (lo + 0.05)`。
- `kPauseHintColor{180, 180, 180, 255}`：修正後的提示文字色常數（測試的「單一真實來源」）。
- `kPausePanel{20, 22, 30, 230}`：暫停選單面板底色。
- `kHelpPanel{18, 20, 28, 245}`：說明遮罩面板底色。
- `TEST_CASE("暫停選單提示色已不再是 DarkGray")`。
- `TEST_CASE("180 灰在暫停面板上達到 AA 對比 4.5:1")`。
- `TEST_CASE("180 灰在說明面板上達到 AA 對比 4.5:1")`。
- `TEST_CASE("修正前的 DarkGray 配色未通過 WCAG AA")`：對照組，驗證測試本身的有意義性。

## 相依與在架構中的位置

- **#include（往外）**：`Color.h`（`Color`、`Colors::DarkGray`）、`<cmath>`（`std::pow`）。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 View 層的顏色常數，對應 `PauseMenu`/`HelpOverlay` 的文字渲染。

## OO 概念與設計重點

「常數即契約」是此測試的核心設計：View 端使用的字面值 `{180,180,180,255}` 必須與 `kPauseHintColor` 完全一致，若有人把 `Colors::DarkGray` 重新放回深色面板，必須先刪掉第一個 TEST_CASE 才能回退，形成強制性的可審計改動。「對照組」（舊配色不及格）確保 `Ratio` 函式的實作沒有方向錯誤——若亮度計算的分子分母顛倒，舊配色反而會「通過」，此 TEST_CASE 就會失敗並發出警報。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_accessibility_contrast.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_accessibility_contrast.cpp) · [← 全檔索引](../files-index.md)
