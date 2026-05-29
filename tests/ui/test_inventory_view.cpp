#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "ui/InventoryView.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/entities/Player.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/dialog/DialogLayout.h"   // CellWidth — 換行斷言用

#include <string>
#include <vector>

/**
 * @file test_inventory_view.cpp
 * @brief 驗證物品欄：DrawInventory 的面板／逐列文字／選取游標／說明面板繪製，
 *        BuildInventoryRows 彙整金幣、消耗品、手持傘與任務紙張，背包傘列依
 *        手持傘種類而非結局旗標、各分類左側色塊、分頁視窗計算與長說明換行。
 */

namespace {

// DrawInventory 是 InventoryRow 資料物件加游標索引的純函式 —— 以注入的攔截器
// 觀察，在無 GL 環境下斷言面板、逐列文字與說明面板。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    std::vector<nccu::engine::math::Color> rectColors;
    std::vector<std::string> texts;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color c) override {
        ++rects; rectColors.push_back(c);
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override { texts.emplace_back(t); }
};

bool Has(const std::vector<std::string>& v, const std::string& s) {
    for (const auto& x : v) if (x == s) return true;
    return false;
}

bool HasRectRGB(const Spy& s, nccu::engine::math::Color want) {
    for (const nccu::engine::math::Color& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

}  // namespace

// 繪製背景、面板、標題、每列一行文字與選取游標。
TEST_CASE("DrawInventory: backdrop + panel + title + one line per row + caret") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"能量飲料", 1, "喝下提神", true, "EnergyDrink"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);                       // 暗背景 + 面板
    CHECK(Has(r.texts, "物品欄"));             // 標題
    // 第 0 列被選取 -> 「> 」游標；第 1 列 -> 「  」留白。並顯示數量。
    CHECK(Has(r.texts, "> 暖暖包 x2"));
    CHECK(Has(r.texts, "  能量飲料 x1"));
}

// 游標會把選取符號移到被選取的列。
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

// 會畫出被選取列的說明與使用提示。
TEST_CASE("DrawInventory: selected row's description + use hint are drawn") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾全身雨水", true, "HotPack"},
        {"金幣", 100, "你的錢包餘額", false, nccu::kItemMoney},
    };
    // 游標在可用消耗品上 -> 顯示「E 使用」提示與其說明。
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "立刻烘乾全身雨水"));
    CHECK(Has(r.texts, "↑↓ 選擇   E 使用"));

    // 游標在僅供檢視的金幣列 -> 只顯示一般提示（無 E 使用）。
    Spy r2;
    nccu::DrawInventory(r2, rows, /*cursor=*/1, 800.0f, 450.0f);
    CHECK(Has(r2.texts, "你的錢包餘額"));
    CHECK(Has(r2.texts, "↑↓ 選擇"));
    CHECK_FALSE(Has(r2.texts, "↑↓ 選擇   E 使用"));
}

// 單一持有的列（count 為 0）不顯示 xN 後綴。
TEST_CASE("DrawInventory: a single-instance row (count 0) shows no xN suffix") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"真傘", 0, "完美結局的關鍵", false, nccu::kItemTrueUmbrella},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 真傘"));
    CHECK_FALSE(Has(r.texts, "> 真傘 x0"));
}

// 空背包會顯示「（空）」佔位字。
TEST_CASE("DrawInventory: empty bag shows the （空） placeholder") {
    Spy r;
    std::vector<nccu::InventoryRow> none;
    nccu::DrawInventory(r, none, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);
    CHECK(Has(r.texts, "物品欄"));
    CHECK(Has(r.texts, "（空）"));
}

// 超出範圍的游標會被夾住，而非當機。
TEST_CASE("DrawInventory: an out-of-range cursor is clamped, not a crash") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 1, "立刻烘乾", true, "HotPack"},
    };
    // 游標 99 必須夾到最後一列（也是唯一一列）並仍能繪製。
    nccu::DrawInventory(r, rows, /*cursor=*/99, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 暖暖包 x1"));
}

// BuildInventoryRows 會彙整玩家實際持有的每一種類別 —— 金幣 + 持有的消耗品 +
// 攜帶的傘 + 當前章節的任務紙張 —— 每項都帶有道具表名稱。
namespace {
const nccu::InventoryRow* Find(const std::vector<nccu::InventoryRow>& rows,
                               std::string_view itemId) {
    for (const auto& r : rows) if (r.itemId == itemId) return &r;
    return nullptr;
}
}  // namespace

// BuildInventoryRows 彙整金幣、消耗品、傘與任務紙張。
TEST_CASE("Item 2c: BuildInventoryRows aggregates money + consumable + umbrella + quest") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    // 預設 Player：只有金幣（100）、空背包、無旗標。
    {
        const auto rows = nccu::BuildInventoryRows(p);
        REQUIRE(rows.size() == 1);
        const nccu::InventoryRow* money = Find(rows, nccu::kItemMoney);
        REQUIRE(money != nullptr);
        CHECK(money->name == "金幣");
        CHECK(money->count == 100);          // 餘額存於 count
        CHECK_FALSE(money->usable);          // 金幣僅供檢視
        CHECK_FALSE(money->description.empty());
    }

    // 持有消耗品、真傘、申請書，以及三頁筆記中的兩頁。
    // 背包傘列由「手持傘種類」（SetHeldUmbrella）驅動，而非持久的結局旗標 ——
    // 此處兩者都設，如同實際發放處（旗標是結局 A 的標記；手持種類才是背包列的
    // 真正來源）。
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    p.SetHeldUmbrella(HeldUmbrella::True);
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagFoundForm);
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

    // The held umbrella, derived from HeldUmbrellaKind() (B2.1).
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

// B2.1: the bag umbrella row reflects the umbrella the player is HOLDING
// RIGHT NOW (HeldUmbrellaKind), not a persistent ending flag. The ugly /
// cursed ending flags are NEVER cleared, so keying the bag off them left a
// stale row after the umbrella was lost; the held-kind is the source of truth.
TEST_CASE("B2.1: bag umbrella row reflects the HELD umbrella, not ending flags") {
    // Ugly held → ugly row (even though Flag_BoughtUglyUmbrella is the
    // Ending C marker; here it is the held kind that drives the row).
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.SetHeldUmbrella(HeldUmbrella::Ugly);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK(Find(rows, nccu::kItemUglyUmbrella) != nullptr);
        CHECK(Find(rows, nccu::kItemTrueUmbrella) == nullptr);
    }
    // The cursed held umbrella → 詛咒傘 row.
    Player q{nccu::engine::math::Vec2{0, 0}};
    q.SetHeldUmbrella(HeldUmbrella::Cursed);
    {
        const auto rows = nccu::BuildInventoryRows(q);
        const nccu::InventoryRow* c = Find(rows, nccu::kItemCursedUmbrella);
        REQUIRE(c != nullptr);
        CHECK(c->name == "詛咒傘");
    }
    // The carried 苦主's umbrella (mid-quest, before the return grant) is a
    // CARRIED quest item (flag-driven, NO shelter) — still shown, but it is
    // not a held-over-head umbrella, so HasUmbrella stays false.
    Player v{nccu::engine::math::Vec2{0, 0}};
    v.SetFlag(nccu::kFlagHasVictimUmbrella);
    {
        const auto rows = nccu::BuildInventoryRows(v);
        CHECK(Find(rows, nccu::kItemVictimUmbrella) != nullptr);
        CHECK_FALSE(v.HasUmbrella());
    }
}

// B2.1 core regression: a LOST umbrella must vanish from the bag. The ending
// flag persists (it decides A/B/C), but SetHasUmbrella(false) — the Ch4-entry
// 傘再度失蹤 / a per-chapter「傘又掉了」reset — clears the held kind, so the
// umbrella row disappears and no stale row lingers.
TEST_CASE("B2.1: SetHasUmbrella(false) removes the umbrella row though the ending flag persists") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    // Hold the cursed umbrella + the persistent Ending B marker (as
    // CursedUmbrella::BeClaimed sets them together).
    p.SetHeldUmbrella(HeldUmbrella::Cursed).SetFlag(nccu::kFlagTookCursedUmbrella);
    REQUIRE(Find(nccu::BuildInventoryRows(p), nccu::kItemCursedUmbrella) != nullptr);

    // The umbrella is lost (Ch4 entry / per-chapter reset).
    p.SetHasUmbrella(false);
    const auto rows = nccu::BuildInventoryRows(p);
    // No umbrella row of ANY kind, even though the ending flag is still set.
    CHECK(Find(rows, nccu::kItemCursedUmbrella) == nullptr);
    CHECK(Find(rows, nccu::kItemTrueUmbrella) == nullptr);
    CHECK(Find(rows, nccu::kItemUglyUmbrella) == nullptr);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));   // ending flag untouched
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
}

// B2.1: every held-over-head kind maps to exactly one catalog row (the 破傘
// / 陷阱傘 / 管理員的傘 additions included); None / Victim yield no held-kind
// row (Victim is carried, shown via its quest flag).
TEST_CASE("B2.1: each HeldUmbrella kind maps to its catalog row") {
    struct Case { HeldUmbrella kind; const char* item; };
    const Case cases[] = {
        {HeldUmbrella::True,          nccu::kItemTrueUmbrella},
        {HeldUmbrella::Cursed,        nccu::kItemCursedUmbrella},
        {HeldUmbrella::Ugly,          nccu::kItemUglyUmbrella},
        {HeldUmbrella::Fragile,       nccu::kItemFragileUmbrella},
        {HeldUmbrella::ProfessorTrap, nccu::kItemProfTrapUmbrella},
        {HeldUmbrella::Loaner,        nccu::kItemLoanerUmbrella},
    };
    for (const auto& c : cases) {
        Player p{nccu::engine::math::Vec2{0, 0}};
        p.SetHeldUmbrella(c.kind);
        const auto rows = nccu::BuildInventoryRows(p);
        const nccu::InventoryRow* r = Find(rows, c.item);
        REQUIRE(r != nullptr);
        CHECK_FALSE(r->name.empty());
        CHECK_FALSE(r->description.empty());
        CHECK_FALSE(r->usable);            // umbrellas are view-only
    }
    // None / Victim → no held-kind umbrella row.
    Player none{nccu::engine::math::Vec2{0, 0}};
    CHECK(nccu::HeldUmbrellaCatalogId(none.HeldUmbrellaKind()) == nullptr);
    CHECK(nccu::HeldUmbrellaCatalogId(HeldUmbrella::Victim) == nullptr);
}

// T6: each bag row gets a left-edge CATEGORY swatch so 金幣 / 雨傘 / 任務紙張
// read as a different KIND than usable consumables. The umbrella row draws
// the SAME shared glyph (and its signature colour) the world / ending use,
// keyed off the carried-umbrella sentinel.
TEST_CASE("T6: the umbrella bag row draws its umbrella-look swatch") {
    using nccu::game::gfx::UmbrellaLook;
    // Cursed umbrella row → the dark-purple swatch.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"詛咒傘", 0, "傘骨上刻著別人的名字", false,
             nccu::kItemCursedUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
    }
    // Ugly umbrella row → the fluorescent-green swatch.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"螢光綠醜傘", 0, "醜得全校最好認", false,
             nccu::kItemUglyUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    }
    // True umbrella row → the blue swatch.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"真傘", 0, "完美結局的關鍵", false, nccu::kItemTrueUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
}

TEST_CASE("T6: a multi-category bag draws more rects than a bare list") {
    // A bag with money + a consumable + an umbrella + a paper draws a swatch
    // per row (so the categories are visually distinct), i.e. clearly more
    // rects than the backdrop + panel + underline alone.
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"金幣", 100, "餘額", false, nccu::kItemMoney},
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"真傘", 0, "關鍵", false, nccu::kItemTrueUmbrella},
        {"申請書", 0, "交給助教", false, nccu::kItemForm},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    // backdrop + panel + underline = 3; then a selection bar + >=1 swatch
    // rect per row. Four categories ⇒ comfortably more than 3 + 4.
    CHECK(r.rects > 8);
    // The 4 row names + the description name + description + hint still draw.
    CHECK(Has(r.texts, "> 金幣 x100"));
    CHECK(Has(r.texts, "  真傘"));
}

// ---- U2-T1: paging window math (pure, no renderer) -----------------------
TEST_CASE("U2-T1: InventoryPageCount / InventoryPageOf window math") {
    using nccu::InventoryPageCount;
    using nccu::InventoryPageOf;
    const int P = nccu::kInventoryRowsPerPage;
    REQUIRE(P >= 1);

    // Empty / single page.
    CHECK(InventoryPageCount(0) == 1);          // empty bag is "1 / 1"
    CHECK(InventoryPageCount(1) == 1);
    CHECK(InventoryPageCount(P) == 1);          // exactly one full page
    CHECK(InventoryPageCount(P + 1) == 2);      // one over → a 2nd page
    CHECK(InventoryPageCount(2 * P) == 2);
    CHECK(InventoryPageCount(2 * P + 1) == 3);

    // The cursor's page: row 0..P-1 → page 0, P..2P-1 → page 1, …
    CHECK(InventoryPageOf(0, 3 * P) == 0);
    CHECK(InventoryPageOf(P - 1, 3 * P) == 0);
    CHECK(InventoryPageOf(P, 3 * P) == 1);
    CHECK(InventoryPageOf(2 * P, 3 * P) == 2);
    // Out-of-range cursor is clamped before the division (no negative page,
    // no page past the last) — mirrors the View's own clamp.
    CHECK(InventoryPageOf(-5, 2 * P) == 0);
    CHECK(InventoryPageOf(9999, 2 * P) == 1);   // last page
    CHECK(InventoryPageOf(0, 0) == 0);          // empty bag
}

// U2-T1: a bag with more rows than fit shows the cursor's PAGE (so the
// selected row is visible) and a 「第 N／M 頁」 indicator. The off-page rows
// are NOT drawn; the on-page rows + the selected row ARE.
TEST_CASE("U2-T1: an over-full bag pages and shows the page indicator") {
    const int P = nccu::kInventoryRowsPerPage;
    // Build P+2 rows: r00..r(P+1). Names are unique so we can check which
    // page is drawn.
    std::vector<nccu::InventoryRow> rows;
    for (int i = 0; i < P + 2; ++i) {
        std::string nm = "道具" + std::to_string(i);
        rows.push_back({nm, 0, "說明", false, "id" + std::to_string(i)});
    }
    const int total = nccu::InventoryPageCount(P + 2);
    REQUIRE(total == 2);

    // Cursor on row 0 → page 1: row0 visible, the last row (page 2) NOT.
    {
        Spy r;
        nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
        CHECK(Has(r.texts, "> 道具0"));
        CHECK_FALSE(Has(r.texts, "  道具" + std::to_string(P + 1)));
        CHECK(Has(r.texts, "第 1／2 頁   ←／→ 翻頁"));
    }
    // Cursor on the last row → page 2: that row visible + selected; row0 NOT.
    {
        Spy r;
        nccu::DrawInventory(r, rows, /*cursor=*/P + 1, 800.0f, 450.0f);
        CHECK(Has(r.texts, "> 道具" + std::to_string(P + 1)));
        CHECK_FALSE(Has(r.texts, "  道具0"));
        CHECK(Has(r.texts, "第 2／2 頁   ←／→ 翻頁"));
    }
}

// U2-T1: a single-page bag still shows a 「第 1／1 頁」 indicator (consistent
// affordance), but WITHOUT the ←/→ 翻頁 hint (there is nowhere to flip).
TEST_CASE("U2-T1: a single-page bag shows 第 1／1 頁 without the flip hint") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"金幣", 100, "餘額", false, nccu::kItemMoney},
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "第 1／1 頁"));
    CHECK_FALSE(Has(r.texts, "第 1／1 頁   ←／→ 翻頁"));
}

// ---- U2-T2: the long post-G4 description WRAPS inside the box ------------
// The densest catalog line must be split into rows by CellWidth so no single
// drawn text run exceeds the box's inner cell budget — i.e. it can't spill
// past the border. We assert via the same CellWidth the wrap uses.
TEST_CASE("U2-T2: a long description is wrapped to fit the box width") {
    Spy r;
    const std::string longDesc =
        "使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響業力。";
    std::vector<nccu::InventoryRow> rows{
        {"防水噴霧", 1, longDesc, true, "WaterproofSpray"}};
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);

    // Every drawn text run that is a slice of the description must be within
    // the box budget (54 cells — the value DrawInventory wraps to). The
    // un-split full string (≈48 cells here) would also pass the 54 bound,
    // so to prove WRAPPING really happened we additionally check that a
    // deliberately over-long description is broken into >1 run.
    for (const std::string& t : r.texts)
        CHECK(nccu::dialog::CellWidth(t) <= 54);

    Spy r2;
    std::string huge;
    for (int i = 0; i < 8; ++i) huge += "防水噴霧雨量擋雨業力";  // ~80 cells
    std::vector<nccu::InventoryRow> rows2{
        {"測試", 0, huge, false, "id"}};
    nccu::DrawInventory(r2, rows2, 0, 800.0f, 450.0f);
    int sliceRows = 0;
    for (const std::string& t : r2.texts)
        if (t.find("防水噴霧") != std::string::npos) ++sliceRows;
    CHECK(sliceRows >= 2);                       // it wrapped onto >1 row
    for (const std::string& t : r2.texts)
        CHECK(nccu::dialog::CellWidth(t) <= 54); // never wider than the box
}

// ---- U2-T3: UmbrellaLookOf maps the new held-umbrella sentinels ----------
// The bag swatch must draw the CORRECT umbrella look for every held kind —
// previously fragile/proftrap fell through to the (wrong) intact blue. We
// assert the swatch's signature colour appears in the drawn rects.
TEST_CASE("U2-T3: fragile→破傘 and proftrap→陷阱傘 swatches are correct") {
    using nccu::game::gfx::UmbrellaLook;
    using nccu::game::gfx::UmbrellaLookColor;

    // 破傘 (Fragile) → the FragileBroken (handle/ribs, grey) signature.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"破傘", 0, "骨架斷了的傘", false, nccu::kItemFragileUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::FragileBroken)));
        // And NOT the intact-blue it used to wrongly default to.
        CHECK_FALSE(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
    // 陷阱傘 (ProfessorTrap) → the danger-red signature.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"陷阱傘", 0, "傘骨上有奇怪的刻字", false,
             nccu::kItemProfTrapUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::ProfessorTrap)));
        CHECK_FALSE(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
    // The plain 管理員的傘 loaner is fine as the clean blue canopy.
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"管理員的傘", 0, "圖書館管理員借你的傘", false,
             nccu::kItemLoanerUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
}

// U2-T3: the Ch3 物物交換鏈 carried items (香腸 / 大聲公) are VIEW-ONLY 道具,
// not usable consumables — they must (a) NOT show the 「E 使用」 hint and (b)
// draw a DISTINCT food/道具 swatch, not the teal potion-flask of a usable
// consumable.
TEST_CASE("U2-T3: Ch3 trade items are non-usable food-swatch rows") {
    // The distinct food swatch colour DrawSwatch uses for RowKind::Food.
    const nccu::engine::math::Color kFoodParcel{225, 140, 55, 255};
    // The teal consumable-flask body — the Ch3 items must NOT draw this.
    const nccu::engine::math::Color kConsumableFlask{60, 200, 180, 255};

    for (const char* id : {nccu::kItemSausage, nccu::kItemLoudspeaker}) {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"道具", 0, "拿去交換", /*usable=*/false, id}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        // View-only: the plain hint, never the use hint.
        CHECK(Has(r.texts, "↑↓ 選擇"));
        CHECK_FALSE(Has(r.texts, "↑↓ 選擇   E 使用"));
        // Food swatch present; the teal consumable flask absent.
        CHECK(HasRectRGB(r, kFoodParcel));
        CHECK_FALSE(HasRectRGB(r, kConsumableFlask));
    }
}
