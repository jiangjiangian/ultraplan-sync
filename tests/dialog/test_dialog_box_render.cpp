#include "doctest/doctest.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogView.h"
#include "engine/render/IRenderer.h"
#include <string>
#include <vector>

/**
 * @file test_dialog_box_render.cpp
 * @brief 驗證 DrawDialog 的對話框算繪：未啟用時不畫任何東西、啟用單行時畫出面板
 *        與當前台詞、選項模式下每個選項各畫一段文字且被選中者前綴 "> "。
 */

namespace {

// 間諜 IRenderer——沿用 test_umbrella_render.cpp 的 CountingRenderer 模式，
// 但額外擷取文字字串，讓我們能在無 GL 環境下驗證台詞／選項內容。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<std::string> texts;

    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color) override { ++rects; }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override { texts.emplace_back(t); }
};

} // namespace

// 未啟用的對話不畫任何東西。
TEST_CASE("Inactive dialog draws nothing") {
    nccu::DialogState d;
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

// 啟用的單行對話會畫出面板與當前台詞文字。
TEST_CASE("Active line dialog draws a panel + the current line text") {
    nccu::DialogState d;
    d.Open({"hello"});
    Spy s;
    nccu::DrawDialog(s, d);
    CHECK(s.rects >= 1);                 // 面板底色（與外框矩形）
    REQUIRE(s.texts.size() == 1);
    CHECK(s.texts[0] == "hello");
}

// 選項模式下每個選項各畫一段文字，被選中者前綴 "> "。
TEST_CASE("Choice mode draws one text per option, selected prefixed '> '") {
    nccu::DialogState d;
    d.Open({"intro"}, {{"refuse", 0, "", false},
                        {"accept", -5, "f", false}});
    d.Advance();                         // -> 選項模式
    Spy s;
    nccu::DrawDialog(s, d);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0].rfind("> ", 0) == 0);   // 選項 0 被選中
    CHECK(s.texts[1].rfind("  ", 0) == 0);   // 選項 1 未選中
}
