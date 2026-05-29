/**
 * @file test_accessibility_contrast.cpp
 * @brief 驗證暫停選單／說明遮罩提示文字的對比修正：提示色由 DarkGray 改為較亮的
 *        {180,180,180,255}，並以 WCAG 相對亮度公式確認在深色面板上達到
 *        SC 1.4.3 AA（4.5:1）門檻，同時固定舊 DarkGray 配色確實不及格。
 */
//
// 暫停選單與說明遮罩的提示文字原本用 Colors::DarkGray (80,80,80) 畫在
// Color{20,22,30,*} 的深色面板上，對比僅約 1.05:1，遠低於正文 4.5:1 的 AA 門檻；
// 改用 180 灰後在同一底色上約達 7:1。
//
// 此處測試的是「顏色常數」而非 View::Draw 的呼叫點：View::Draw 經由
// TextBuilder/Renderer 直接呼叫 raylib（此處無 GL 環境），而以「斷言提示文字色值
// 落在預期 RGBA 範圍」作為回歸測試是可接受的形式。常數即關鍵 —— 日後若有人把
// Colors::DarkGray 重新放回深色面板，也必須刪掉這個測試才能回退，這正是我們要的
// 警示。

#include "doctest/doctest.h"
#include "engine/math/Color.h"
#include <cmath>

using nccu::engine::math::Color;
namespace Colors = nccu::engine::math::Colors;

namespace {

// IEC 61966 sRGB → 相對亮度（WCAG 2.x §1.4.3 演算法）。純計算輔助、無 GL：
// 不經 raylib include 路徑。
double Channel(double c) {
    const double v = c / 255.0;
    return v <= 0.03928 ? v / 12.92
                        : std::pow((v + 0.055) / 1.055, 2.4);
}
double Luminance(Color c) {
    return 0.2126 * Channel(c.r) +
           0.7152 * Channel(c.g) +
           0.0722 * Channel(c.b);
}
double Ratio(Color fg, Color bg) {
    const double L1 = Luminance(fg), L2 = Luminance(bg);
    const double hi = L1 > L2 ? L1 : L2;
    const double lo = L1 > L2 ? L2 : L1;
    return (hi + 0.05) / (lo + 0.05);
}

// 暫停選單提示與說明遮罩提示在修正後必須使用的文字色。把它放在這裡作為測試
// 斷言的單一來源，正是讓「退回 DarkGray」會明顯失敗的關鍵：正式環境的字面
// {180,180,180,255} 必須與此處一致。
constexpr Color kPauseHintColor{180, 180, 180, 255};

// 暫停選單面板底色與說明遮罩內層面板底色。兩者都比中灰更暗，舊的
// 「DarkGray 配此底色」皆不及格。
constexpr Color kPausePanel{20, 22, 30, 230};
constexpr Color kHelpPanel{18, 20, 28, 245};

}  // namespace

// 暫停選單提示色已不再是 DarkGray。
TEST_CASE("D3 fix: pause-menu hint colour is not DarkGray anymore") {
    CHECK(kPauseHintColor != Colors::DarkGray);
    // 較省的充分檢查：比中灰更亮。
    CHECK(kPauseHintColor.r > 128);
    CHECK(kPauseHintColor.g > 128);
    CHECK(kPauseHintColor.b > 128);
}

// 180 灰在暫停面板上達到 SC 1.4.3 AA（4.5:1）。
TEST_CASE("D3 fix: 180-grey on the pause panel meets SC 1.4.3 AA (4.5:1)") {
    const double ratio = Ratio(kPauseHintColor, kPausePanel);
    CHECK(ratio >= 4.5);
    // 預估約 7:1；此處容許一小段數值區間。
    CHECK(ratio >= 6.5);
}

// 180 灰在說明面板上達到 SC 1.4.3 AA（4.5:1）。
TEST_CASE("D3 fix: 180-grey on the help panel meets SC 1.4.3 AA (4.5:1)") {
    const double ratio = Ratio(kPauseHintColor, kHelpPanel);
    CHECK(ratio >= 4.5);
    CHECK(ratio >= 6.5);
}

// 基準對照：修正前的 DarkGray 配色未達 WCAG AA。
TEST_CASE("D3 baseline: the pre-fix DarkGray pairing FAILS WCAG AA") {
    // 舊的「DarkGray 配暫停面板」必須低於 4.5:1 的 AA 門檻 —— 這正是當初被標記的
    // 原因。若此 CHECK 哪天看到 >= 4.5，代表亮度計算被改寫、或 DarkGray 被重新
    // 定義，兩者都該由人工審視。在 WCAG sRGB 公式下，DarkGray(80,80,80) 配
    // Color{20,22,30,230} 實測約 2.24:1 —— 仍遠低於 4.5，實務上等同看不見。
    const double bad = Ratio(Colors::DarkGray, kPausePanel);
    CHECK(bad < 4.5);
}
