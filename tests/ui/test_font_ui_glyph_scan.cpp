#include "doctest/doctest.h"
#include "gfx/Font.h"
#include "ui/EndingView.h"
#include "ui/GameHelp.h"
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
// Headless-safe: CollectCodepoints() + raylib's codepoint decoder need
// no GL context (same as the sibling font test).

using nccu::gfx::detail::CollectCodepoints;

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

TEST_CASE("5c: every 遊戲說明 (GameHelp) glyph is baked into the atlas") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string_view ln : nccu::kGameHelpLines)
        RequireAllCovered(atlas, std::string{ln}, "help-line");
    RequireAllCovered(atlas, std::string{nccu::kGameHelpClosing},
                      "help-closing");
}

TEST_CASE("5c: every View.cpp HUD/menu/inventory literal glyph is baked") {
    const std::vector<int> atlas = CollectCodepoints();
    for (const std::string& s : ViewLiterals())
        RequireAllCovered(atlas, s, "view-literal");
}
