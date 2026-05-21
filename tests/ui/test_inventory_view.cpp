#include "doctest/doctest.h"
#include "ui/InventoryView.h"
#include "gfx/IRenderer.h"

#include <string>
#include <unordered_map>
#include <vector>

// S5b-5: DrawInventory is a pure function of the count map — spy the
// injected renderer (same shape as test_ending_card_render) and assert
// the panel + per-item lines without a GL context.

namespace {

struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    std::vector<std::string> texts;
    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color) override { ++rects; }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {}
    void DrawText(std::string_view t, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override { texts.emplace_back(t); }
};

bool Has(const std::vector<std::string>& v, const std::string& s) {
    for (const auto& x : v) if (x == s) return true;
    return false;
}

}  // namespace

TEST_CASE("DrawInventory: backdrop + panel + title + one line per item") {
    Spy r;
    std::unordered_map<std::string, int> items{
        {"HotPack", 2}, {"EnergyDrink", 1}};
    nccu::DrawInventory(r, items, 800.0f, 450.0f);

    CHECK(r.rects >= 2);                       // dim backdrop + panel
    CHECK(Has(r.texts, "物品欄"));             // title
    CHECK(Has(r.texts, "HotPack x2"));
    CHECK(Has(r.texts, "EnergyDrink x1"));
}

TEST_CASE("DrawInventory: empty inventory shows the （空） placeholder") {
    Spy r;
    std::unordered_map<std::string, int> none;
    nccu::DrawInventory(r, none, 800.0f, 450.0f);

    CHECK(r.rects >= 2);
    CHECK(Has(r.texts, "物品欄"));
    CHECK(Has(r.texts, "（空）"));
}

TEST_CASE("DrawInventory: a zero-count entry is not rendered") {
    Spy r;
    std::unordered_map<std::string, int> items{{"Ghost", 0}};
    nccu::DrawInventory(r, items, 800.0f, 450.0f);

    CHECK_FALSE(Has(r.texts, "Ghost x0"));
    CHECK(Has(r.texts, "（空）"));             // nothing positive -> empty
}
