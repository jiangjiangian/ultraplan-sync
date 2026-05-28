#ifndef GFX_DECORATIONS_H_
#define GFX_DECORATIONS_H_
#include "gfx/SpriteStrip.h"
#include "engine/math/Vec2.h"
#include "quest/Chapter3Quest.h"   // kSportsTrackCx/Cy — 操場 centre
#include "state/SemesterState.h"
#include <array>

// The placed AMBIENT decorations — purely cosmetic, no gameplay effect.
// View-side data (NOT GameObjects, NOT in World::Objects()): the View
// loads each strip texture in its ctor and draws the def from the render
// clock, so the simulation and the harness state.jsonl never see them
// (the harness serialises World::Objects(), which these are absent from —
// the run is byte-identical with or without the art). Each is shown only
// while the semester FSM is in its `chapter`, and draws nothing at all if
// its strip PNG is missing (the empty-resources path) — exactly like
// every other asset in this project.
//
// Both are third-party fan art credited in CREDITS.md; the binaries are
// user-managed and must not be committed (CLAUDE.md §5). Drop the
// converted strips (see tools/gif_to_strip.py) at the stripPath below.

namespace nccu::gfx {

// 羅馬廣場 plaza. The Ch2 roster comment in include/quest/ChapterSpawns.h
// pins it: "學霸 (bookworm) sits under the 羅馬廣場 statue (plaza centre
// ~1088,960; placed at the south rim 1088,1100)". The bookworm NPC is
// parked at the south RIM (1088,1100) so the player can walk up and talk.
//
// A-T2: the chiikawa "statue" was at the plaza geometric CENTRE (1088,960),
// a full 140 px ABOVE the 學霸 — so on screen it read as a separate object
// floating north of him, not the statue he is slumped under. Bring it DOWN
// toward the bookworm (1088,1040: 60 px above his 1088,1100 post) so the
// 24 px player + the seated 學霸 + the ~80 px statue read as one grouped
// "he's resting under the chiikawa monument" tableau (owner: 把吉伊卡哇移
// 近學霸). Still purely cosmetic — never a GameObject, never collides, never
// in state.jsonl (the harness serialises World::Objects(), not this).
inline constexpr Vec2 kRomaPlazaStatue{1088.0f, 1040.0f};

// A-T2: the Ch3 cat's drawn CENTRE. The 操場 field is rect (1384,541,
// 621x399) but the 綜合院館 building (rect 1681,677,371x326) OVERLAPS the
// field's EAST half — so the cat at the geometric track centre
// (kSportsTrackCx=1694, kSportsTrackCy=740) sat INSIDE the 綜院 footprint
// and the building sprite painted OVER it ("放左邊一點才看的到" — the owner
// couldn't see it). Move it WEST of the 綜院's left edge (1681) onto the
// clearly-open western field (x1530, mask-verified strictly walkable +
// well inside the 1384–2005 field), keeping the same track row (y740) so
// it still reads as a cat on the 操場. No longer occluded by 綜院.
inline constexpr Vec2 kSportsCatPos{1530.0f, kSportsTrackCy};

// The decoration table. Indexed only by the View; order is irrelevant.
//   chiikawa — Ch2 期中考 (Chapter2_Midterms): a pulsing "statue" at the
//       羅馬廣場 centre, ~80px so it reads as a plaza monument (the 24px
//       player stands much smaller beside it); the ping-pong zoom gives
//       the gentle 放大縮小 breathing the owner asked for.
//   cat — Ch3 校慶 (Chapter3_SportsDay): a SMALL cat (~28px — the owner's
//       「小小的一隻，不要太大隻」) at the 操場 centre (kSportsTrackCx/Cy,
//       the same field centre the lap-progress ring is measured around),
//       breathing via the same ping-pong.
inline constexpr std::array<DecorationDef, 2> kDecorations{{
    DecorationDef{SemesterState::Chapter2_Midterms, kRomaPlazaStatue,
                  "resources/assets/decorations/chiikawa_strip.png",
                  /*frameCount=*/17, /*drawScale=*/80.0f, /*fps=*/6.0},
    DecorationDef{SemesterState::Chapter3_SportsDay, kSportsCatPos,
                  "resources/assets/decorations/cat_strip.png",
                  /*frameCount=*/24, /*drawScale=*/28.0f, /*fps=*/8.0},
}};

} // namespace nccu::gfx

#endif // GFX_DECORATIONS_H_
