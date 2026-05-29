#ifndef GFX_FONT_H_
#define GFX_FONT_H_
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

// Process-wide CJK font manager. raylib's built-in default font is
// ASCII-only, so every Chinese string (HUD / dialog / ending / inventory)
// rendered through the gfx text layer came out as `?`/boxes. This header
// loads ONE font with the full set of codepoints the game actually
// displays, and exposes it for DrawTextEx. raylib.h is confined here, in
// keeping with the gfx-layer invariant.
namespace nccu::engine::render {

namespace detail {

// Ordered CJK font candidates. raylib's LoadFontData hardcodes
// stbtt_InitFont(..., 0): no .ttc collection-index selection, and on a
// font whose outlines its bundled stb_truetype cannot rasterize (CFF /
// PostScript — which PingFang and Hiragino use) it returns glyphCount==0
// and silently falls back to the ASCII-only default → every Chinese
// glyph renders as `?`. (Verified against raylib master src/rtext.c.)
// So a single fixed path is fragile: EnsureFont must TRY each candidate
// and keep the first that actually yields glyphCount>0.
//
// Order: a user-supplied bundled font first (drop a real .ttf at
// resources/assets/fonts/cjk.ttf for a guaranteed, parser-friendly face
// — resources/ is user-managed/untracked so no binary font is
// committed); then macOS faces best-effort (TrueType-outline ones before
// the CFF .ttc, so whichever the local stb_truetype can parse wins);
// then Linux Noto.
inline const std::vector<std::string>& FontCandidates() {
    static const std::vector<std::string> kCandidates = {
        "resources/assets/fonts/cjk.ttf",
        "resources/assets/fonts/cjk.otf",
        "resources/assets/fonts/cjk.ttc",
        "resources/assets/fonts/font.ttf",
        "resources/assets/fonts/font.otf",
        "resources/assets/fonts/font.ttc",
        // macOS — try the .ttf / TrueType-outline faces before the
        // CFF-outline .ttc (PingFang/Hiragino) which stb_truetype often
        // cannot rasterize. None is guaranteed present; the loop skips
        // missing ones and validates glyphCount>0 on the rest.
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/Library/Fonts/Songti.ttc",
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        // Linux (Noto CJK).
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJKtc-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
    };
    return kCandidates;
}

// The CJK characters baked into the four UI .cpp files' string literals
// (View.cpp / DialogView.cpp / EndingView.cpp / InventoryView.cpp). Kept
// as a compact static list so the font works even if docs/content is
// unreadable. UTF-8; decoded via raylib's own codepoint loader.
inline const char* UiLiteralChars() {
    return
        // EndingView captions (A 真相大白 / B 屠龍者終成惡龍 / C 破財消災)
        // — every glyph kept here so the cards still render correctly even
        // on the content-unreadable fallback path (some, e.g. 討/厭, do
        // not occur in docs/content/*.md at all).
        "雨過天晴傘還在你手上"
        "你成為了曾經最討厭的那種人"
        "這樣以後再也不會有拿錯的了"
        // Punctuation used by the UI literals above (CJK quotes / comma /
        // full stop) — full-width, atlas-collected like the ideographs.
        "「」，。"
        // DialogView B4 pagination affordance: U+25BC ▼ "more" cue. Not an
        // ASCII char and absent from every content .md, so without this it
        // renders as the raylib no-glyph `?` (the V1 tofu bug). DialogView
        // uses only the down-cue; no ▲ up-cue exists, so none is added.
        "\xE2\x96\xBC"  // U+25BC ▼
        // U+2190 ← / U+2192 → / U+2191 ↑ / U+2193 ↓ — arrow affordances
        // used by the title, character-select and in-game menu hint
        // lines. Not ASCII, absent from docs/content → tofu without this.
        "\xE2\x86\x90\xE2\x86\x92\xE2\x86\x91\xE2\x86\x93"
        // Title screen / in-game menu / 5-persona character-select string
        // literals (TitleScreen.cpp / CharacterSelect.cpp / View.cpp menu
        // overlay). Kept here so every new UI glyph still renders on the
        // content-unreadable fallback path, exactly like the ending
        // captions above. Game title + menu verbs + persona names/blurbs.
        "尋傘記政大山下篇開始遊戲離開選擇你的角色"
        "五位學生沒性別之分確認返首頁繼續重新動"
        "夜貓通宵K書黑眼圈是勳章social咖系活開心果"
        "邊緣人圖館落常駐民卷王GPA行事曆排深佛隨修"
        "課丟急金幣元遊戲選單"
        // InventoryView panel
        "物品欄空"
        // REQUIREMENT #10: every glyph used by a building name in
        // include/Buildings.h kAll. View.cpp renders "Inside: " +
        // World::CurrentBuildingName() (the building HUD line); any
        // building-name ideograph absent from BOTH docs/content/*.md AND
        // this literal set falls to raylib's no-glyph `?` — the reported
        // "建築物名字出現 ? 缺字" defect (井/仁/勇/塘/夫/志/泳/雩 were
        // missing). The FULL 56-glyph building-name set is baked here (not
        // just the 8 currently-missing) so a future Buildings.h rename
        // cannot silently reintroduce a tofu building name. Same atlas
        // mechanism / rationale as the U+25BC ▼ fix (BUGLEDGER V1) and
        // the ending-caption block above. Kept on the content-unreadable
        // fallback path too, exactly like those.
        "中井仁勇務友合商四圖堂場塘大夫學小希廊心志思操政新智"
        "書服果校樂樓正法泳活游研究綜維聞育舖英行訊資走門院集雩風館體"
        // REQUIREMENT #9: every glyph in the shared 遊戲說明 help text
        // (include/GameHelp.h kGameHelpLines + kGameHelpClosing), shown
        // on the title-screen help page AND the in-game 說明 overlay.
        // Most already appear in docs/content/*.md; 訪 (拜訪) does not, so
        // without this the help panel would tofu it to `?` — same atlas
        // mechanism / #10 lesson as the building-name block above. The
        // FULL help glyph set is baked (not just 訪) so future help-copy
        // edits cannot silently reintroduce a tofu glyph. Cycle 9.E
        // (audit H3) appended 暫停凍壓力計 for the menu (M) pause-freezes-
        // rain hint line; 結 already in 終, 力/計 not in the old set.
        "三下不並久了他作偷動取向品善局屠屬建待復惡或戲戶把拿"
        "撐撿標欄沖消淋災物相破種積終綠者花術被訪購躲透進選醜量錢鍵龍"
        "暫停凍結壓力計"
        // Cycle 9.E.3 pause-menu accessibility toggles ("減少動畫 [開/關]"
        // / "擴大目標 [開/關]"). 動/標/開 already baked above; 減/少/畫/
        // 擴/目/關 added here so the toggle rows render correctly on the
        // content-unreadable fallback path too (every glyph in those two
        // rows must reach the atlas — same V1/REQ#10 rationale as the
        // building-name / help blocks).
        "減少畫擴目關"
        // A-T3 ending-screen 3-option menu (EndingView): 回首頁 / 重新開始 /
        // 結束 + the "← → 選擇   E 確認" nav hint. Most glyphs (首/頁/重/新/
        // 開/始/結/選/擇/確/認 + the arrows) are already baked above; only
        // 回 (回首頁) and 束 (結束) are new and appear in NO docs/content/*.md,
        // so they reach the atlas ONLY here. The glyph-scan test
        // (test_font_ui_glyph_scan.cpp scans EndingCardStrings(), which now
        // includes these menu labels) FAILS the build on any uncovered glyph,
        // so this is verified, not guessed — same V1/REQ#10 mechanism.
        "回束"
        // Item 1 / 1e: the enriched ending SCREEN (EndingView.cpp) and the
        // actionable 遊戲說明 endings section (GameHelp.h) draw a handful of
        // glyphs that occur in NO docs/content/*.md file, so they reach the
        // atlas ONLY here (same V1/REQ#10 mechanism). The 5c glyph-scan test
        // (tests/ui/test_font_ui_glyph_scan.cpp) enumerates EndingCardStrings
        // + kGameHelpLines + the View literals and FAILS if any glyph is
        // absent, so this block is verified, not guessed. The three that
        // were missing (everything else was already covered by the broad
        // content/UI sets):
        //   U+2500 ─  the "── 為什麼你走到這裡 ──" reason-section rule.
        //   U+2713 ✓  the 結算 checklist "condition met" mark.
        //   U+58AE 墮 only in Ending B's path label 「墮落結局」.
        "\xE2\x94\x80"      // U+2500 ─
        "\xE2\x9C\x93"      // U+2713 ✓
        "\xE5\xA2\xAE"      // U+58AE 墮
        // T5: the broadened glyph-scan (tests/ui/test_font_ui_glyph_scan.cpp)
        // now covers EVERY code-built UI string — the HUD 目標 objective text,
        // the ItemCatalog names+descriptions, the Vendor toast pieces, the new
        // 【雨傘外觀】 help lines and the ending strings. These few glyphs occur
        // in those code literals but in NO docs/content/*.md, so they reach the
        // atlas ONLY here (same V1/REQ#10 mechanism); the scan verifies it.
        //   北 objective 地圖東北   垂 help 傘面下垂   紫 help 暗紫色
        //   幸 catalog 小確幸       血 catalog 三頁心血
        "北垂紫幸血"
        // G1/G4: glyphs newly used by the Ending D card (EndingView), the
        // G4 ItemCatalog effect descriptions (CatalogStrings) and the Ch4
        // ending 自白 dialogs — and present in NO docs/content/*.md, so they
        // reach the atlas ONLY here. The 5c/T5 glyph-scan test verifies the
        // EndingCard + Catalog surfaces; the 自白 glyphs are baked alongside
        // so the in-game DialogView renders them too (no tofu either path).
        //   U+2212 −  the typographic minus in "雨量 −15/−25/−35" descriptions
        //   磨 Ending D「把傘磨破了」   壯 自白「骨架紮實」(紮 already covered)
        //   握 自白「你握著…」「你的手指扣上」  彈/擋 catalog「彈開」「擋雨」
        //   鳴 自白「發出細微的嗡鳴」
        "\xE2\x88\x92"      // U+2212 −
        "磨壯握彈擋鳴"
        // U2-T4 (owner item 6): the new 【道具須知】 help-tips section
        // (GameHelp.h kGameHelpPage2) adds these glyphs. Most ALSO appear in
        // docs/content, but — per the building-name / #10 lesson above — the
        // FULL new-tips glyph set is baked here so the help text renders even
        // on the content-unreadable fallback path AND a future copy-edit
        // can't silently reintroduce a tofu glyph. The two fullwidth puncts
        // need explicit UTF-8 (！ U+FF01 / ； U+FF1B); the rest are ideographs.
        // The U2-T4 glyph-scan (test_font_ui_glyph_scan.cpp) enumerates
        // kGameHelpPages and FAILS the build on any uncovered glyph, so this
        // block is verified, not guessed.
        "跨節保留其餘道具只該效市清當耗多數可緩得用必使頭冒就接來去業默決定"
        "\xEF\xBC\x81"      // U+FF01 ！
        "\xEF\xBC\x9B"      // U+FF1B ；
        // UI-B-1: the PERMANENT no-tofu gate. test_font_ui_literal_scan.cpp
        // scans EVERY CJK codepoint in EVERY "…" string literal across src/ +
        // include/ AND every docs/content/*.md, and FAILS the build unless each
        // is in the effective atlas — and unless every source-literal-only
        // glyph (one in NO .md, so the content-load path can't cover it on a
        // fresh clone) is in THIS set specifically. That scan surfaced these
        // glyphs as the ONLY source-literal-only ones not already baked above:
        //   敬 — the DLC teaser ShowMessage 「DLC開發中 / 敬請期待」 (DlcSign.cpp);
        //        the reported 缺字→`?` the owner kept hitting (whack-a-mole).
        //   刺 — vendor greeting 「螢光綠到刺眼」 (ChapterVendors.cpp).
        //   君 — item pickup 「A 君的名字貼紙」 (ChapterQuestItems.h).
        //   含/扶/毫 — DialogOpener.cpp narration branches.
        //   央 — a centred-toast helper string (ChapterToast.h).
        //   櫃 — 置物櫃 dialogue (Chapter2Quest.cpp / DialogOpener.cpp).
        //   牽/羊 — CursedUmbrella.cpp pickup line 「順手牽羊」.
        // The two CJK curly quotes 「“」「”」 (U+201C/D) are parser tokens in
        // DialogLoader/VendorLoader (stripped before display, so they don't
        // actually render) — baked anyway so the all-literal scan is complete
        // and a future direct render of them can't tofu either.
        "敬刺君含扶毫央櫃牽羊"
        "\xE2\x80\x9C"      // U+201C "
        "\xE2\x80\x9D"      // U+201D "
        // UI-C-2: the human-only 載入畫面 (src/ui/LoadingScreen.cpp) labels
        // 「載入中…」 / 「正在準備政大山下的雨天…」. Every glyph but 載 (U+8F09)
        // is already covered by docs/content / the blocks above; 載 occurs in
        // NO docs/content/*.md, so it reaches the atlas ONLY here (the
        // all-literal UI-B-1 scan, test_font_ui_literal_scan.cpp, FAILS the
        // build otherwise — same V1/REQ#10 mechanism). Baked here so the
        // loading label renders even on the content-unreadable fallback path.
        "載";
}

// Collect distinct codepoints: ASCII 32..126 always, then every codepoint
// in every docs/content/*.md file, then the hardcoded UI literals. If the
// content directory cannot be read, a broad common-CJK block is added so
// text still mostly renders.
inline std::vector<int> CollectCodepoints() {
    std::set<int> cps;
    for (int c = 32; c <= 126; ++c) cps.insert(c);

    auto add_utf8 = [&cps](const char* utf8) {
        if (!utf8 || utf8[0] == '\0') return;
        int count = 0;
        int* decoded = ::LoadCodepoints(utf8, &count);
        for (int i = 0; i < count; ++i) {
            if (decoded[i] > 0) cps.insert(decoded[i]);
        }
        ::UnloadCodepoints(decoded);
    };

    static const char* kContentFiles[] = {
        "chapter1.md", "chapter2.md", "chapter3.md", "chapter4.md",
        "ending_a.md", "ending_b.md", "ending_c.md",
        "interlude_market.md", "voice_bible.md",
    };
    bool any_content = false;
    // The game runs from the project root (View.cpp uses the same relative
    // base); try a parent prefix too as belt-and-suspenders.
    static const char* kBases[] = {"docs/content/", "../docs/content/"};
    for (const char* base : kBases) {
        for (const char* name : kContentFiles) {
            std::string path = std::string{base} + name;
            if (!::FileExists(path.c_str())) continue;
            char* text = ::LoadFileText(path.c_str());
            if (!text) continue;
            add_utf8(text);
            ::UnloadFileText(text);
            any_content = true;
        }
        if (any_content) break;
    }

    add_utf8(UiLiteralChars());

    if (!any_content) {
        // Content unreadable: bake a broad common-CJK block so the bulk
        // of Traditional Chinese still renders. CJK Unified Ideographs
        // (U+4E00..U+9FFF) + common punctuation already covered by the
        // literal/ASCII sets above.
        for (int c = 0x4E00; c <= 0x9FFF; ++c) cps.insert(c);
        for (int c = 0x3000; c <= 0x303F; ++c) cps.insert(c);  // CJK punct
        for (int c = 0xFF00; c <= 0xFFEF; ++c) cps.insert(c);  // fullwidth
    }

    std::vector<int> out(cps.begin(), cps.end());
    // Defensive cap: a runaway set would blow up the glyph atlas.
    constexpr std::size_t kMaxCodepoints = 16384;
    if (out.size() > kMaxCodepoints) out.resize(kMaxCodepoints);
    return out;
}

// Holds the loaded font + validity. A function-local static of this type
// would destruct AFTER main() returns — i.e. after the Window dtor has
// already called ::CloseWindow() and torn down the GL context — and
// UnloadFont touches GL. So teardown is explicit (ShutdownFont, called
// while the window is still alive), never via a static dtor.
struct FontState {
    ::Font font{};
    bool   attempted{false};
    bool   loaded{false};
};

inline FontState& State() {
    static FontState s;
    return s;
}

} // namespace detail

// Loads the CJK font once. MUST be called after InitWindow (raylib needs
// a GL context for the glyph atlas texture) and before the first text
// draw. Safe to call repeatedly (loads at most once). No-op without a
// ready window — this is what keeps the headless test build green: there
// is no GL context under tests, so the font never loads and text falls
// back to the raylib default (`?`), which is expected and harmless there.
inline void EnsureFont() {
    detail::FontState& s = detail::State();
    if (s.attempted) return;
    if (!::IsWindowReady()) return;   // headless / pre-InitWindow guard
    s.attempted = true;

    std::vector<int> cps = detail::CollectCodepoints();
    constexpr int kFontSize = 32;     // rasterize big; DrawTextEx scales down

    // Try every candidate that exists on disk; keep the FIRST that loads
    // with real glyphs. A CFF .ttc raylib/stb_truetype cannot rasterize
    // comes back glyphCount==0 — skip it and fall through instead of
    // giving up (the bug that left Chinese as `?` even though a usable
    // face existed further down the list).
    for (const std::string& path : detail::FontCandidates()) {
        if (!::FileExists(path.c_str())) continue;
        ::Font f = ::LoadFontEx(path.c_str(), kFontSize,
                                cps.data(), static_cast<int>(cps.size()));
        if (f.texture.id == 0 || f.glyphCount == 0) {
            ::UnloadFont(f);          // unparseable face → try the next
            continue;
        }
        ::SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        s.font   = f;
        s.loaded = true;
        return;
    }
    // Nothing parseable anywhere → keep the ASCII default. Drop a real
    // .ttf at resources/assets/fonts/cjk.ttf to guarantee CJK rendering.
}

inline bool IsCJKFontLoaded() { return detail::State().loaded; }

// Valid only when IsCJKFontLoaded() is true.
inline const ::Font& CJKFont() { return detail::State().font; }

// Explicit teardown. Call from main() BEFORE the window closes (while the
// GL context is still alive). Idempotent.
inline void ShutdownFont() {
    detail::FontState& s = detail::State();
    if (s.loaded) {
        ::UnloadFont(s.font);
        s.loaded = false;
    }
}

} // namespace nccu::engine::render

#endif // GFX_FONT_H_
