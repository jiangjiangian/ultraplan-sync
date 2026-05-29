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
#include "game/quest/Chapter3Quest.h"   // 操場跑道環幾何（kSportsTrack*）
#include "ui/QuestGiverIndicator.h"
#include "game/state/InterludeExitMarker.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"  // 共用的「遊戲說明」分頁渲染器（與 TitleScreen 去重）
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
#include "game/gfx/Decorations.h"     // kDecorations —已擺放的環境裝飾條
#include "engine/render/CameraScope.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace nccu {
using namespace nccu::game::gfx;  // game/gfx 輔助函式

// 建築以「獨立 sprite」疊在純地形底圖上繪製，使站在某建築地面線「上方」的角色被
// 它遮住（walk-behind）。底圖本身已含開放地面特徵（跑道、廣場），故
// kBuildingCollisionSkip 中點名的兩者不配 sprite——它們的像素已在
// worldmap_base.png 裡。
View::View(int windowWidth, int windowHeight)
    : worldmap_(nccu::engine::render::Texture::Load("resources/assets/maps/worldmap_base.png")),
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
        auto tex = nccu::engine::render::Texture::Load(path);
        if (!tex.IsValid()) continue;  // 美術尚未產出 → 不配 sprite
        const std::size_t idx = buildingTextures_.size();
        buildingTextures_.push_back(std::move(tex));
        buildings_.push_back(BuildingSprite{
            idx, b.triggerRect,
            b.triggerRect.y + b.triggerRect.height,
            b.flipX, b.flipY});
    }

    // 環境裝飾條（純裝飾、無玩法效果）。每條的貼圖在此「只」載入一次；某定義的
    // PNG 缺檔（空資源情形）會得到 !IsValid() 並被跳過——故缺資產時這條根本沒有
    // DecorationSprite 可畫，也不會當機。在 InitWindow 之後載入（View 在視窗／GL
    // 環境就緒後才建立），並隨 View 解構，故 RAII 與 GL 的順序成立。這些絕不進入
    // World::Objects()，故不影響自動跑流程。
    decorations_.reserve(nccu::game::gfx::kDecorations.size());
    for (std::size_t i = 0; i < nccu::game::gfx::kDecorations.size(); ++i) {
        auto tex = nccu::engine::render::Texture::Load(nccu::game::gfx::kDecorations[i].stripPath);
        if (!tex.IsValid()) continue;   // 美術尚未放入 → 不配 sprite
        decorations_.push_back(DecorationSprite{i, std::move(tex)});
    }
}

// Draw() 為分派器——先處理章節字卡轉場（每幀都跑），再分流：結局取代世界；否則
// 在跟隨相機下畫世界、再畫螢幕座標的 HUD、最後畫每個疊層。每個 helper 只負責單一
// 關注點；詳見 View.h 的說明區塊。
void View::Draw(const World& world) {
    const SemesterState st = world.Semester().Current();

    UpdateChapterCardTransition(st);

    if (IsEndingState(st)) {
        RenderEnding(world, st);
        return;   // 結局取代世界；玩家可操作之處是那裡繪製的選單
    }

    if (const Player* p = world.GetPlayer()) {
        camera_.Follow(p->GetPosition(), screenCenter_)
               .ClampToWorld(worldSize_, viewportSize_);
    }

    RenderWorld(world, st);
    RenderHud(world, st);
    RenderOverlays(world);
}

// 偵測章節邊界的 FSM 轉場並武裝書檔大字卡。在結局提前返回「之前」做，使
// lastSemester_ 在每個狀態都保持最新（第四章 → 結局的跳轉不武裝任何字卡——
// ChapterCardForTransition 對結局目的地回傳 None——那一節拍由 EndingView 負責）。
// 純渲染側：觸發只讀 View 本就持有的 World 快照；不發事件、不發布、不寫模型，故
// 存檔逐位元不變。「找到傘了」節拍借用延後轉場的空檔：章節要等其收尾旁白對話關
// 閉後（清關旗標／閘門）才翻往市集，故字卡自然出現在奪回場景之後。
void View::UpdateChapterCardTransition(SemesterState st) {
    if (!lastSemester_.has_value()) {
        // 本次首幀：把 World 的開場狀態視為一次新進入，使開場「傘，不見了」字卡為
        // 第一章（或某起始狀態 warp 的章節）觸發。from == 同一狀態會是空操作，故把
        // `from` 種為一個非章節哨兵。
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
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 每幀「先」把整張 framebuffer 清為「不透明」黑，再畫字卡，以消除結局畫面不斷
    // 閃爍／「像關閉畫布」的現象。此分支會提前返回（下方不會跑 Renderer::Clear），
    // 而 DrawEndingCard 自己的背景是 Color{0,0,0,a}，`a` 在淡入期間由 0 漸升
    // （endingAlpha_）。raylib 的 BeginDrawing「不」清畫面、EndDrawing 交換「兩
    // 個」緩衝，故在低 alpha 時半透明背景會讓殘留的 swap-chain 內容（上一幀世界，
    // 或「另一個」緩衝）透出而閃爍。此處的不透明清除使淡入成為乾淨的「字卡疊在純黑
    // 上」，無透出、無逐緩衝交替。
    Renderer{}.Clear(Colors::Black);

    // 減少動畫的玩家略過約半秒的亮度漸變，首幀即看到不透明字卡。
    endingAlpha_ = EndingFadeAlphaStep(
        endingAlpha_, nccu::engine::platform::Time::DeltaSeconds(),
        world.ReducedMotion());
    // 結局 .md（其敘事與理由字卡）「絕不」被繪製，因為此分支提前返回——故「你為何抵
    // 達此結局」必須「在程式碼中」呈現。在 View 仍持有 World／Player 的此處，把渲染
    // 基本值（業力＋結局判定旗標）萃取進純渲染 DTO，交給 EndingView。EndingView 本
    // 身絕不碰 World／Player（MVC 純度）——它只渲染這個 DTO。
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
    using namespace nccu::engine::render;
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
            const auto& def = nccu::game::gfx::kDecorations[decorations_[i].defIndex];
            if (def.chapter != st) continue;
            const Texture& tex = decorations_[i].texture;
            const Rect dest = nccu::game::gfx::DecorationDestRect(
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
                const auto&             def = nccu::game::gfx::kDecorations[ds.defIndex];
                const Texture&          tex = ds.texture;
                // Ping-pong (triangle-wave) frame from the render clock, then
                // the source sub-rect for that frame; centred dest = the
                // breathing 放大縮小. All maths is the pure SpriteStrip
                // helpers (headless-tested); the blit is the same DrawSprite
                // path buildings use — no raylib in any Model/Item code.
                const int frame = nccu::game::gfx::FrameAt(
                    decorationClock_, def.frameCount, def.fps);
                const Rect src = nccu::game::gfx::StripSourceRect(
                    frame, def.frameCount, tex.Width(), tex.Height());
                const Rect dest = nccu::game::gfx::DecorationDestRect(
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
    using namespace nccu::engine::render;
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
    using namespace nccu::engine::render;
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
