#include "doctest/doctest.h"
#include "game/gfx/SpriteStrip.h"
#include "game/gfx/Decorations.h"
#include <cmath>
#include <limits>
#include <vector>

/**
 * @file test_sprite_strip.cpp
 * @brief 驗證精靈圖條的逐格計算：FrameAt 來回（ping-pong）動畫序列、
 *        StripSourceRect 切格、DecorationDestRect 置中縮放，以及 kDecorations 裝飾物擺放。
 */

using namespace nccu::engine::math;
using namespace nccu::game::gfx;
using nccu::SemesterState;
using nccu::kSportsTrackCx;
using nccu::kSportsTrackCy;

// 在每個整數刻度 t = k/fps 取樣 FrameAt 共 ticks 次，得到 View 每格會顯示的索引序列。
// fps 取 1.0 即可——此 helper 只取決於 floor(t*fps)。
static std::vector<int> Sample(int n, int ticks) {
    std::vector<int> seq;
    seq.reserve(static_cast<std::size_t>(ticks));
    for (int k = 0; k < ticks; ++k)
        seq.push_back(FrameAt(static_cast<double>(k), n, 1.0));
    return seq;
}

// n=4 時產生來回三角序列 0,1,2,3,2,1,0,1,...；週期為 2*(n-1)=6 刻度。
TEST_CASE("FrameAt：n=4 時產生來回三角序列 0,1,2,3,2,1,0,1,...") {
    // 第 7 個取樣（索引 6）回到 0，之後循環重複。
    const std::vector<int> expected =
        {0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1};
    CHECK(Sample(4, static_cast<int>(expected.size())) == expected);
}

// 頂點為 n-1，每個週期恰好碰到一次。
TEST_CASE("FrameAt：頂點為 n-1，每個週期恰好碰到一次") {
    // n=4：只有刻度 3（及 3 + 6k）回傳 3；鄰格皆為 2。
    CHECK(FrameAt(2.0, 4, 1.0) == 2);
    CHECK(FrameAt(3.0, 4, 1.0) == 3);   // 頂點
    CHECK(FrameAt(4.0, 4, 1.0) == 2);
    CHECK(FrameAt(9.0, 4, 1.0) == 3);   // 一個週期後再次回到頂點（3+6）
}

// 長時間取樣下索引永遠落在 [0, n-1]。
TEST_CASE("FrameAt：長時間取樣下索引永遠落在 [0, n-1]") {
    for (int n = 2; n <= 8; ++n) {
        for (int k = 0; k < 500; ++k) {
            const int f = FrameAt(static_cast<double>(k), n, 1.0);
            CHECK(f >= 0);
            CHECK(f <= n - 1);
        }
    }
}

// 兩格圖條來回 0,1,0,1,...（週期 2）。
TEST_CASE("FrameAt：兩格圖條來回 0,1,0,1,...（週期 2）") {
    const std::vector<int> expected = {0, 1, 0, 1, 0, 1};
    CHECK(Sample(2, 6) == expected);
}

// 退化輸入皆安全且為全函式，一律回傳第 0 格。
TEST_CASE("FrameAt：退化輸入皆安全且為全函式，一律回傳第 0 格") {
    CHECK(FrameAt(5.0, 1, 6.0) == 0);    // 單格圖條即靜態
    CHECK(FrameAt(5.0, 0, 6.0) == 0);    // 無任何格
    CHECK(FrameAt(5.0, -3, 6.0) == 0);   // 無意義的格數
    CHECK(FrameAt(5.0, 4, 0.0) == 0);    // 停止的時鐘
    CHECK(FrameAt(5.0, 4, -1.0) == 0);   // 負的 fps
    CHECK(FrameAt(std::nan(""), 4, 6.0) == 0);        // NaN 時間
    CHECK(FrameAt(std::numeric_limits<double>::infinity(), 4, 6.0) == 0);
}

// 負時間也不會產生負索引。
TEST_CASE("FrameAt：負時間也不會產生負索引") {
    for (int k = -50; k < 0; ++k) {
        const int f = FrameAt(static_cast<double>(k), 4, 1.0);
        CHECK(f >= 0);
        CHECK(f <= 3);
    }
}

// fps 控制每格持續幾個刻度。
TEST_CASE("FrameAt：fps 控制每格持續幾個刻度") {
    // fps=6 時每格持續 1/6 秒，故 t 在 [0,1/6) 為第 0 格、[1/6,2/6) 為第 1 格。
    CHECK(FrameAt(0.0,        8, 6.0) == 0);
    CHECK(FrameAt(0.10,       8, 6.0) == 0);   // 仍 < 1/6
    CHECK(FrameAt(1.0 / 6.0,  8, 6.0) == 1);   // 進入第 1 格
    CHECK(FrameAt(2.0 / 6.0,  8, 6.0) == 2);
}

// StripSourceRect 由左至右切割水平圖條。
TEST_CASE("StripSourceRect：由左至右切割水平圖條") {
    // 256x40 圖條共 8 格 -> 每格 32x40，沿 x 軸排列。
    const Rect r0 = StripSourceRect(0, 8, 256, 40);
    CHECK(r0.x == doctest::Approx(0.0f));
    CHECK(r0.y == doctest::Approx(0.0f));
    CHECK(r0.width == doctest::Approx(32.0f));
    CHECK(r0.height == doctest::Approx(40.0f));

    const Rect r3 = StripSourceRect(3, 8, 256, 40);
    CHECK(r3.x == doctest::Approx(96.0f));   // 3 * 32
    CHECK(r3.width == doctest::Approx(32.0f));

    const Rect rLast = StripSourceRect(7, 8, 256, 40);
    CHECK(rLast.x == doctest::Approx(224.0f));            // 7 * 32
    CHECK(rLast.x + rLast.width == doctest::Approx(256.0f)); // 結束於 texW
}

// frameCount<=0 時退化為整張貼圖。
TEST_CASE("StripSourceRect：frameCount<=0 時退化為整張貼圖") {
    const Rect r = StripSourceRect(0, 0, 100, 50);
    CHECK(r.width == doctest::Approx(100.0f));
    CHECK(r.height == doctest::Approx(50.0f));
}

// DecorationDestRect 以錨點置中，較長邊縮放為 drawScale。
TEST_CASE("DecorationDestRect：以錨點置中，較長邊縮放為 drawScale") {
    DecorationDef d{SemesterState::Chapter3_SportsDay, Vec2{100.0f, 200.0f},
                    "x", /*frameCount=*/8, /*drawScale=*/40.0f, /*fps=*/8.0};
    // 256x64 貼圖共 8 格 -> 一格 32x64；較長邊（64，高）縮放至 40 -> 比例 0.625 -> w=20, h=40。
    const Rect r = DecorationDestRect(d, 256, 64);
    CHECK(r.width == doctest::Approx(20.0f));
    CHECK(r.height == doctest::Approx(40.0f));
    // 置中：矩形中點即為錨點。
    CHECK(r.x + r.width * 0.5f == doctest::Approx(100.0f));
    CHECK(r.y + r.height * 0.5f == doctest::Approx(200.0f));
}

// 寬大於高的格子，將寬縮放為 drawScale。
TEST_CASE("DecorationDestRect：寬大於高的格子將寬縮放為 drawScale") {
    DecorationDef d{SemesterState::Chapter2_Midterms, Vec2{0.0f, 0.0f},
                    "x", /*frameCount=*/4, /*drawScale=*/80.0f, /*fps=*/6.0};
    // 400x50 貼圖共 4 格 -> 一格 100x50；較長邊為 100（寬）-> 比例 0.8 -> w=80, h=40。
    const Rect r = DecorationDestRect(d, 400, 50);
    CHECK(r.width == doctest::Approx(80.0f));
    CHECK(r.height == doctest::Approx(40.0f));
}

// 釘住 kDecorations 裝飾物的擺放：chiikawa 在學霸附近、貓在綜院西側（避免被建築遮擋）。
TEST_CASE("kDecorations：chiikawa 在學霸附近、貓在綜院西側") {
    // 此表是 View 讀取的唯一來源；釘住擺放位置與章節，讓誤改在此被攔下。
    //
    // 擺放理由：
    //   chiikawa — 由廣場幾何中心 (1088,960) 下移到 (1088,1040)，位於學霸
    //     1088,1100 站位上方 60 px，讓它讀起來像他癱坐其下的雕像（近學霸）。
    //   cat — 由跑道中心 (1694,740，落在綜合院館 x1681.. 範圍內而被遮住) 左移
    //     到 x1530 的開闊西側操場，位於綜院左緣 1681 之西，使其可見；
    //     仍在同一跑道列 y740，仍是小小一隻。
    bool sawChiikawa = false, sawCat = false;
    for (const auto& d : kDecorations) {
        if (d.chapter == SemesterState::Chapter2_Midterms) {
            sawChiikawa = true;
            CHECK(d.center.x == doctest::Approx(1088.0f));
            CHECK(d.center.y == doctest::Approx(1040.0f));   // 960 -> 1040
            // 落在學霸南緣站位 (1088,1100) 正上方，使兩者構成同一畫面組，不會在他下方或越過他。
            CHECK(d.center.y < 1100.0f);
            CHECK(d.center.y > 960.0f);                      // 確實已下移
            CHECK(d.frameCount >= 1);
        } else if (d.chapter == SemesterState::Chapter3_SportsDay) {
            sawCat = true;
            CHECK(d.center.x == doctest::Approx(1530.0f));   // 1694 -> 1530
            // 重點：位於綜合院館左緣 (1681) 之西，使建築不再蓋過它；仍在操場
            // 範圍內（rect x1384..2005）的跑道列上。
            CHECK(d.center.x < 1681.0f);                     // 不在綜院下方
            CHECK(d.center.x > 1384.0f);                     // 仍在操場上
            CHECK(d.center.y == doctest::Approx(kSportsTrackCy));  // 740，跑道列
            CHECK(d.drawScale <= 32.0f);   // 小小一隻
            CHECK(d.frameCount >= 1);
        }
    }
    CHECK(sawChiikawa);
    CHECK(sawCat);
}
