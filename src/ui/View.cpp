#include "ui/View.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/controller/GameObjectQueries.h"
#include "game/world/Buildings.h"
#include "game/world/Obstacles.h"
#include "game/world/WorldConfig.h"
#include "game/dialog/DialogView.h"
#include "ui/EndingView.h"
#include "ui/ChapterCard.h"
#include "ui/InventoryView.h"
#include "ui/MessageView.h"
#include "game/quest/QuestObjective.h"
#include "game/quest/QuestIndicator.h"
#include "game/quest/Chapter3Quest.h"   // 操場 track-ring geometry (kSportsTrack*)
#include "ui/QuestGiverIndicator.h"
#include "game/state/InterludeExitMarker.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"  // shared 遊戲說明 page renderer (de-dup with TitleScreen)
#include "ui/RainHud.h"
#include "ui/hud/ObjectiveBar.h"
#include "ui/hud/RainVignette.h"
#include "ui/hud/SportsLapRing.h"
#include "ui/hud/StatusPanel.h"
#include "ui/overlay/HelpOverlay.h"
#include "ui/overlay/MenuAffordance.h"
#include "ui/overlay/PauseMenu.h"
#include "ui/world/QuestGiverIndicators.h"
#include "ui/world/SportsLapTrack.h"
#include "ui/ReducedMotion.h"
#include "engine/render/Renderer.h"
#include "engine/platform/Time.h"
#include "game/gfx/SpriteStrip.h"     // FrameAt / StripSourceRect / DecorationDestRect
#include "game/gfx/Decorations.h"     // kDecorations — the placed ambient strips
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
    using namespace nccu::engine::math;

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
        endingAlpha_, nccu::engine::platform::Time::DeltaSeconds(),
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
    using namespace nccu::engine::math;
    using nccu::queries::ForEachActive;

    Renderer{}.Clear(Colors::RayWhite);
    {
        CameraScope cam{camera_};
        Renderer{}.Texture(worldmap_, Vec2{0.0f, 0.0f});

        // 操場 lap track ground decal — extracted to
        // ui/world/SportsLapTrack.cpp (P1 step 7f). Drawn BEFORE the
        // painter's-order pass so 綜合院館 + the runners paint OVER it.
        // No-op when no lap is active.
        DrawSportsLapTrack(renderer_, world);

        // Painter's order: buildings keyed on their ground line, objects
        // on their feet (top-left + player height). Lower y paints first,
        // so a character above a building's base is covered by it; one
        // standing below it draws on top — classic JRPG walk-behind.
        // Ambient decorations: advance the render clock ONCE per drawn
        // frame, then resolve the ping-pong frame index per visible def
        // below. DeltaSeconds() is the fixed 1/60 step under the harness,
        // so the animation is deterministic; this accumulator is pure View
        // state and never reaches state.jsonl (MVC §5).
        decorationClock_ += static_cast<double>(nccu::engine::platform::Time::DeltaSeconds());

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

        // Quest-giver "!" overlays — extracted to
        // ui/world/QuestGiverIndicators.cpp (P1 step 7f). Drawn after
        // the painter's-order pass but inside the CameraScope so each
        // "!" sits ON TOP of buildings that might occlude its NPC.
        DrawQuestGiverIndicators(renderer_, world);

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
                nccu::engine::platform::Time::DeltaSeconds(),
                world.ReducedMotion());
            DrawInterludeExitMarker(renderer_, interludeMarkerPhase_);
        }
    }
}

void View::RenderHud(const World& world, SemesterState st) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;

    // 操場 校慶 lap progress ring (HUD, screen space) — extracted to
    // ui/hud/SportsLapRing.cpp (P1 step 7e). Early-returns when no
    // sports lap is active.
    DrawSportsLapRing(renderer_, world, viewportSize_.x, viewportSize_.y);

    // Top-left status panel (karma/money/chapter/rain + control hints).
    // Extracted to ui/hud/StatusPanel.cpp (P1 step 7a). Render-only;
    // hugs the widest row via UTF-8 codepoint width estimation.
    DrawStatusPanel(renderer_, world);

    // Rain "pressure" vignette — extracted to ui/hud/RainVignette.cpp
    // (P1 step 7e). No-op when RainMeter < 60.
    DrawRainVignette(renderer_, world, viewportSize_.x, viewportSize_.y);

    // Quest objective bar — extracted to ui/hud/ObjectiveBar.cpp
    // (P1 step 7e). No-op when there is no Player or the objective
    // string is empty for the current chapter.
    DrawObjectiveBar(renderer_, world, st,
                     viewportSize_.x, viewportSize_.y);
}

void View::RenderOverlays(const World& world) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;

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

    // Top-right "M 選單" affordance — extracted to
    // ui/overlay/MenuAffordance.cpp (P1 step 7c). Early-returns when
    // MenuOpen is true, so the call is safe every frame.
    DrawMenuAffordance(renderer_, world, viewportSize_.x, viewportSize_.y);

    // In-game pause menu overlay — extracted to ui/overlay/PauseMenu.cpp
    // (P1 step 7b). No-op when MenuOpen is false, so the call is safe
    // every frame.
    DrawPauseMenu(renderer_, world, viewportSize_.x, viewportSize_.y);

    // In-game 說明 (how-to-play) overlay — extracted to
    // ui/overlay/HelpOverlay.cpp (P1 step 7d). No-op unless MenuOpen
    // AND HelpOpen are both true, so the call is safe every frame.
    DrawHelpOverlay(renderer_, world, viewportSize_.x, viewportSize_.y);

    // U1-T2: the chapter bookend big card, drawn LAST so its brief
    // full-attention beat sits above the world / HUD / dialog / menus.
    // Advance its deterministic timer (Time::DeltaSeconds() is the fixed
    // 1/60 step under the harness, so the card shows for the same frames
    // every replay), then render. A no-op while inactive. Pure View state —
    // never reaches state.jsonl. reducedMotion (audit D8 / SC 2.3.3) makes
    // it appear opaque immediately and hard-cut at the end (no luminance
    // ramp). The card auto-clears after ChapterCardState::kTotal seconds.
    chapterCard_.Step(nccu::engine::platform::Time::DeltaSeconds());
    DrawChapterCard(renderer_, chapterCard_, viewportSize_.x, viewportSize_.y,
                    world.ReducedMotion());
}

} // namespace nccu
