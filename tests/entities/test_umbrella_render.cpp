#include "doctest/doctest.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/FragileUmbrella.h"
#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/CursedUmbrella.h"
#include "engine/render/IRenderer.h"

#include <set>
#include <tuple>
#include <vector>

/**
 * @file test_umbrella_render.cpp
 * @brief 驗證四種雨傘的算繪外觀必須明顯不同：各 UmbrellaStyle 畫出不同的輪廓
 *        （矩形指紋與數量相異），且每把傘的傘面色調彼此相距夠遠，不會被混淆。
 */

namespace {

// 間諜 IRenderer：記錄每個繪圖基元而不觸碰 GL 環境，讓多型的 Render() 路徑
// 可在無視窗環境下測試。
struct CountingRenderer final : nccu::engine::render::IRenderer {
    struct RectCall { nccu::engine::math::Rect r; nccu::engine::math::Color c; };
    std::vector<RectCall> rects;
    int sprites = 0;
    int texts = 0;

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override {
        ++texts;
    }
};

// 整個圖形的標準指紋（依繪製順序記下每個矩形的幾何與顏色）。兩種雨傘款式
// 「相同」的充要條件就是此指紋相符。
std::vector<std::tuple<float,float,float,float,int,int,int,int>>
Fingerprint(const CountingRenderer& spy) {
    std::vector<std::tuple<float,float,float,float,int,int,int,int>> fp;
    for (const auto& rc : spy.rects)
        fp.emplace_back(rc.r.x, rc.r.y, rc.r.width, rc.r.height,
                        rc.c.r, rc.c.g, rc.c.b, rc.c.a);
    return fp;
}

template <class U>
CountingRenderer DrawOf() {
    U u{nccu::engine::math::Vec2{100.0f, 200.0f}};
    CountingRenderer spy;
    u.Render(spy);
    return spy;
}

} // namespace

// 四種雨傘必須看起來明顯不同。每個 UmbrellaStyle 都畫出自己獨特的輪廓、配上
// 自己鮮明且彼此分得開的色調，同時維持純矩形的向量圖形（無 sprite／文字——
// 符合 MVC：View 依物件自身的資料算繪）。
TEST_CASE("REQ#9: the four umbrellas each render a DISTINCT glyph") {
    const CountingRenderer t = DrawOf<TrueUmbrella>();
    const CountingRenderer f = DrawOf<FragileUmbrella>();
    const CountingRenderer p = DrawOf<ProfessorTrapUmbrella>();
    const CountingRenderer c = DrawOf<CursedUmbrella>();

    // 每種款式都仍是純向量圖形（無 sprite、無文字）。
    for (const CountingRenderer* s : {&t, &f, &p, &c}) {
        CHECK(s->sprites == 0);
        CHECK(s->texts == 0);
        CHECK(s->rects.size() >= 3);          // 真正畫出來的雨傘
    }

    // 關鍵斷言：四個完整圖形指紋兩兩相異（幾何與／或顏色），而非「同形狀、
    // 不同色調」。把指紋放入 std::set 必須得到 4 個。
    using FP = std::vector<std::tuple<float,float,float,float,int,int,int,int>>;
    std::set<FP> distinct{Fingerprint(t), Fingerprint(f),
                          Fingerprint(p), Fingerprint(c)};
    CHECK(distinct.size() == 4);

    // 形狀也不同（不只是替同一骨架換色）：各款式的矩形數量相異，因此輪廓
    // 是真的不同，而非只是上色。
    std::set<std::size_t> rectCounts{
        t.rects.size(), f.rects.size(), p.rects.size(), c.rects.size()};
    CHECK(rectCounts.size() >= 3);            // 4 種中至少 3 種矩形數量不同
}

// 每把傘鮮明的色調必須相距夠遠。驗證每一對傘面色調的通道差總和夠大，
// 在地圖上才不會被混淆。
TEST_CASE("REQ#9: umbrella canopy tints are boldly separated") {
    auto canopy = [](const CountingRenderer& s) {
        // 對每種款式而言，rects[0] 都是最上方的傘面色塊，其顏色就是
        // 該把傘的標誌色調。
        return s.rects.at(0).c;
    };
    const nccu::engine::math::Color ct = canopy(DrawOf<TrueUmbrella>());
    const nccu::engine::math::Color cf = canopy(DrawOf<FragileUmbrella>());
    const nccu::engine::math::Color cp = canopy(DrawOf<ProfessorTrapUmbrella>());
    const nccu::engine::math::Color cc = canopy(DrawOf<CursedUmbrella>());

    auto dist = [](nccu::engine::math::Color a, nccu::engine::math::Color b) {
        auto d = [](int x, int y) { return x > y ? x - y : y - x; };
        return d(a.r, b.r) + d(a.g, b.g) + d(a.b, b.b);
    };
    // 每一對的曼哈頓色距 ≥ 120：才能確保四把傘的傘面色調彼此分得夠開。
    const nccu::engine::math::Color all[] = {ct, cf, cp, cc};
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            CHECK(dist(all[i], all[j]) >= 120);
}
