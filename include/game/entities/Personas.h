#ifndef GAME_ENTITIES_PERSONAS_H_
#define GAME_ENTITIES_PERSONAS_H_
#include "engine/math/Color.h"
#include <array>
#include <string>
#include <string_view>

namespace nccu {

// Blueprint Phase 4 — game-domain persona data, moved out of
// ui/CharacterSelect.h. The persona definitions (label / blurb /
// spritePath / tint) and the per-run selection result are MODEL data,
// not UI presentation — the asset-warming pass (TexturePreload),
// GameplayScene's persona-tint apply, and the harness skip path all
// consume them. ui/CharacterSelect.h still re-exports them via a
// transitive include for source-compat.

// One selectable persona. Non-gendered campus archetypes (NCCU 政大山下
// student personas) — NOT a binary male/female pick. Each maps onto
// an already-shipped Pipoya sheet under resources/assets/sprites/
// school_uniform_3/ (RED LINE: no new binaries added to resources/);
// distinct personas that happen to share a base sheet are pulled
// apart at draw time by a runtime DrawTexturePro colour `tint`
// (raylib 5.5 colour-modulate), so all five read as visually distinct
// without any new art. `tint` is also stamped onto the in-world
// Player sprite (the View tints the front object with the chosen
// persona's colour).
struct Persona {
    std::string_view label;     // CJK persona name shown in the picker
    std::string_view blurb;     // one-line flavour shown under the name
    std::string_view spritePath;// resource-relative Pipoya sheet
    nccu::engine::math::Color        tint;      // draw-time colour modulate (RGBA)
};

// The five personas. Order is the menu order (index 0..4). Fixed at
// compile time so the harness can deterministically resolve a sprite
// path/tint from UMBRELLA_SPRITE without running the picker.
inline constexpr std::array<Persona, 5> kPersonas{{
    {"夜貓子", "通宵 K 書，黑眼圈是勳章",
     "resources/assets/sprites/school_uniform_3/female_03.png",
     nccu::engine::math::Color{150, 170, 255, 255}},   // cool indigo
    {"social咖", "系上活動的開心果",
     "resources/assets/sprites/school_uniform_3/male_02.png",
     nccu::engine::math::Color{255, 175, 90, 255}},    // warm orange
    {"邊緣人", "圖書館角落的常駐住民",
     "resources/assets/sprites/school_uniform_3/female_01.png",
     nccu::engine::math::Color{170, 220, 180, 255}},   // muted green
    {"卷王", "GPA 4.3，行事曆排到深夜",
     "resources/assets/sprites/school_uniform_3/male_03.png",
     nccu::engine::math::Color{255, 150, 170, 255}},   // rose
    {"佛系生", "隨緣修課，傘丟了也不急",
     "resources/assets/sprites/school_uniform_3/female_02.png",
     nccu::engine::math::Color{210, 200, 120, 255}},   // amber
}};

// Per-run persona selection — the value the CharacterSelectScene
// confirms and the GameplayScene consumes. The harness skip path
// constructs one directly from UMBRELLA_SPRITE.
struct CharacterSelectResult {
    std::string spritePath;
    nccu::engine::math::Color  tint{255, 255, 255, 255};
    bool        closed{false};
};

} // namespace nccu

#endif // GAME_ENTITIES_PERSONAS_H_
