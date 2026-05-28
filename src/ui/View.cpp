#include "ui/View.h"
#include "world/World.h"
#include "entities/Player.h"
#include "entities/GameObject.h"
#include "controller/GameObjectQueries.h"
#include "world/Buildings.h"
#include "world/Obstacles.h"
#include "world/WorldConfig.h"
#include "dialog/DialogView.h"
#include "ui/EndingView.h"
#include "ui/ChapterCard.h"
#include "ui/InventoryView.h"
#include "ui/MessageView.h"
#include "quest/QuestObjective.h"
#include "quest/QuestIndicator.h"
#include "quest/Chapter3Quest.h"   // 操場 track-ring geometry (kSportsTrack*)
#include "ui/QuestGiverIndicator.h"
#include "state/InterludeExitMarker.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"  // shared 遊戲說明 page renderer (de-dup with TitleScreen)
#include "ui/RainHud.h"
#include "ui/ReducedMotion.h"
#include "engine/render/Renderer.h"
#include "engine/platform/Time.h"
#include "gfx/SpriteStrip.h"     // FrameAt / StripSourceRect / DecorationDestRect
#include "gfx/Decorations.h"     // kDecorations — the placed ambient strips
#include "engine/render/CameraScope.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace nccu {

// Buildings are drawn as separate sprites over a terrain-only base so a
// character standing above a building's ground line is occluded by it
// (walk-behind). The base map carries the open-ground features (running
// track, plaza), so the two named in kBuildingCollisionSkip get no
// sprite — their pixels already live in worldmap_base.png.
View::View(int windowWidth, int windowHeight)
    : worldmap_(nccu::gfx::Texture::Load("resources/assets/maps/worldmap_base.png")),
      screenCenter_{windowWidth / 2.0f, windowHeight / 2.0f},
      worldSize_{::world::kSize, ::world::kSize},
      viewportSize_{static_cast<float>(windowWidth),
                    static_cast<float>(windowHeight)} {
    const auto& skip = obstacles::kBuildingCollisionSkip;
    buildingTextures_.reserve(buildings::kAll.size());
    buildings_.reserve(buildings::kAll.size());
    for (const auto& b : buildings::kAll) {
        if (std::find(skip.begin(), skip.end(), b.name) != skip.end()) continue;
        std::string path = "resources/assets/buildings_3d_trimmed/";
        path += std::string(b.name);
        path += ".png";
        auto tex = nccu::gfx::Texture::Load(path);
        if (!tex.IsValid()) continue;  // art not generated yet → no sprite
        const std::size_t idx = buildingTextures_.size();
        buildingTextures_.push_back(std::move(tex));
        buildings_.push_back(BuildingSprite{
            idx, b.triggerRect,
            b.triggerRect.y + b.triggerRect.height,
            b.flipX, b.flipY});
    }

    // Ambient decoration strips (cosmetic, no gameplay effect). Load each
    // strip's texture ONCE here; a def whose PNG is missing (the empty-
    // resources path) yields !IsValid() and is skipped — so the missing-
    // asset case simply has no DecorationSprite to draw, no crash. Loaded
    // after InitWindow (the View is built once the window/GL context is
    // live) and torn down with the View, so the RAII-vs-GL order holds
    // (raylib-core KB). These never enter World::Objects(), so the harness
    // is unaffected.
    decorations_.reserve(nccu::gfx::kDecorations.size());
    for (std::size_t i = 0; i < nccu::gfx::kDecorations.size(); ++i) {
        auto tex = nccu::gfx::Texture::Load(nccu::gfx::kDecorations[i].stripPath);
        if (!tex.IsValid()) continue;   // art not dropped in → no sprite
        decorations_.push_back(DecorationSprite{i, std::move(tex)});
    }
}

// Draw() is the dispatcher — chapter card transition first (runs every
// frame), then split: endings replace the world, otherwise the world is
// drawn under the follow camera, then the screen-space HUD, then every
// overlay. Each helper owns ONE concern; see the doc block in View.h.
void View::Draw(const World& world) {
    const SemesterState st = world.Semester().Current();

    UpdateChapterCardTransition(st);

    if (IsEndingState(st)) {
        RenderEnding(world, st);
        return;   // ending replaces the world; agency is the menu drawn there
    }

    if (const Player* p = world.GetPlayer()) {
        camera_.Follow(p->GetPosition(), screenCenter_)
               .ClampToWorld(worldSize_, viewportSize_);
    }

    RenderWorld(world, st);
    RenderHud(world, st);
    RenderOverlays(world);
}

// U1-T2: detect a chapter-boundary FSM transition and arm the big bookend
// card. Done BEFORE the ending early-return so lastSemester_ stays current
// across every state (a Ch4 → Ending hop arms NOTHING —
// ChapterCardForTransition returns None for an ending dest — so the
// EndingView owns that beat). Pure render-side: the trigger reads the World
// snapshot the View already has; no event, no publish, no model write, so
// state.jsonl is byte-identical. The card rides the deferred-transition
// gap for the 找到傘了 beat: a chapter only flips to the market once its
// closing narration dialog has closed (LiftChapterXClear / the gate), so
// the card naturally appears after the reclaim scene.
void View::UpdateChapterCardTransition(SemesterState st) {
    if (!lastSemester_.has_value()) {
        // First paint of this run: treat the World's opening state as a
        // fresh entry so the inciting 傘，不見了 card fires for Chapter 1
        // (or a UMBRELLA_START_STATE warp's chapter). from == the same
        // state would be a no-op, so seed `from` as a non-chapter sentinel.
        const ChapterCardKind k =
            ChapterCardForTransition(SemesterState::Ending_C, st);
        if (k != ChapterCardKind::None)
            chapterCard_.Trigger(k, ChapterCardHeadline(k, st),
                                  ChapterCardSubtitle(k, st));
        lastSemester_ = st;
    } else if (*lastSemester_ != st) {
        const ChapterCardKind k = ChapterCardForTransition(*lastSemester_, st);
        if (k != ChapterCardKind::None)
            chapterCard_.Trigger(k, ChapterCardHeadline(k, st),
                                  ChapterCardSubtitle(k, st));
        lastSemester_ = st;
    }
}

void View::RenderEnding(const World& world, SemesterState st) {
    using namespace nccu::gfx;

    // A-T3 (完結章節畫面不斷閃爍/像關閉畫布): clear the WHOLE framebuffer
    // to OPAQUE black FIRST, every frame, before drawing the card. This
    // branch early-returns (no Renderer::Clear runs below), and
    // DrawEndingCard's own backdrop is Color{0,0,0,a} with `a` ramping
    // from 0 during the fade-in (endingAlpha_). raylib's BeginDrawing
    // does NOT clear and EndDrawing swaps TWO buffers, so at low alpha
    // the semi-transparent backdrop let the stale swap-chain content
    // (the last world frame, or the OTHER buffer) bleed through and
    // flicker — exactly the "像關閉畫布" the owner saw. An opaque clear
    // here makes the fade a clean card-over-solid-black with no
    // bleed-through and no per-buffer alternation. (Verified on harness
    // shots: pre-fix the first ending frames showed the world map behind
    // faint text; post-fix every ending frame is steady.)
    Renderer{}.Clear(Colors::Black);

    // Audit D8 / SC 2.3.3: reduced-motion players skip the half-
    // second luminance ramp and see the card opaque on first paint.
    endingAlpha_ = EndingFadeAlphaStep(
        endingAlpha_, nccu::gfx::Time::DeltaSeconds(),
        world.ReducedMotion());
    // Item 1: the ending .md (its narrative + the new 字卡 reason
    // callbacks) is NEVER drawn because this branch early-returns —
    // so the "why you reached this ending" must be surfaced IN CODE.
    // Extract the render primitives (karma + the EndingGate flags)
    // into a render-only DTO here, where the View still owns World/
    // Player, and hand it to the EndingView. EndingView never touches
    // World/Player itself (MVC purity) — it just renders the DTO.
    EndingSummary es;
    es.state = st;
    if (const Player* ep = world.GetPlayer()) {
        es.karma            = ep->GetKarma();
        es.hasTrueUmbrella  = ep->HasFlag(kFlagHasTrueUmbrella);
        es.consoledTA       = ep->HasFlag(kFlagConsoledTA);
        es.tookCursed       = ep->HasFlag(kFlagTookCursedUmbrella);
        es.boughtUgly       = ep->HasFlag(kFlagBoughtUglyUmbrella);
        es.finaleChoiceMade = ep->HasFlag(kFlagTaFinaleChoiceMade);
    }
    // A-T3: the ending screen is now an INTERACTIVE, STEADY screen with a
    // bottom 3-option menu (回首頁 / 重新開始 / 結束). The View only
    // RENDERS the highlighted row (World::EndingMenuCursor, a pure UI
    // cursor moved by ←/→ in GameController); the menu INTENT
    // (Restart / Quit) flows through World::PendingAppAction set in the
    // controller — no gameplay logic in the View (MVC red line).
    DrawEndingCard(renderer_, es, world.Semester().CurrentName(),
                   endingAlpha_, viewportSize_.x, viewportSize_.y,
                   world.EndingMenuCursor());
}

void View::RenderWorld(const World& world, SemesterState st) {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;

    Renderer{}.Clear(Colors::RayWhite);
    {
        CameraScope cam{camera_};
        Renderer{}.Texture(worldmap_, Vec2{0.0f, 0.0f});

        // 操場 校慶 lap track — a dotted STADIUM outline (running-track
        // shape: top + bottom straights joined by left + right semicircles)
        // on the field the player laps; dots already passed disappear so it
        // shrinks as the lap completes (走完動態消除). Drawn HERE — right
        // after the base map, BEFORE the building/object painter's pass — so
        // it is a GROUND DECAL: 綜合院館 (which overlaps the 操場's east
        // edge) and the runners paint OVER it (layering request:
        // 地圖 → 線條 → 綜院). World space (inside the CameraScope).
        if (world.SportsLapActive()) {
            const float prog = world.SportsLapProgress();
            constexpr int kDots = 48;
            constexpr float kPi = 3.14159265f;
            const float cx = nccu::kSportsTrackCx, cy = nccu::kSportsTrackCy;
            const float a  = nccu::kSportsTrackHalfLen, r = nccu::kSportsTrackR;
            const float straight = 2.0f * a, arc = kPi * r;
            const float perim = 2.0f * straight + 2.0f * arc;
            for (int i = 0; i < kDots; ++i) {
                const float frac = static_cast<float>(i) /
                                   static_cast<float>(kDots);
                if (frac < prog) continue;          // already walked → erased
                const float d = frac * perim;       // distance along perimeter
                float x, y;
                if (d < straight) {                 // top straight, L→R
                    x = cx - a + d;            y = cy - r;
                } else if (d < straight + arc) {    // right end-cap (east)
                    const float th = (d - straight) / r;        // 0..pi
                    x = cx + a + r * std::sin(th);
                    y = cy - r * std::cos(th);
                } else if (d < 2.0f * straight + arc) {  // bottom straight, R→L
                    x = cx + a - (d - straight - arc);  y = cy + r;
                } else {                            // left end-cap (west)
                    const float th = (d - 2.0f * straight - arc) / r;
                    x = cx - a - r * std::sin(th);
                    y = cy + r * std::cos(th);
                }
                renderer_.DrawRect(Rect{x - 5.0f, y - 5.0f, 10.0f, 10.0f},
                                   Color{255, 255, 255, 240});  // lane-marker white
            }
        }

        // Painter's order: buildings keyed on their ground line, objects
        // on their feet (top-left + player height). Lower y paints first,
        // so a character above a building's base is covered by it; one
        // standing below it draws on top — classic JRPG walk-behind.
        // Ambient decorations: advance the render clock ONCE per drawn
        // frame, then resolve the ping-pong frame index per visible def
        // below. DeltaSeconds() is the fixed 1/60 step under the harness,
        // so the animation is deterministic; this accumulator is pure View
        // state and never reaches state.jsonl (MVC §5).
        decorationClock_ += static_cast<double>(Time::DeltaSeconds());

        drawOrder_.clear();
        drawOrder_.reserve(buildings_.size() + decorations_.size() +
                           world.Objects().size());
        for (std::size_t i = 0; i < buildings_.size(); ++i) {
            drawOrder_.push_back(
                DrawRef{buildings_[i].baseY, DrawKind::Building, nullptr, i});
        }
        // Decorations of the CURRENT chapter only — keyed by the def's
        // `chapter` so the chiikawa shows in Ch2 and the cat in Ch3, never
        // both. Depth key is the sprite's bottom edge (its feet), so it
        // walk-behinds against NPCs exactly like a building does.
        for (std::size_t i = 0; i < decorations_.size(); ++i) {
            const auto& def = nccu::gfx::kDecorations[decorations_[i].defIndex];
            if (def.chapter != st) continue;
            const Texture& tex = decorations_[i].texture;
            const Rect dest = nccu::gfx::DecorationDestRect(
                def, tex.Width(), tex.Height());
            drawOrder_.push_back(
                DrawRef{dest.y + dest.height, DrawKind::Decoration, nullptr, i});
        }
        ForEachActive(world.Objects(), [this](const GameObject& o) {
            drawOrder_.push_back(DrawRef{
                o.GetPosition().y + ::world::kPlayerHeight,
                DrawKind::Object, &o, 0});
        });
        std::sort(drawOrder_.begin(), drawOrder_.end(),
                  [](const DrawRef& a, const DrawRef& b) { return a.y < b.y; });
        for (const DrawRef& d : drawOrder_) {
            switch (d.kind) {
            case DrawKind::Object:
                // ISP role dispatch: render only objects that play the
                // IDrawable role. d.obj is const, so use the const
                // accessor; a non-drawable object (none today, but the
                // contract allows it) is simply skipped.
                if (const auto* dr = d.obj->AsDrawable()) dr->Render(renderer_);
                break;
            case DrawKind::Building: {
                const BuildingSprite& bs  = buildings_[d.index];
                const Texture&        tex = buildingTextures_[bs.texIndex];
                const float sw = static_cast<float>(tex.Width());
                const float sh = static_cast<float>(tex.Height());
                // Negative source extents make DrawTexturePro mirror the
                // sprite — carries the Tiled flip into the render.
                renderer_.DrawSprite(
                    tex,
                    Rect{0.0f, 0.0f,
                         bs.flipX ? -sw : sw,
                         bs.flipY ? -sh : sh},
                    bs.dest);
                break;
            }
            case DrawKind::Decoration: {
                const DecorationSprite& ds  = decorations_[d.index];
                const auto&             def = nccu::gfx::kDecorations[ds.defIndex];
                const Texture&          tex = ds.texture;
                // Ping-pong (triangle-wave) frame from the render clock, then
                // the source sub-rect for that frame; centred dest = the
                // breathing 放大縮小. All maths is the pure SpriteStrip
                // helpers (headless-tested); the blit is the same DrawSprite
                // path buildings use — no raylib in any Model/Item code.
                const int frame = nccu::gfx::FrameAt(
                    decorationClock_, def.frameCount, def.fps);
                const Rect src = nccu::gfx::StripSourceRect(
                    frame, def.frameCount, tex.Width(), tex.Height());
                const Rect dest = nccu::gfx::DecorationDestRect(
                    def, tex.Width(), tex.Height());
                renderer_.DrawSprite(tex, src, dest);
                break;
            }
            }
        }

        // Quest-giver "!" overlay (H4). Drawn AFTER the painter's-order
        // pass but still inside the CameraScope so the icon follows the
        // NPC in world space — and on top of buildings/sprites that might
        // otherwise occlude a quest-giver tucked behind a footprint.
        // QuestIndicatorVisible (quest layer) is the SINGLE decision point:
        // it folds the roster's virtual IsQuestGiver() bit together with
        // the per-chapter rules (Ch3 sequential A→B→C chain, Item 4a head
        // light; Ch4 finale 助教-only `!`, Item 1b) so the View stays
        // pure-render — no gameplay logic, no dynamic_cast. NB the Ch4
        // finale NPC is isQuestGiver=false in the roster, so the decision
        // can NO LONGER short-circuit on IsQuestGiver() alone; the predicate
        // owns it. QuestGiverIndicator routes every primitive through
        // IRenderer, keeping the helper headless-testable.
        const Player* qgPlayer = world.GetPlayer();
        const nccu::SemesterState qgState = world.Semester().Current();
        ForEachActive(world.Objects(), [&](const GameObject& o) {
            if (!qgPlayer) return;
            if (!nccu::QuestIndicatorVisible(o.NpcId(), o.IsQuestGiver(),
                                             qgState, *qgPlayer)) return;
            // The hit box lives at the NPC's feet; QuestGiverIndicator
            // lifts the "!" above the bottom-anchored sprite top.
            DrawQuestGiverIndicator(
                renderer_,
                Rect{o.GetPosition().x, o.GetPosition().y,
                     ::world::kPlayerWidth, ::world::kPlayerHeight});
        });

        // H3 visual ground marker — only paint the dashed gold exit line
        // while the player is inside the Interlude (other chapters reuse
        // the same world tile but treat that south band as ordinary
        // road, so the marker would be misleading). Drawn INSIDE the
        // CameraScope so the line sits at world y == kInterludeExitMinY
        // and tracks the camera. Phase ticks with the local accumulator
        // below so the dashes sweep west-to-east — pure visual flourish,
        // does not affect the trigger (InterludeExit.h is geometry-only).
        if (st == SemesterState::Interlude_Market) {
            // Audit D8 / SC 2.3.3: reduced-motion freezes the W→E sweep
            // (phase stops advancing); the line is still drawn so the
            // ground-marker affordance is preserved, only animation is.
            interludeMarkerPhase_ += InterludeMarkerPhaseStep(
                nccu::gfx::Time::DeltaSeconds(),
                world.ReducedMotion());
            DrawInterludeExitMarker(renderer_, interludeMarkerPhase_);
        }
    }
}

void View::RenderHud(const World& world, SemesterState st) {
    using namespace nccu::gfx;

    // 操場 校慶 lap progress ring (HUD, screen space) — fills clockwise as
    // the lap completes; the screen companion to the ground track.
    if (world.SportsLapActive()) {
        const float prog = world.SportsLapProgress();
        constexpr int kDots = 16;
        constexpr float kTwoPi = 6.2831853f;
        const float cx = viewportSize_.x - 60.0f, cy = 120.0f, r = 24.0f;
        for (int i = 0; i < kDots; ++i) {
            const float frac = static_cast<float>(i) /
                               static_cast<float>(kDots);
            const float a = -kTwoPi * 0.25f + frac * kTwoPi;  // top, clockwise
            const float x = cx + r * std::cos(a);
            const float y = cy + r * std::sin(a);
            renderer_.DrawRect(Rect{x - 3.0f, y - 3.0f, 6.0f, 6.0f},
                               frac < prog ? Color{255, 230, 90, 255}
                                           : Color{255, 255, 255, 70});
        }
    }

    // Top-left status: WASD hint, karma/umbrella, optional building,
    // chapter name, rain meter. Previously plain DarkGray/Blue text drawn
    // straight onto the bright worldmap — barely legible. Now it reuses
    // the proven objective-panel idiom (a translucent Color{20,22,30,185}
    // backing rect + bright text) so every value pops on any background.
    {
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
        renderer_.DrawRect(Rect{kHudX - kPad, kHudY - kPad, panelW, panelH},
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

    // Rain "pressure" vignette — PURE render derived only from
    // GetRainMeter() (no sim/state/input touched: MVC §5). The rain is
    // non-lethal this cycle; the feedback is purely visual. Screen-edge
    // darkening in two tiers (≥60 subtle, ≥85 stronger) drawn as four
    // border bands (cheap, no full-screen texture/alloc, deterministic).
    if (const Player* p = world.GetPlayer()) {
        const float rm = p->GetRainMeter();
        unsigned char va = 0;
        if (rm >= 85.0f)      va = 90;
        else if (rm >= 60.0f) va = 45;
        if (va > 0) {
            const Color v{0, 0, 0, va};
            const float W = viewportSize_.x;
            const float H = viewportSize_.y;
            const float b = std::min(W, H) * 0.12f;  // band thickness
            renderer_.DrawRect(Rect{0.0f, 0.0f, W, b}, v);          // top
            renderer_.DrawRect(Rect{0.0f, H - b, W, b}, v);         // bottom
            renderer_.DrawRect(Rect{0.0f, 0.0f, b, H}, v);          // left
            renderer_.DrawRect(Rect{W - b, 0.0f, b, H}, v);         // right
        }
    }

    // Quest objective: a panel-backed one-liner, top-centre but BELOW
    // the left status column (its lines end ~y86) so it never overlaps
    // the karma/chapter readout — the playtest's "不要佔到左上數值狀態
    // ，放一個面板當底". Smaller than the status text; the dark panel
    // makes the bright text pop ("比較顯眼"). Width is estimated from the
    // UTF-8 codepoint count (lead bytes — continuation bytes are
    // 10xxxxxx) treating each glyph as ~size wide, enough to centre a
    // one-liner without a text-measurement dependency in the View.
    if (const Player* p = world.GetPlayer()) {
        const std::string obj = CurrentObjective(st, *p);
        if (!obj.empty()) {
            constexpr int   kObjSize = 14;
            constexpr float kPad     = 6.0f;
            // Measure the actual rendered text so the backing panel hugs it
            // exactly — it grows and shrinks with the objective string
            // instead of guessing a fixed width from a glyph count.
            const Vec2 m = TextBuilder{obj}.Size(kObjSize).Measure();
            const float panelW = m.x + kPad * 2.0f;
            const float panelH = m.y + kPad * 2.0f;
            float px = viewportSize_.x * 0.5f - panelW * 0.5f;
            if (px < 4.0f) px = 4.0f;
            // Sit BELOW the top-left status panel (≤6 rows reaches ~y132),
            // so a wide objective bar never touches the karma/money/rain
            // readout — the playtest's "任務指引往下一點不要碰到數值面板".
            constexpr float kPanelY = 138.0f;
            renderer_.DrawRect(Rect{px, kPanelY, panelW, panelH},
                               Color{20, 22, 30, 185});
            TextBuilder{obj}.At(Vec2{px + kPad, kPanelY + kPad})
                .Size(kObjSize).Color(Colors::White).Draw();
        }
    }
}

void View::RenderOverlays(const World& world) {
    using namespace nccu::gfx;

    // Transient ShowMessage toasts: above the world/HUD labels, BELOW the
    // dialog box — an open conversation takes visual precedence, matching
    // the existing DrawDialog ordering. Suppressed during endings since
    // Draw already early-returned above for IsEndingState. Text flows
    // through the same IRenderer path → it picks up the CJK font (the
    // gfx Font fix), so Chinese cues render, not as `?`.
    //
    // Cycle 9.G — TWO independent channels are drawn each frame so a
    // chapter-clear toast (Top) and a regular ShowMessage (Bottom) can
    // coexist (cycle9f-post-iteration-diagnosis §B). Top is drawn
    // FIRST so its painter order sits below Bottom on the z-layer —
    // overlap is moot at the kSlotGap MessageView uses, but the
    // ordering keeps Bottom (the more recent everyday toast) on top
    // if a future layout shrinks the gap.
    DrawHudMessage(renderer_, world.HudMessage(HudSlot::Top),
                   world.HudAge(HudSlot::Top),
                   viewportSize_.x, viewportSize_.y,
                   world.ReducedMotion(), HudSlot::Top);
    DrawHudMessage(renderer_, world.HudMessage(HudSlot::Bottom),
                   world.HudAge(HudSlot::Bottom),
                   viewportSize_.x, viewportSize_.y,
                   world.ReducedMotion(), HudSlot::Bottom);

    DrawDialog(renderer_, world.Dialog());

    // Tab inventory overlay, on top of the (frozen) world + dialog.
    // Reactive: a pure function of World::InventoryOpen + the Player's
    // bag, drawn each frame it is open (no retained UI state). The View
    // owns World/Player here and builds the render-only InventoryRow DTO
    // (BuildInventoryRows) so InventoryView itself stays render-only —
    // same MVC split as the EndingSummary it hands DrawEndingCard.
    if (world.InventoryOpen()) {
        if (const Player* ip = world.GetPlayer())
            DrawInventory(renderer_, BuildInventoryRows(*ip),
                          world.InventoryCursor(),
                          viewportSize_.x, viewportSize_.y);
    }

    // Top-right affordance: a small always-on hint that an in-game menu
    // exists ("M 選單"). Panel-backed so it stays legible on any tile;
    // hidden while the menu itself is open (the menu replaces it).
    if (!world.MenuOpen()) {
        constexpr float kAffSize = 14.0f;
        constexpr float kPad     = 5.0f;
        const std::string aff = "M 選單";
        int glyphs = 0;
        for (unsigned char c : aff)
            if ((c & 0xC0) != 0x80) ++glyphs;
        // "M " is 1 narrow + 選單 2 wide → estimate width generously.
        const float tw = static_cast<float>(glyphs) * kAffSize;
        const float panelW = tw + kPad * 2.0f;
        const float panelH = kAffSize + kPad * 2.0f;
        const float px = viewportSize_.x - panelW - 6.0f;
        renderer_.DrawRect(Rect{px, 6.0f, panelW, panelH},
                           Color{20, 22, 30, 170});
        TextBuilder{aff}.At(Vec2{px + kPad, 6.0f + kPad})
            .Size(static_cast<int>(kAffSize)).Color(Colors::White).Draw();
    }

    // In-game pause menu overlay — drawn LAST so it sits above the world,
    // HUD, dialog and inventory. Reactive: a pure function of
    // World::MenuOpen + MenuCursor (no retained UI state in the View).
    // A full-screen dim then a centred panel; the cursor row gets a
    // marker + highlight colour so keyboard selection is unambiguous.
    if (world.MenuOpen()) {
        const float W = viewportSize_.x;
        const float H = viewportSize_.y;
        renderer_.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 150});

        constexpr float kPanelW = 340.0f;
        // Cycle 9.E.3: panel grew from 250 to 330 px to accommodate 6
        // rows (was 4) — kFirstY 78 + 5 * kRowH 40 = 278 px from the
        // top edge, plus a ~50 px hint band → 330 keeps the same visual
        // breathing room above & below the rows the 4-row layout had.
        constexpr float kPanelH = 330.0f;
        const float px = W * 0.5f - kPanelW * 0.5f;
        const float py = H * 0.5f - kPanelH * 0.5f;
        renderer_.DrawRect(Rect{px, py, kPanelW, kPanelH},
                           Color{20, 22, 30, 230});

        TextBuilder{"遊戲選單"}
            .At(Vec2{px + kPanelW * 0.5f - 64.0f, py + 24.0f})
            .Size(28).Color(Colors::Gold).Draw();

        // Cycle 9.E.3: 6 items now — 繼續 / 說明 / 減少動畫[開|關] /
        // 擴大目標[開|關] / 重新開始 / 離開. Toggle rows (2,3) show the
        // current state suffix; ordering mirrors GameController's
        // switch. 說明 opens the help overlay drawn below; rows 2 and 3
        // flip World::SetReducedMotion / SetLargeTargets in place
        // (cursor stays on the row so the player can verify the change).
        // Destructive items (Restart/Quit) sit farthest from index 0 so
        // an accidental Enter on Resume doesn't risk progress loss.
        static const char* kLabels[World::kMenuItemCount] = {
            "繼續", "說明", "減少動畫", "擴大目標", "重新開始", "離開"};
        constexpr float kFirstY = 78.0f;
        constexpr float kRowH   = 40.0f;
        for (int i = 0; i < World::kMenuItemCount; ++i) {
            const bool sel = (i == world.MenuCursor());
            std::string row =
                (sel ? std::string("> ") : std::string("  ")) +
                kLabels[i];
            // Toggle rows: append the current on/off state in brackets.
            // The leading "  " / "> " keeps every row's x-alignment so
            // the cursor caret column stays stable as it moves up/down.
            if (i == 2) {
                row += world.ReducedMotion() ? "  [開]" : "  [關]";
            } else if (i == 3) {
                row += world.LargeTargets() ? "  [開]" : "  [關]";
            }
            TextBuilder{row}
                .At(Vec2{px + 70.0f, py + kFirstY + i * kRowH})
                .Size(24)
                .Color(sel ? Color{255, 153, 0, 255} : Colors::White)
                .Draw();
        }
        // Audit D3 / SC 1.4.3: was Colors::DarkGray (80,80,80) on the
        // Color{20,22,30,230} pause-menu panel — ~1.05:1, effectively
        // invisible. Color{180,180,180,255} hits ~7:1 (AAA-large,
        // AA-normal) on the same backing.
        TextBuilder{"↑ ↓ 選擇   Enter 確認   M 繼續"}
            .At(Vec2{px + 20.0f, py + kPanelH - 30.0f})
            .Size(14).Color(Color{180, 180, 180, 255}).Draw();
    }

    // REQUIREMENT #9 + U2-T4: the in-game 說明 (how-to-play) overlay —
    // drawn ABOVE the menu (which is still up behind it). Pure function of
    // World::HelpOpen() + World::HelpPage(); the same shared GameHelp text
    // the title screen uses, so the two never drift. A near-full-screen
    // panel showing one PAGE of the (now-paged) help; ←/→ flip the page,
    // M/E/Enter (all handled in GameController) dismiss it back to the menu
    // (ESC quits the program).
    if (world.MenuOpen() && world.HelpOpen()) {
        const float W = viewportSize_.x;
        const float H = viewportSize_.y;
        // Full-screen scrim ON TOP of the paused menu (overlay-only — the
        // title screen Clears instead, so this stays at the call site).
        renderer_.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 205});
        // Shared 遊戲說明 page body (review MINOR de-dup): same panel/title/
        // paged body/indicator/返回 chip the title screen draws. The overlay
        // values: 245α panel, light indicator, "M / E 返回選單" chip at -58.
        nccu::ui::DrawHelpPage(
            [this](Rect r, Color c) { renderer_.DrawRect(r, c); },
            nccu::ui::HelpPageStyle{
                W, H,
                std::max(0, std::min(world.HelpPage(),
                                     nccu::kGameHelpPageCount - 1)),
                Color{18, 20, 28, 245},
                Color{200, 200, 210, 255},
                "M / E 返回選單", -58.0f});
    }

    // U1-T2: the chapter bookend big card, drawn LAST so its brief
    // full-attention beat sits above the world / HUD / dialog / menus.
    // Advance its deterministic timer (Time::DeltaSeconds() is the fixed
    // 1/60 step under the harness, so the card shows for the same frames
    // every replay), then render. A no-op while inactive. Pure View state —
    // never reaches state.jsonl. reducedMotion (audit D8 / SC 2.3.3) makes
    // it appear opaque immediately and hard-cut at the end (no luminance
    // ramp). The card auto-clears after ChapterCardState::kTotal seconds.
    chapterCard_.Step(nccu::gfx::Time::DeltaSeconds());
    DrawChapterCard(renderer_, chapterCard_, viewportSize_.x, viewportSize_.y,
                    world.ReducedMotion());
}

} // namespace nccu
