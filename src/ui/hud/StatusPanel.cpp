#include "ui/hud/StatusPanel.h"

#include "world/World.h"
#include "entities/Player.h"
#include "ui/RainHud.h"                   // RainTierPrefix
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>

namespace nccu {

void DrawStatusPanel(nccu::gfx::IRenderer& r, const World& world) {
    using namespace nccu::gfx;

    // Top-left status: WASD hint, karma/umbrella, optional building,
    // chapter name, rain meter. Previously plain DarkGray/Blue text drawn
    // straight onto the bright worldmap — barely legible. Now it reuses
    // the proven objective-panel idiom (a translucent Color{20,22,30,185}
    // backing rect + bright text) so every value pops on any background.
    constexpr float kHudX    = 10.0f;
    constexpr float kHudY    = 10.0f;
    constexpr float kLineH   = 20.0f;
    constexpr int   kHudSize = 16;
    constexpr float kPad     = 6.0f;

    const Player* p = world.GetPlayer();
    char kbuf[64] = {0};
    if (p)
        std::snprintf(kbuf, sizeof(kbuf), "karma: %d   umbrella: %s",
            p->GetKarma(), p->HasUmbrella() ? "yes" : "no");
    // Money readout — the player must always see their purse during
    // play (feature 金幣 HUD). Read-only from World/Player data; the
    // View never mutates state (MVC purity). Updates live because the
    // HUD is redrawn every frame from GetMoney().
    char mbuf[48] = {0};
    if (p)
        std::snprintf(mbuf, sizeof(mbuf), "金幣: %d 元", p->GetMoney());
    const bool inside = !world.CurrentBuildingName().empty();
    const std::string insideLine =
        inside ? ("Inside: " + world.CurrentBuildingName())
               : std::string{};
    const std::string chapter{world.Semester().CurrentName()};
    // Audit D2 / SC 1.4.1: prefix a redundant tier glyph so the
    // rain readout's three pressure tiers are legible without
    // relying on the white→gold→red ramp alone (deutera/protan
    // viewers see gold and red as nearly identical olive/brown).
    // The colour ramp below is preserved as amplification.
    char rbuf[40] = {0};
    if (p) {
        const std::string_view tag =
            RainTierPrefix(p->GetRainMeter());
        std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
            static_cast<int>(tag.size()), tag.data(),
            static_cast<int>(p->GetRainMeter() + 0.5f));
    }

    // UI-B-2: control hints. The movement/pick-up line, plus a second
    // line surfacing the two overlay keys (Tab → 物品欄 inventory, M →
    // 選單 menu) in the top-left HUD where players look first. 物品欄 /
    // 選單 are baked into the font atlas (UiLiteralChars); ASCII ':' is
    // used (matching the WASD line) so no new glyph is needed. Named
    // locals so the width estimate and the draw use the SAME text.
    const std::string ctrlLine1 = "WASD: move    E: pick up";
    const std::string ctrlLine2 = "Tab: 物品欄   M: 選單";

    // Lines actually present (Inside is conditional). Width estimated
    // from UTF-8 lead-byte count like the objective panel below — the
    // chapter name is CJK so worst-case ~size px per glyph.
    int rows = 1;                         // WASD hint
    rows += 1;                            // UI-B-2 Tab/M control hint
    if (p)    rows += 1;                  // karma/umbrella
    if (p)    rows += 1;                  // 金幣 (money)
    if (inside) rows += 1;                // Inside
    rows += 1;                            // chapter
    if (p)    rows += 1;                  // rain
    auto glyphsOf = [](const std::string& s) {
        int g = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++g;
        return static_cast<std::size_t>(g);
    };
    std::size_t maxGlyphs = ctrlLine1.size();
    // UI-B-2: the Tab/M hint mixes ASCII + 4 full-width CJK (物品欄/選單).
    // Count its CJK glyphs ×2 (wide) plus the ASCII run so the black
    // backing widens to fit the longer hint list cleanly. glyphsOf
    // counts codepoints; add the CJK count again to approximate the
    // extra width the full-width cells take, like the chapter/金幣 rows.
    auto cjkGlyphsOf = [](const std::string& s) {
        int g = 0;  // lead bytes 0xE0.. = 3-byte CJK BMP this game uses
        for (unsigned char c : s) if ((c & 0xF0) == 0xE0) ++g;
        return static_cast<std::size_t>(g);
    };
    maxGlyphs = std::max(maxGlyphs,
                         glyphsOf(ctrlLine2) + cjkGlyphsOf(ctrlLine2));
    maxGlyphs = std::max(maxGlyphs, glyphsOf(kbuf));
    // 金幣 line is CJK (3 wide ideographs + digits) — count its
    // lead-byte glyphs ×2 like the chapter line so the panel is wide
    // enough on any money value.
    maxGlyphs = std::max(maxGlyphs, glyphsOf(mbuf) * 2);
    if (inside) maxGlyphs = std::max(maxGlyphs, glyphsOf(insideLine));
    maxGlyphs = std::max(maxGlyphs, glyphsOf(chapter) * 2);  // CJK wide
    maxGlyphs = std::max(maxGlyphs, glyphsOf(rbuf));
    const float panelW =
        static_cast<float>(maxGlyphs) * (kHudSize * 0.55f) + kPad * 2.0f;
    const float panelH =
        static_cast<float>(rows) * kLineH + kPad * 2.0f - 4.0f;
    r.DrawRect(Rect{kHudX - kPad, kHudY - kPad, panelW, panelH},
               Color{20, 22, 30, 185});

    float y = kHudY;
    TextBuilder{ctrlLine1}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::White).Draw();
    y += kLineH;
    // UI-B-2: the Tab/M control hint, a soft grey so it reads as a
    // secondary affordance under the primary WASD/E line.
    TextBuilder{ctrlLine2}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Color{200, 205, 215, 255}).Draw();
    y += kLineH;
    if (p) {
        TextBuilder{kbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::White).Draw();
        y += kLineH;
    }
    if (p) {
        // Gold so the purse reads at a glance and matches the coin
        // motif; stays on the dark panel so it pops on any map tile.
        TextBuilder{mbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
        y += kLineH;
    }
    if (inside) {
        TextBuilder{insideLine}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
        y += kLineH;
    }
    TextBuilder{chapter}
        .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::Gold).Draw();
    y += kLineH;
    if (p) {
        // Rain readout colour ramps with the meter so the rising risk
        // is legible at a glance (tiers mirror the vignette below).
        const float rm = p->GetRainMeter();
        const Color rc = rm >= 85.0f ? Colors::Red
                       : rm >= 60.0f ? Colors::Gold
                                     : Colors::White;
        TextBuilder{rbuf}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(rc).Draw();
        y += kLineH;
    }
}

}  // namespace nccu
