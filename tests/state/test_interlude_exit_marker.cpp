#include "doctest/doctest.h"
#include "game/state/InterludeExitMarker.h"
#include "game/state/InterludeExit.h"
#include "engine/render/IRenderer.h"

#include <string>
#include <vector>

/**
 * @file test_interlude_exit_marker.cpp
 * @brief 驗證幕間出口的視覺地標（南側帶狀區北緣的金色虛線）：佈局計算、
 *        動畫相位、繪製通道（陰影 + 金色本體）與顏色契約。
 */

namespace {

// 攔截用的 IRenderer：記錄每個繪圖原語，讓 View 端輔助函式可在無 GL 環境下
// 測試（不會有任何 raylib 繪圖呼叫真正送進畫面）。
struct Spy final : nccu::engine::render::IRenderer {
    struct RectCall { nccu::engine::math::Rect r; nccu::engine::math::Color c; };
    struct TextCall { std::string s; nccu::engine::math::Vec2 pos; int size;
                      nccu::engine::math::Color c; };
    std::vector<RectCall> rects;
    std::vector<TextCall> texts;
    int sprites = 0;

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view text, nccu::engine::math::Vec2 pos, int size,
                  nccu::engine::math::Color c) override {
        texts.push_back({std::string{text}, pos, size, c});
    }
};

} // namespace

// 南側出口區先前只是個無聲的 y>=1900 觸發；文字/事件回饋已先做，視覺地標
// 則由本批測試固定。下列測試釘住純佈局輔助函式與 IRenderer 繪製路徑，
// 避免日後回退。

// 佈局會沿帶狀區產生虛線段。
TEST_CASE("InterludeExitMarker: layout produces dashes along the band") {
    // 相位為 0 的確定性佈局至少要產生一段虛線；若回退成空清單（迴圈零步、
    // 邊界差一），整個視覺會悄悄消失。
    const auto L = nccu::LayoutInterludeExitMarker(0.0f);
    REQUIRE(!L.dashes.empty());

    // 每段虛線都位於出口區北緣（y == minY）、落在走廊的 x 範圍內、並使用
    // 設定好的線寬。任何一段偏離帶狀區都代表地標指向了錯誤的線 — 這是視覺契約。
    for (const auto& d : L.dashes) {
        CHECK(d.rect.y == doctest::Approx(nccu::kInterludeExitMinY));
        CHECK(d.rect.height == doctest::Approx(
            nccu::kInterludeMarkerThickness));
        CHECK(d.rect.x >= nccu::kInterludeExitMinX);
        CHECK(d.rect.x + d.rect.width <= nccu::kInterludeExitMaxX);
        // 被裁切的虛線段可能比 dashLen 短，但絕不會更長。
        CHECK(d.rect.width <= nccu::kInterludeMarkerDashLen +
                              0.001f);
        CHECK(d.rect.width > 0.0f);
    }
}

// 虛線橫跨整條走廊，因此會有多段。
TEST_CASE("InterludeExitMarker: spans the corridor (multiple dashes)") {
    // 走廊寬 1800 px（150..1950）。以 40px 虛線 + 20px 間隔（週期 60）計算，
    // 完整一輪約有 30 段。此處設一個寬鬆下界，確保日後調整虛線尺寸時仍能
    // 真正「鋪滿走廊」。
    const auto L = nccu::LayoutInterludeExitMarker(0.0f);
    CHECK(L.dashes.size() >= 20);
}

// 相位 0 與相位等於一個完整週期時，結果相同（具週期性）。
TEST_CASE("InterludeExitMarker: phase 0 ≡ phase period (periodic)") {
    // 相位參數以虛線週期取模；相位等於一個完整週期時，產生的虛線清單必須與
    // 相位 0 相同 — 否則動畫累加器會隨時間讓視覺漂移。
    const float period = nccu::kInterludeMarkerDashLen +
                         nccu::kInterludeMarkerGapLen;
    const auto A = nccu::LayoutInterludeExitMarker(0.0f);
    const auto B = nccu::LayoutInterludeExitMarker(period);
    REQUIRE(A.dashes.size() == B.dashes.size());
    for (std::size_t i = 0; i < A.dashes.size(); ++i) {
        CHECK(A.dashes[i].rect.x == doctest::Approx(B.dashes[i].rect.x));
        CHECK(A.dashes[i].rect.width ==
              doctest::Approx(B.dashes[i].rect.width));
    }
}

// 相位偏移會把虛線往東移動相位對應的像素數。
TEST_CASE("InterludeExitMarker: phase shifts dashes east by phase pixels") {
    // 一個小於間隔長度的相位偏移，會把每段未被裁切虛線的前緣往東移動恰好
    // 該量 — 即視覺上的流動效果。
    const auto A = nccu::LayoutInterludeExitMarker(0.0f);
    const auto B = nccu::LayoutInterludeExitMarker(10.0f);
    // 挑一段安全落在走廊內（兩端皆未被裁切）的虛線比較其 x 位置。
    REQUIRE(A.dashes.size() >= 5);
    REQUIRE(B.dashes.size() >= 5);
    const auto& a = A.dashes[5];
    const auto& b = B.dashes[5];
    // 若兩段都未被裁切（皆為完整寬度），位移量即為 +10。
    if (a.rect.width == doctest::Approx(nccu::kInterludeMarkerDashLen) &&
        b.rect.width == doctest::Approx(nccu::kInterludeMarkerDashLen)) {
        CHECK(b.rect.x - a.rect.x == doctest::Approx(10.0f));
    }
}

// 每段虛線繪製兩個矩形：陰影 + 金色本體。
TEST_CASE("InterludeExitMarker: draw emits shadow+gold per dash") {
    // 繪製器對每段虛線畫兩個矩形（陰影 + 金色本體），與任務指示器採用相同的
    // 兩道繪製手法。若回退而漏掉陰影，仍會畫出東西，但在明亮地磚上會看不清。
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    const auto layout = nccu::LayoutInterludeExitMarker(0.0f);
    REQUIRE(!layout.dashes.empty());
    CHECK(spy.rects.size() == layout.dashes.size() * 2);
    CHECK(spy.texts.empty());
    CHECK(spy.sprites == 0);
}

// 金色本體用 #FFC83D，陰影為深色半透明。
TEST_CASE("InterludeExitMarker: gold body uses #FFC83D, shadow is dark") {
    // 固定顏色契約 — 此金色與任務指示器面板一致，使兩種視覺提示共用同一調色盤。
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    REQUIRE(spy.rects.size() >= 2);

    // 繪製器對每段虛線先畫陰影再畫金色本體。抽查第一組。
    const auto shadow = spy.rects[0];
    const auto gold   = spy.rects[1];

    CHECK(shadow.c.r == 0);
    CHECK(shadow.c.g == 0);
    CHECK(shadow.c.b == 0);
    CHECK(shadow.c.a < 255);   // 半透明

    CHECK(gold.c.r == 255);
    CHECK(gold.c.g == 200);
    CHECK(gold.c.b == 61);
    CHECK(gold.c.a == 255);

    // 陰影相對金色本體往東南偏移 2 px。
    CHECK(shadow.r.x - gold.r.x == doctest::Approx(2.0f));
    CHECK(shadow.r.y - gold.r.y == doctest::Approx(2.0f));
}

// 地標必須位於出口區的北緣。
TEST_CASE("InterludeExitMarker: marker lives at the exit zone NORTH edge") {
    // 地標必須落在出口區的門檻（y==minY）而非其下方 — 否則玩家會在看到線之前
    // 就已跨入觸發帶，違背「提前預警」的用意。
    Spy spy;
    nccu::DrawInterludeExitMarker(spy, 0.0f);
    REQUIRE(!spy.rects.empty());
    // 陰影與本體都畫在這個 y（陰影往南微移 2 px）。
    bool foundOnEdge = false;
    for (const auto& rc : spy.rects) {
        if (rc.r.y == doctest::Approx(nccu::kInterludeExitMinY)) {
            foundOnEdge = true;
            break;
        }
    }
    CHECK(foundOnEdge);
}
