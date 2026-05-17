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

// Bundled-first, then per-OS system CJK fonts. The first path that exists
// AND loads with a non-zero glyph set wins. To ship a portable graded
// build, drop a .ttf/.otf/.ttc into resources/assets/fonts/ — it is tried
// before any system font. resources/ is user-managed and untracked, so no
// binary font is committed; the macOS system-font fallback below makes the
// game readable immediately on the developer's machine.
inline std::string ResolveFontPath() {
    // resources/assets/fonts/ is preferred but its filename is unknown;
    // probe a few conventional names rather than scanning the dir (keeps
    // this header free of <filesystem>).
    static const char* kCandidates[] = {
        "resources/assets/fonts/cjk.ttf",
        "resources/assets/fonts/cjk.otf",
        "resources/assets/fonts/cjk.ttc",
        "resources/assets/fonts/font.ttf",
        "resources/assets/fonts/font.otf",
        "resources/assets/fonts/font.ttc",
        // macOS system CJK faces.
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/Library/Fonts/Songti.ttc",
        // Linux (Noto CJK) fallbacks.
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJKtc-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
    };
    for (const char* c : kCandidates) {
        if (::FileExists(c)) return std::string{c};
    }
    return std::string{};
}

// The CJK characters baked into the four UI .cpp files' string literals
// (View.cpp / DialogView.cpp / EndingView.cpp / InventoryView.cpp). Kept
// as a compact static list so the font works even if docs/content is
// unreadable. UTF-8; decoded via raylib's own codepoint loader.
inline const char* UiLiteralChars() {
    return
        // EndingView caption + placeholder
        "這樣以後再也不會有人拿錯你的傘了結局字卡待接入"
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

    const std::string path = detail::ResolveFontPath();
    if (path.empty()) return;         // no CJK font anywhere → fallback

    std::vector<int> cps = detail::CollectCodepoints();
    constexpr int kFontSize = 32;     // rasterize big; DrawTextEx scales down
    ::Font f = ::LoadFontEx(path.c_str(), kFontSize,
                            cps.data(), static_cast<int>(cps.size()));
    if (f.texture.id == 0 || f.glyphCount == 0) {
        ::UnloadFont(f);              // failed load → keep default font
        return;
    }
    ::SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    s.font   = f;
    s.loaded = true;
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
