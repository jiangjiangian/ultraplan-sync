#ifndef CHARACTER_SELECT_H_
#define CHARACTER_SELECT_H_
#include "gfx/Window.h"
#include "gfx/Color.h"
#include <array>
#include <string>
#include <string_view>

namespace nccu {

// One selectable persona. Non-gendered campus archetypes (NCCU 政大山下
// student personas) — NOT a binary male/female pick. Each maps onto an
// already-shipped Pipoya sheet under resources/assets/sprites/
// school_uniform_3/ (RED LINE: no new binaries added to resources/);
// distinct personas that happen to share a base sheet are pulled apart
// at draw time by a runtime DrawTexturePro colour `tint` (raylib 5.5
// colour-modulate), so all five read as visually distinct without any
// new art. `tint` is also stamped onto the in-world Player sprite (the
// View tints the front object with the chosen persona's colour).
struct Persona {
    std::string_view label;     // CJK persona name shown in the picker
    std::string_view blurb;     // one-line flavour shown under the name
    std::string_view spritePath;// resource-relative Pipoya sheet
    gfx::Color        tint;      // draw-time colour modulate (RGBA)
};

// The five personas. Order is the menu order (index 0..4). Fixed at
// compile time so the harness can deterministically resolve a sprite
// path/tint from UMBRELLA_SPRITE without running the picker.
inline constexpr std::array<Persona, 5> kPersonas{{
    {"夜貓子", "通宵 K 書，黑眼圈是勳章",
     "resources/assets/sprites/school_uniform_3/female_03.png",
     gfx::Color{150, 170, 255, 255}},   // cool indigo
    {"social咖", "系上活動的開心果",
     "resources/assets/sprites/school_uniform_3/male_02.png",
     gfx::Color{255, 175, 90, 255}},    // warm orange
    {"邊緣人", "圖書館角落的常駐住民",
     "resources/assets/sprites/school_uniform_3/female_01.png",
     gfx::Color{170, 220, 180, 255}},   // muted green
    {"卷王", "GPA 4.3，行事曆排到深夜",
     "resources/assets/sprites/school_uniform_3/male_03.png",
     gfx::Color{255, 150, 170, 255}},   // rose
    {"佛系生", "隨緣修課，傘丟了也不急",
     "resources/assets/sprites/school_uniform_3/female_02.png",
     gfx::Color{210, 200, 120, 255}},   // amber
}};

struct CharacterSelectResult {
    // Resource-relative path of the chosen sprite sheet.
    // Empty when `closed` is true.
    std::string spritePath;

    // Draw-time colour modulate for the chosen persona's sprite (white
    // = no recolour). Carried into the World so the View tints the
    // player object — keeps the 5 personas visually distinct without
    // committing new sprite binaries.
    gfx::Color tint{255, 255, 255, 255};

    // True if the user closed the window mid-selection, OR pressed the
    // back/Esc affordance to return to the title screen. The caller
    // (main.cpp) treats `closed` as "go back / do not start a run".
    bool closed{false};
};

// Pre-gameplay screen: pick one of five non-gendered campus personas.
// Keyboard-navigable (Up/Down/Left/Right + Enter; Esc backs out to the
// title). Runs its own draw loop on `win` and blocks until the player
// either confirms a persona or backs out / closes the window. Loads the
// five persona preview sheets directly from resources/.
//
// HARNESS: this is NEVER called when the autoplay harness is active —
// main.cpp bypasses it (and the title screen) exactly as the old
// character-select was bypassed, honouring UMBRELLA_SPRITE — so every
// .claude/scripts/* playtest still falls straight into gameplay with a
// deterministic, byte-identical state.jsonl.
CharacterSelectResult RunCharacterSelect(gfx::Window& win);

} // namespace nccu

#endif // CHARACTER_SELECT_H_
