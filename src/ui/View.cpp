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
#include "ui/InventoryView.h"
#include "ui/MessageView.h"
#include "quest/QuestObjective.h"
#include "quest/QuestIndicator.h"
#include "quest/Chapter3Quest.h"   // 操場 track-ring geometry (kSportsTrack*)
#include "ui/QuestGiverIndicator.h"
#include "state/InterludeExitMarker.h"
#include "ui/GameHelp.h"
#include "ui/RainHud.h"
#include "ui/ReducedMotion.h"
#include "gfx/Renderer.h"
#include "gfx/Time.h"
#include "gfx/CameraScope.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
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
}

void View::Draw(const World& world) {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;

    const SemesterState st = world.Semester().Current();
    if (IsEndingState(st)) {
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
            es.hasTrueUmbrella  = ep->HasFlag("Flag_HasTrueUmbrella");
            es.consoledTA       = ep->HasFlag("Flag_ConsoledTA");
            es.tookCursed       = ep->HasFlag("Flag_TookCursedUmbrella");
            es.boughtUgly       = ep->HasFlag("Flag_BoughtUglyUmbrella");
            es.finaleChoiceMade = ep->HasFlag("Flag_TaFinaleChoiceMade");
        }
        DrawEndingCard(renderer_, es, world.Semester().CurrentName(),
                       endingAlpha_, viewportSize_.x, viewportSize_.y);
        return;   // ending replaces the world; player has no agency here
    }

    if (const Player* p = world.GetPlayer()) {
        camera_.Follow(p->GetPosition(), screenCenter_)
               .ClampToWorld(worldSize_, viewportSize_);
    }

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
        drawOrder_.clear();
        drawOrder_.reserve(buildings_.size() + world.Objects().size());
        for (std::size_t i = 0; i < buildings_.size(); ++i) {
            drawOrder_.push_back(DrawRef{buildings_[i].baseY, nullptr, i});
        }
        ForEachActive(world.Objects(), [this](const GameObject& o) {
            drawOrder_.push_back(DrawRef{
                o.GetPosition().y + ::world::kPlayerHeight, &o, 0});
        });
        std::sort(drawOrder_.begin(), drawOrder_.end(),
                  [](const DrawRef& a, const DrawRef& b) { return a.y < b.y; });
        for (const DrawRef& d : drawOrder_) {
            if (d.obj) {
                // ISP role dispatch: render only objects that play the
                // IDrawable role. d.obj is const, so use the const
                // accessor; a non-drawable object (none today, but the
                // contract allows it) is simply skipped.
                if (const auto* dr = d.obj->AsDrawable()) dr->Render(renderer_);
            } else {
                const BuildingSprite& bs  = buildings_[d.building];
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

        // Lines actually present (Inside is conditional). Width estimated
        // from UTF-8 lead-byte count like the objective panel below — the
        // chapter name is CJK so worst-case ~size px per glyph.
        int rows = 1;                         // WASD hint
        if (p)    rows += 1;                  // karma/umbrella
        if (p)    rows += 1;                  // 金幣 (money)
        if (inside) rows += 1;                // Inside
        rows += 1;                            // chapter
        if (p)    rows += 1;                  // rain
        std::size_t maxGlyphs = std::string("WASD: move    E: pick up").size();
        auto glyphsOf = [](const std::string& s) {
            int g = 0;
            for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++g;
            return static_cast<std::size_t>(g);
        };
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
        TextBuilder{"WASD: move    E: pick up"}
            .At(Vec2{kHudX, y}).Size(kHudSize).Color(Colors::White).Draw();
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

    // REQUIREMENT #9: the in-game 說明 (how-to-play) overlay — drawn
    // ABOVE the menu (which is still up behind it). Pure function of
    // World::HelpOpen(); the same shared GameHelp text the title screen
    // uses, so the two never drift. A near-full-screen panel so the
    // ~22-cell help lines fit; M/E/Enter (handled in GameController)
    // dismisses it back to the menu (ESC quits the program).
    if (world.MenuOpen() && world.HelpOpen()) {
        const float W = viewportSize_.x;
        const float H = viewportSize_.y;
        renderer_.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 205});
        const float pad = 24.0f;
        renderer_.DrawRect(Rect{pad, pad, W - pad * 2.0f, H - pad * 2.0f},
                           Color{18, 20, 28, 245});
        TextBuilder{"遊戲說明"}
            .At(Vec2{W * 0.5f - 52.0f, pad + 6.0f})
            .Size(24).Color(Colors::Gold).Draw();
        // T4: the help text grew (the keys split one-per-line + a new
        // 【雨傘外觀】 section), so the per-row pitch is 15 (blank separators
        // 5) to keep header + 20 text rows + 3 blanks above the footer chip
        // in the 450 px window without scrolling: 15*19 + 5*3 = 300 from
        // hy0=64 lands the closing line ~364, well clear of the chip at
        // ~392. Section headers (【…】) are tinted gold so the four sections
        // read apart at a glance.
        float hy = pad + 40.0f;
        const auto isHeader = [](std::string_view s) {
            return !s.empty() && s.front() == static_cast<char>('\xE3');
        };  // 【 is U+3010 → lead byte 0xE3
        for (const std::string_view ln : nccu::kGameHelpLines) {
            if (!ln.empty())
                TextBuilder{std::string{ln}}
                    .At(Vec2{pad + 22.0f, hy})
                    .Size(15)
                    .Color(isHeader(ln) ? Colors::Gold : Colors::White).Draw();
            hy += ln.empty() ? 5.0f : 15.0f;
        }
        TextBuilder{std::string{nccu::kGameHelpClosing}}
            .At(Vec2{pad + 22.0f, hy}).Size(15).Color(Colors::White).Draw();
        // T4c: make the 返回 prompt PROMINENT — it was faint dark-grey on
        // the dark panel and easy to miss. A gold-bordered chip + bright
        // bold-size gold label, centred at the bottom, so the way out is
        // unmistakable.
        const float chipW = 188.0f, chipH = 26.0f;
        const float chipX = W * 0.5f - chipW * 0.5f;
        const float chipY = H - pad - chipH - 8.0f;
        renderer_.DrawRect(Rect{chipX, chipY, chipW, chipH},
                           Color{62, 52, 18, 255});
        renderer_.DrawRect(Rect{chipX, chipY, chipW, 2.0f}, Colors::Gold);
        renderer_.DrawRect(Rect{chipX, chipY + chipH - 2.0f, chipW, 2.0f},
                           Colors::Gold);
        TextBuilder{"M / E 返回選單"}
            .At(Vec2{W * 0.5f - 58.0f, chipY + 5.0f})
            .Size(17).Color(Colors::Gold).Draw();
    }
}

} // namespace nccu
