#include "View.h"
#include "World.h"
#include "Player.h"
#include "GameObject.h"
#include "GameObjectQueries.h"
#include "Buildings.h"
#include "Obstacles.h"
#include "WorldConfig.h"
#include "DialogView.h"
#include "EndingView.h"
#include "InventoryView.h"
#include "MessageView.h"
#include "QuestObjective.h"
#include "QuestGiverIndicator.h"
#include "InterludeExitMarker.h"
#include "GameHelp.h"
#include "RainHud.h"
#include "ReducedMotion.h"
#include "gfx/Renderer.h"
#include "gfx/Time.h"
#include "gfx/CameraScope.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
#include <algorithm>
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
        DrawEndingCard(renderer_, st, world.Semester().CurrentName(),
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
                d.obj->Render(renderer_);
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
        // otherwise occlude a quest-giver tucked behind a footprint. The
        // virtual IsQuestGiver() (default false on GameObject, overridden
        // by NPC) closes the dispatch under inheritance, so no dynamic_cast.
        // QuestGiverIndicator routes every primitive through IRenderer,
        // keeping the View pure-render and the helper headless-testable.
        ForEachActive(world.Objects(), [this](const GameObject& o) {
            if (!o.IsQuestGiver()) return;
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
            int glyphs = 0;
            for (unsigned char c : obj)
                if ((c & 0xC0) != 0x80) ++glyphs;
            constexpr float kObjSize = 14.0f;
            constexpr float kPad     = 6.0f;
            const float tw = glyphs * kObjSize;
            const float panelW = tw + kPad * 2.0f;
            const float panelH = kObjSize + kPad * 2.0f;
            float px = viewportSize_.x * 0.5f - panelW * 0.5f;
            if (px < 4.0f) px = 4.0f;
            constexpr float kPanelY = 88.0f;   // clears the 4 status lines
            renderer_.DrawRect(Rect{px, kPanelY, panelW, panelH},
                               Color{20, 22, 30, 185});
            TextBuilder{obj}.At(Vec2{px + kPad, kPanelY + kPad})
                .Size(static_cast<int>(kObjSize)).Color(Colors::White).Draw();
        }
    }

    // Transient ShowMessage toast: above the world/HUD labels, BELOW the
    // dialog box — an open conversation takes visual precedence, matching
    // the existing DrawDialog ordering. Suppressed during endings since
    // Draw already early-returned above for IsEndingState. Text flows
    // through the same IRenderer path → it picks up the CJK font (the
    // gfx Font fix), so Chinese cues render, not as `?`.
    DrawHudMessage(renderer_, world.HudMessage(), world.HudAge(),
                   viewportSize_.x, viewportSize_.y,
                   world.ReducedMotion());

    DrawDialog(renderer_, world.Dialog());

    // Tab inventory overlay, on top of the (frozen) world + dialog.
    // Reactive: a pure function of World::InventoryOpen + the Player's
    // count map, drawn each frame it is open (no retained UI state).
    if (world.InventoryOpen()) {
        if (const Player* ip = world.GetPlayer())
            DrawInventory(renderer_, ip->Consumables(),
                          viewportSize_.x, viewportSize_.y);
    }

    // Top-right affordance: a small always-on hint that an in-game menu
    // exists ("ESC 選單"). Panel-backed so it stays legible on any tile;
    // hidden while the menu itself is open (the menu replaces it).
    if (!world.MenuOpen()) {
        constexpr float kAffSize = 14.0f;
        constexpr float kPad     = 5.0f;
        const std::string aff = "ESC 選單";
        int glyphs = 0;
        for (unsigned char c : aff)
            if ((c & 0xC0) != 0x80) ++glyphs;
        // "ESC " is 4 narrow + 選單 2 wide → estimate width generously.
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
        TextBuilder{"↑ ↓ 選擇   Enter 確認   ESC 繼續"}
            .At(Vec2{px + 20.0f, py + kPanelH - 30.0f})
            .Size(14).Color(Color{180, 180, 180, 255}).Draw();
    }

    // REQUIREMENT #9: the in-game 說明 (how-to-play) overlay — drawn
    // ABOVE the menu (which is still up behind it). Pure function of
    // World::HelpOpen(); the same shared GameHelp text the title screen
    // uses, so the two never drift. A near-full-screen panel so the
    // ~22-cell help lines fit; ESC/E/Enter (handled in GameController)
    // dismisses it back to the menu.
    if (world.MenuOpen() && world.HelpOpen()) {
        const float W = viewportSize_.x;
        const float H = viewportSize_.y;
        renderer_.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 205});
        const float pad = 24.0f;
        renderer_.DrawRect(Rect{pad, pad, W - pad * 2.0f, H - pad * 2.0f},
                           Color{18, 20, 28, 245});
        TextBuilder{"遊戲說明"}
            .At(Vec2{W * 0.5f - 64.0f, pad + 14.0f})
            .Size(26).Color(Colors::Gold).Draw();
        float hy = pad + 56.0f;
        for (const std::string_view ln : nccu::kGameHelpLines) {
            if (!ln.empty())
                TextBuilder{std::string{ln}}
                    .At(Vec2{pad + 24.0f, hy})
                    .Size(16).Color(Colors::White).Draw();
            hy += 22.0f;
        }
        TextBuilder{std::string{nccu::kGameHelpClosing}}
            .At(Vec2{pad + 24.0f, hy}).Size(16).Color(Colors::White).Draw();
        // Audit D3 / SC 1.4.3: same fix as the pause-menu hint above —
        // help overlay sits on Color{18,20,28,245}, even darker than
        // the menu, so Colors::DarkGray was effectively invisible.
        TextBuilder{"ESC / E 返回選單"}
            .At(Vec2{W * 0.5f - 70.0f, H - pad - 28.0f})
            .Size(14).Color(Color{180, 180, 180, 255}).Draw();
    }
}

} // namespace nccu
