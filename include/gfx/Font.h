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
namespace nccu::gfx {

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
        // InventoryView panel
        "物品欄空";
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

} // namespace nccu::gfx

#endif // GFX_FONT_H_
