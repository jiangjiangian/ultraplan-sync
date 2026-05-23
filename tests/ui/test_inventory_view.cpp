#include "doctest/doctest.h"
#include "ui/InventoryView.h"
#include "quest/ItemCatalog.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "entities/Player.h"
#include "gfx/IRenderer.h"

#include <string>
#include <vector>

// Item 2: DrawInventory is a pure function of the InventoryRow DTO + the
// cursor index — spy the injected renderer (same shape as
// test_ending_card_render) and assert the panel + per-row lines + the
// description panel without a GL context.

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

TEST_CASE("DrawInventory: backdrop + panel + title + one line per row + caret") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"能量飲料", 1, "喝下提神", true, "EnergyDrink"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);                       // dim backdrop + panel
    CHECK(Has(r.texts, "物品欄"));             // title
    // Row 0 is selected -> "> " caret; row 1 -> "  " padding. Counts shown.
    CHECK(Has(r.texts, "> 暖暖包 x2"));
    CHECK(Has(r.texts, "  能量飲料 x1"));
}

TEST_CASE("DrawInventory: cursor moves the caret to the selected row") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"能量飲料", 1, "喝下提神", true, "EnergyDrink"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/1, 800.0f, 450.0f);
    CHECK(Has(r.texts, "  暖暖包 x2"));
    CHECK(Has(r.texts, "> 能量飲料 x1"));
}

TEST_CASE("DrawInventory: selected row's description + use hint are drawn") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾全身雨水", true, "HotPack"},
        {"金幣", 100, "你的錢包餘額", false, nccu::kItemMoney},
    };
    // Cursor on the usable consumable -> "E 使用" hint + its description.
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "立刻烘乾全身雨水"));
    CHECK(Has(r.texts, "↑↓ 選擇   E 使用"));

    // Cursor on the view-only 金幣 row -> the plain hint (no E 使用).
    Spy r2;
    nccu::DrawInventory(r2, rows, /*cursor=*/1, 800.0f, 450.0f);
    CHECK(Has(r2.texts, "你的錢包餘額"));
    CHECK(Has(r2.texts, "↑↓ 選擇"));
    CHECK_FALSE(Has(r2.texts, "↑↓ 選擇   E 使用"));
}

TEST_CASE("DrawInventory: a single-instance row (count 0) shows no xN suffix") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"真傘", 0, "完美結局的關鍵", false, nccu::kItemTrueUmbrella},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 真傘"));
    CHECK_FALSE(Has(r.texts, "> 真傘 x0"));
}

TEST_CASE("DrawInventory: empty bag shows the （空） placeholder") {
    Spy r;
    std::vector<nccu::InventoryRow> none;
    nccu::DrawInventory(r, none, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);
    CHECK(Has(r.texts, "物品欄"));
    CHECK(Has(r.texts, "（空）"));
}

TEST_CASE("DrawInventory: an out-of-range cursor is clamped, not a crash") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 1, "立刻烘乾", true, "HotPack"},
    };
    // cursor 99 must clamp to the last (only) row and still draw it.
    nccu::DrawInventory(r, rows, /*cursor=*/99, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 暖暖包 x1"));
}

// Item 2(c)/(e) regression: BuildInventoryRows aggregates EVERY category
// the player meaningfully holds — 金幣 + held consumables + the carried
// umbrella + current-cycle quest papers — each with the catalog name.
namespace {
const nccu::InventoryRow* Find(const std::vector<nccu::InventoryRow>& rows,
                               std::string_view itemId) {
    for (const auto& r : rows) if (r.itemId == itemId) return &r;
    return nullptr;
}
}  // namespace

TEST_CASE("Item 2c: BuildInventoryRows aggregates money + consumable + umbrella + quest") {
    Player p{nccu::gfx::Vec2{0, 0}};
    // Default Player: only 金幣 (100), empty bag, no flags.
    {
        const auto rows = nccu::BuildInventoryRows(p);
        REQUIRE(rows.size() == 1);
        const nccu::InventoryRow* money = Find(rows, nccu::kItemMoney);
        REQUIRE(money != nullptr);
        CHECK(money->name == "金幣");
        CHECK(money->count == 100);          // balance carried in count
        CHECK_FALSE(money->usable);          // 金幣 is view-only
        CHECK_FALSE(money->description.empty());
    }

    // Hold consumables, the true umbrella, the 申請書, and 2/3 notes.
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    p.SetFlag("Flag_HasTrueUmbrella");
    p.SetFlag("Flag_FoundForm");
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote3);

    const auto rows = nccu::BuildInventoryRows(p);

    // 金幣 first (deterministic order), still present.
    REQUIRE(!rows.empty());
    CHECK(rows.front().itemId == nccu::kItemMoney);

    // Consumables with their counts + usable flag.
    const nccu::InventoryRow* hot = Find(rows, "HotPack");
    REQUIRE(hot != nullptr);
    CHECK(hot->name == "暖暖包");
    CHECK(hot->count == 2);
    CHECK(hot->usable);
    const nccu::InventoryRow* energy = Find(rows, "EnergyDrink");
    REQUIRE(energy != nullptr);
    CHECK(energy->name == "能量飲料");
    CHECK(energy->count == 1);
    CHECK(energy->usable);

    // The carried umbrella, derived from the flag (true umbrella wins).
    const nccu::InventoryRow* umb = Find(rows, nccu::kItemTrueUmbrella);
    REQUIRE(umb != nullptr);
    CHECK(umb->name == "真傘");
    CHECK(umb->count == 0);                  // single instance, no xN
    CHECK_FALSE(umb->usable);

    // Quest papers: 申請書 + the 三頁筆記 collapsed into one "xN" row.
    const nccu::InventoryRow* form = Find(rows, nccu::kItemForm);
    REQUIRE(form != nullptr);
    CHECK(form->name == "申請書");
    const nccu::InventoryRow* notes = Find(rows, nccu::kItemNotes);
    REQUIRE(notes != nullptr);
    CHECK(notes->count == 2);                // 2 of the 3 held
    CHECK_FALSE(notes->usable);
}

TEST_CASE("Item 2c: umbrella row reflects the carried variant by flag priority") {
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_BoughtUglyUmbrella");
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK(Find(rows, nccu::kItemUglyUmbrella) != nullptr);
        CHECK(Find(rows, nccu::kItemTrueUmbrella) == nullptr);
    }
    // The cursed path.
    Player q{nccu::gfx::Vec2{0, 0}};
    q.SetFlag("Flag_TookCursedUmbrella");
    {
        const auto rows = nccu::BuildInventoryRows(q);
        const nccu::InventoryRow* c = Find(rows, nccu::kItemCursedUmbrella);
        REQUIRE(c != nullptr);
        CHECK(c->name == "詛咒傘");
    }
    // The carried 苦主's umbrella (mid-quest, before the return grant).
    Player v{nccu::gfx::Vec2{0, 0}};
    v.SetFlag(nccu::kFlagHasVictimUmbrella);
    {
        const auto rows = nccu::BuildInventoryRows(v);
        CHECK(Find(rows, nccu::kItemVictimUmbrella) != nullptr);
    }
}
