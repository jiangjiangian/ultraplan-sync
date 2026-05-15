#include "doctest/doctest.h"
#include "EndingView.h"
#include "SemesterState.h"
#include "gfx/IRenderer.h"
#include <string>
#include <vector>

using nccu::SemesterState;

namespace {

// Spy IRenderer — same shape as the proven spy in
// test_dialog_box_render.cpp: counts rects and captures text strings so
// the ending card's draw calls can be asserted without a GL context.
struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<std::string> texts;

    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color) override { ++rects; }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override { texts.emplace_back(t); }
};

} // namespace

TEST_CASE("IsEndingState is true only for Ending_* states") {
    CHECK(nccu::IsEndingState(SemesterState::Ending_C));
    CHECK_FALSE(nccu::IsEndingState(SemesterState::Chapter1_AddDrop));
}

TEST_CASE("DrawEndingCard issues a backdrop + title + caption") {
    Spy r;
    nccu::DrawEndingCard(r, SemesterState::Ending_C, "結局 C", 1.0f,
                         800.0f, 450.0f);
    CHECK(r.rects >= 1);              // backdrop
    CHECK(r.texts.size() >= 2);       // title + 字卡
}
