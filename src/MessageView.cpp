#include "MessageView.h"
#include "ReducedMotion.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

namespace {

// Banner text size and the layout pad around it.
constexpr int   kFontSize = 18;
constexpr float kPadX     = 18.0f;
constexpr float kPadY     = 12.0f;
constexpr float kLineH    = 24.0f;   // > kFontSize: a little leading
constexpr float kMarginB  = 28.0f;   // gap from the screen bottom

// Length in bytes of the UTF-8 sequence that starts with lead byte b.
// 1 for ASCII, 3 for the CJK BMP block this game's strings live in.
std::size_t Utf8Len(unsigned char b) noexcept {
    if (b < 0x80) return 1;
    if ((b >> 5) == 0x6) return 2;
    if ((b >> 4) == 0xE) return 3;
    if ((b >> 3) == 0x1E) return 4;
    return 1;  // invalid lead → treat as one byte, never loop forever
}

// Estimated pen advance for one glyph at kFontSize. raylib has no
// built-in word-wrap (issue #44 / #1992); the community idiom is
// "measure then break manually". IRenderer exposes no measure
// entrypoint and the sibling reactive views (DrawDialog /
// DrawEndingCard) never measure either — they place text at fixed
// coords. Rather than widen IRenderer for one bug, we estimate: a CJK
// glyph is roughly square (~font size), Latin/ASCII roughly half. Good
// enough to keep a banner readable; exact metrics are a GUI concern the
// user manual-verifies.
float GlyphAdvance(std::size_t bytes) noexcept {
    return (bytes >= 3) ? static_cast<float>(kFontSize)
                        : static_cast<float>(kFontSize) * 0.5f;
}

// Greedy width-budget wrap. CJK has no spaces, so we break between
// codepoints (never inside a multibyte sequence) once the running pen
// would overflow maxWidth. Honours any literal '\n' in the source.
// Pure: std::string → lines, no raylib, directly unit-testable.
std::vector<std::string> WrapCjk(const std::string& s, float maxWidth) {
    std::vector<std::string> lines;
    std::string line;
    float pen = 0.0f;
    for (std::size_t i = 0; i < s.size();) {
        if (s[i] == '\n') {
            lines.push_back(line);
            line.clear();
            pen = 0.0f;
            ++i;
            continue;
        }
        const std::size_t n =
            std::min(Utf8Len(static_cast<unsigned char>(s[i])),
                     s.size() - i);
        const float adv = GlyphAdvance(n);
        if (pen + adv > maxWidth && !line.empty()) {
            lines.push_back(line);
            line.clear();
            pen = 0.0f;
        }
        line.append(s, i, n);
        pen += adv;
        i += n;
    }
    lines.push_back(line);
    return lines;
}

}  // namespace

void DrawHudMessage(IRenderer& r, const std::string& message,
                    float age, float screenW, float screenH,
                    bool reducedMotion) {
    if (message.empty() || age >= kHudTtl) return;  // nothing to show

    // Lifetime → alpha. Hold opaque, then ramp 1→0 across the final
    // kHudFade seconds — the raylib-extras Timer idiom (a countdown
    // remaining = lifetime - elapsed, gone at <= 0) recast as a fade.
    // Audit D8 / SC 2.3.3: HudToastFadeT collapses the fade to a hard
    // cut when reducedMotion is true (holds opaque until TTL boundary,
    // then DrawHudMessage's early-return above hides the toast in
    // one frame).
    const float remaining = kHudTtl - age;
    const float t = HudToastFadeT(remaining, kHudFade, reducedMotion);
    const auto  a = static_cast<unsigned char>(t * 255.0f);

    const float maxTextW = screenW * 0.72f;
    const std::vector<std::string> lines = WrapCjk(message, maxTextW);

    // Banner box sized to the widest wrapped line, centred horizontally,
    // anchored kMarginB above the screen bottom.
    float widest = 0.0f;
    for (const std::string& ln : lines) {
        float w = 0.0f;
        for (std::size_t i = 0; i < ln.size();) {
            const std::size_t n =
                std::min(Utf8Len(static_cast<unsigned char>(ln[i])),
                         ln.size() - i);
            w += GlyphAdvance(n);
            i += n;
        }
        widest = std::max(widest, w);
    }

    const float boxW = widest + kPadX * 2.0f;
    const float boxH = static_cast<float>(lines.size()) * kLineH +
                       kPadY * 2.0f;
    const float boxX = (screenW - boxW) * 0.5f;
    const float boxY = screenH - kMarginB - boxH;

    // Backdrop carries the same alpha as the text so the whole toast
    // fades as one (mirrors DrawEndingCard's self-contained fade).
    r.DrawRect(Rect{boxX, boxY, boxW, boxH}, Color{20, 20, 20, a});
    r.DrawRect(Rect{boxX, boxY, boxW, 2.0f},
               Color{245, 245, 245, a});

    float y = boxY + kPadY;
    for (const std::string& ln : lines) {
        r.DrawText(ln, Vec2{boxX + kPadX, y}, kFontSize,
                   Color{245, 245, 245, a});
        y += kLineH;
    }
}

} // namespace nccu
