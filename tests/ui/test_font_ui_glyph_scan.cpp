#include "doctest/doctest.h"
#include "engine/render/Font.h"
#include "ui/EndingView.h"
#include "ui/ChapterCard.h"
#include "ui/GameHelp.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/QuestObjective.h"
#include "game/vendor/VendorMessages.h"
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

// 5c — ROBUST glyph-coverage gate. Every glyph used in a code-built UI
// string MUST be baked into the CJK font atlas (CollectCodepoints()),
// else raylib renders the no-glyph `?` tofu. The existing
// test_font_ui_glyphs.cpp pins specific historical glyphs; THIS test
// SCANS the actual UI literal strings and fails if ANY glyph is absent,
// so a future edit that adds an uncovered glyph (a new ending reason
// line, a help-copy tweak, a menu label) can't silently drift back into
// tofu. The strings come from the SAME source the renderer draws:
// EndingCardStrings() (every ending-screen branch) + GameHelp + the
// View.cpp HUD/menu/inventory literals.
//
// T5 broadens the scan to EVERY code-built UI string the owner saw `?` in:
// the HUD 目標 objective text (QuestObjectiveStrings — 綜合院館 / 集英樓 /
// 羅馬廣場 / 操場 / 校慶 / 期末考終焉…), the ItemCatalog names+descriptions
// (bag rows + vendor toast), the Vendor toast pieces (VendorMessages), the
// new 【雨傘外觀】 help lines, and the ending strings. So an uncovered glyph
// anywhere on those surfaces fails the build, not the player's screen.
//
// Headless-safe: CollectCodepoints() + raylib's codepoint decoder need
// no GL context (same as the sibling font test).

using nccu::engine::render::detail::CollectCodepoints;

namespace {

// Decode a UTF-8 string to codepoints via raylib's own decoder (the path
// CollectCodepoints uses), and assert each is in the atlas set.
void RequireAllCovered(const std::vector<int>& atlas, const std::string& s,
                       const char* whatFor) {
    int n = 0;
    int* dec = ::LoadCodepoints(s.c_str(), &n);
    for (int i = 0; i < n; ++i) {
        const int cp = dec[i];
        if (cp <= 0) continue;
        // ASCII control / space and printable are always in the atlas
        // (CollectCodepoints adds 32..126 unconditionally); skip the
        // no-glyph 0 the decoder emits on a malformed tail.
        INFO(whatFor << " glyph U+" << std::hex << std::setw(4)
                     << std::setfill('0') << cp << " in: " << s);
        CHECK(std::find(atlas.begin(), atlas.end(), cp) != atlas.end());
    }
    ::UnloadCodepoints(dec);
}

// The code-built View.cpp HUD / menu / inventory / affordance literals
// (the CJK ones — ASCII-only strings need no atlas check). Kept here as
// the curated companion to the auto-derived ending/help sets; if a new
// View literal adds a glyph, add it here so the gate covers it.
const std::vector<std::string>& ViewLiterals() {
    static const std::vector<std::string> kV = {
        "金幣: %d 元",
        "M 選單",
        "Tab: 物品欄   M: 選單",   // UI-B-2 top-left control hint
        "遊戲選單",
        "繼續", "說明", "減少動畫", "擴大目標", "重新開始", "離開",
        "  [開]", "  [關]",
        "↑ ↓ 選擇   Enter 確認   M 繼續",
        "遊戲說明",
        "M / E 返回選單",
        "物品欄",
        "（空）",
        "\xE2\x96\xBC more",   // ▼ more (DialogView affordance)
    };
    return kV;
}

} // namespace

TEST_CASE("5c: every ending-screen glyph is baked into the font atlas") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> strs = nccu::EndingCardStrings();
    CHECK(strs.size() > 10);          // sanity: the tables really enumerated
    for (const std::string& s : strs)
        RequireAllCovered(atlas, s, "ending-screen");
}

TEST_CASE("U1-T2: every chapter bookend big-card glyph is baked into the atlas") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> strs = nccu::ChapterCardStrings();
    CHECK(strs.size() >= 8);          // 4 chapter Lost pairs + the Found pair
    for (const std::string& s : strs)
        RequireAllCovered(atlas, s, "chapter-card");
}

TEST_CASE("5c: every 遊戲說明 (GameHelp) glyph is baked into the atlas") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string_view ln : nccu::kGameHelpLines)
        RequireAllCovered(atlas, std::string{ln}, "help-line");
    RequireAllCovered(atlas, std::string{nccu::kGameHelpClosing},
                      "help-closing");
}

// U2-T4: the help is now PAGED (kGameHelpPages). Scan the paged view too,
// so a glyph added to a page line that somehow isn't mirrored in the flat
// kGameHelpLines still can't tofu on screen — the renderers draw the pages.
TEST_CASE("U2-T4: every paged 遊戲說明 glyph is baked into the atlas") {
    const std::vector<int> atlas = CollectCodepoints();
    CHECK(nccu::kGameHelpPageCount == 2);
    int pageNo = 0;
    for (const auto page : nccu::kGameHelpPages) {
        ++pageNo;
        for (const std::string_view ln : page)
            RequireAllCovered(atlas, std::string{ln}, "help-page");
    }
    CHECK(pageNo == 2);
}

TEST_CASE("5c: every View.cpp HUD/menu/inventory literal glyph is baked") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string& s : ViewLiterals())
        RequireAllCovered(atlas, s, "view-literal");
}

TEST_CASE("T5: every HUD 目標 objective-text glyph is baked") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> objs = nccu::QuestObjectiveStrings();
    CHECK(objs.size() >= 9);          // sanity: every chapter/state enumerated
    for (const std::string& s : objs)
        RequireAllCovered(atlas, s, "objective");
}

TEST_CASE("T5: every ItemCatalog name + description glyph is baked") {
    const std::vector<int> atlas = CollectCodepoints();
    const std::vector<std::string> cat = nccu::CatalogStrings();
    CHECK(cat.size() > 10);           // sanity: the catalog table was flattened
    for (const std::string& s : cat)
        RequireAllCovered(atlas, s, "catalog");
}

TEST_CASE("T5: every Vendor toast / message glyph is baked") {
    const std::vector<int> atlas = CollectCodepoints();
    namespace m = nccu::vendor::msg;
    const std::vector<std::string> v = {
        std::string{m::kInsufficientFunds}, std::string{m::kPurchasedPrefix},
        std::string{m::kSpentMid},          std::string{m::kSpentUnitOpen},
        std::string{m::kSpentUnitClose},    std::string{m::kSoldOut},
        std::string{m::kStockLineSep},      std::string{m::kStockLineUnit},
    };
    for (const std::string& s : v)
        RequireAllCovered(atlas, s, "vendor-msg");
}
