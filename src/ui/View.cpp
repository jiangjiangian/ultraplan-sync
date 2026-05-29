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
    // 結局畫面是一個可互動且穩定的畫面，底部帶 3 選項選單（回首頁 / 重新開始 /
    // 結束）。View 只「渲染」當前高亮列（World::EndingMenuCursor，純 UI 游標，由
    // GameController 以 ←/→ 移動）；選單「意圖」（重新開始／結束）則經控制器設定的
    // World::PendingAppAction 流出——View 內不含玩法邏輯（MVC 紅線）。
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

        // 操場跑道的地面貼花——抽到 ui/world/SportsLapTrack.cpp。在畫家順序這一趟
        // 之「前」繪製，使綜合院館與跑者疊在它「之上」。無進行中的跑圈時為空操作。
        DrawSportsLapTrack(renderer_, world);

        // 畫家順序：建築以其地面線為鍵，物件以其雙腳（左上角 + 玩家高度）為鍵。y 較
        // 小者先畫，故位於某建築基座「上方」的角色會被它遮住；站在其「下方」者則畫在
        // 上面——經典 JRPG 的 walk-behind。
        // 環境裝飾：每幀繪製時把渲染時鐘推進「一次」，再於下方對每個可見定義解析其
        // 來回（ping-pong）影格索引。在 harness 下 DeltaSeconds() 是固定的 1/60 步長，
        // 故動畫具決定性；此累加器是純 View 狀態，絕不進入存檔（MVC）。
        decorationClock_ += static_cast<double>(nccu::engine::platform::Time::DeltaSeconds());

        drawOrder_.clear();
        drawOrder_.reserve(buildings_.size() + decorations_.size() +
                           world.Objects().size());
        for (std::size_t i = 0; i < buildings_.size(); ++i) {
            drawOrder_.push_back(
                DrawRef{buildings_[i].baseY, DrawKind::Building, nullptr, i});
        }
        // 只取「當前」章節的裝飾——以定義的 `chapter` 為鍵，使吉伊卡哇在第二章現身、
        // 貓在第三章現身，不會同時出現。深度鍵是 sprite 的底邊（雙腳），故它對 NPC 的
        // walk-behind 行為與建築完全相同。
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
                // ISP 角色分派：只渲染扮演 IDrawable 角色的物件。d.obj 為 const，故用
                // const 取值器；不可繪製的物件（目前沒有，但契約允許）直接略過。
                if (const auto* dr = d.obj->AsDrawable()) dr->Render(renderer_);
                break;
            case DrawKind::Building: {
                const BuildingSprite& bs  = buildings_[d.index];
                const Texture&        tex = buildingTextures_[bs.texIndex];
                const float sw = static_cast<float>(tex.Width());
                const float sh = static_cast<float>(tex.Height());
                // 負的來源範圍會讓 DrawTexturePro 鏡像該 sprite——把 Tiled 的翻轉
                // 帶進渲染。
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
                // 由渲染時鐘取得來回（三角波）影格，再取該影格的來源子矩形；置中的
                // 目標矩形 = 呼吸般的放大縮小。所有運算都是純 SpriteStrip 輔助函式
                //（headless 測試過）；位元搬移走的是建築所用的同一條 DrawSprite 路徑——
                // 任何 Model／Item 程式碼都不含 raylib。
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

        // 任務給予者的「!」提示——抽到 ui/world/QuestGiverIndicators.cpp。在畫家
        // 順序這一趟「之後」、但仍在 CameraScope 內繪製，使每個「!」疊在可能遮住其
        // NPC 的建築「之上」。
        DrawQuestGiverIndicators(renderer_, world);

        // 地面視覺標記——只在玩家位於插曲段內時才畫出虛線金色出口線（其他章節重用同一
        // 張世界地磚，但把南側那一帶當作普通道路，故畫此標記會誤導）。在 CameraScope
        // 內繪製，使該線位於世界座標 y == kInterludeExitMinY 並跟隨相機。相位隨下方的
        // 區域累加器遞進，使虛線由西向東掃動——純視覺點綴，不影響觸發
        //（InterludeExit.h 只管幾何）。
        if (st == SemesterState::Interlude_Market) {
            // 減少動畫時凍結由西向東的掃動（相位停止遞進）；該線仍會繪製，以保留地面
            // 標記的提示性，只凍結動畫。
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

    // 校慶操場跑圈進度環（HUD，螢幕座標）——抽到 ui/hud/SportsLapRing.cpp。無進行中
    // 的運動會跑圈時提前返回。
    DrawSportsLapRing(renderer_, world, viewportSize_.x, viewportSize_.y);

    // 左上角狀態面板（業力／金錢／章節／降雨 + 操作提示）。抽到
    // ui/hud/StatusPanel.cpp。只負責渲染；以 UTF-8 碼點寬度估計緊貼最寬的一列。
    DrawStatusPanel(renderer_, world);

    // 降雨「壓力」暈影——抽到 ui/hud/RainVignette.cpp。降雨計量 < 60 時為空操作。
    DrawRainVignette(renderer_, world, viewportSize_.x, viewportSize_.y);

    // 任務目標列——抽到 ui/hud/ObjectiveBar.cpp。無 Player、或當前章節的目標字串為空
    // 時為空操作。
    DrawObjectiveBar(renderer_, world, st,
                     viewportSize_.x, viewportSize_.y);
}

void View::RenderOverlays(const World& world) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 短暫的 ShowMessage 提示橫幅：在世界／HUD 標籤之上、對話框「之下」——進行中的
    // 對話具視覺優先，與既有的 DrawDialog 順序一致。結局期間不顯示，因為 Draw 已在
    // 上方對 IsEndingState 提前返回。文字走同一條 IRenderer 路徑 → 會套用到 CJK
    // 字型，故中文提示能正常渲染，不會變成 `?`。
    //
    // 每幀繪製「兩」條彼此獨立的通道，使章節通關提示（Top）與一般 ShowMessage
    //（Bottom）能並存。Top 先畫，使其畫家順序在 z 層上位於 Bottom 之下——在 MessageView
    // 使用的 kSlotGap 下重疊與否無關緊要，但此順序可在未來版面縮小間隙時，讓 Bottom
    //（較近期的日常提示）保持在上方。
    DrawHudMessage(renderer_, world.HudMessage(HudSlot::Top),
                   world.HudAge(HudSlot::Top),
                   viewportSize_.x, viewportSize_.y,
                   world.ReducedMotion(), HudSlot::Top);
    DrawHudMessage(renderer_, world.HudMessage(HudSlot::Bottom),
                   world.HudAge(HudSlot::Bottom),
                   viewportSize_.x, viewportSize_.y,
                   world.ReducedMotion(), HudSlot::Bottom);

    DrawDialog(renderer_, world.Dialog());

    // Tab 物品欄疊層，疊在（凍結的）世界 + 對話之上。反應式：為 World::InventoryOpen
    // 與玩家背包的純函式，開啟期間每幀繪製（不保留 UI 狀態）。此處由 View 持有
    // World／Player，並建立只供渲染的 InventoryRow DTO（BuildInventoryRows），使
    // InventoryView 本身維持只負責渲染——與它交給 DrawEndingCard 的 EndingSummary
    // 採同樣的 MVC 切分。
    if (world.InventoryOpen()) {
        if (const Player* ip = world.GetPlayer())
            DrawInventory(renderer_, BuildInventoryRows(*ip),
                          world.InventoryCursor(),
                          viewportSize_.x, viewportSize_.y);
    }

    // 右上角「M 選單」提示——抽到 ui/overlay/MenuAffordance.cpp。MenuOpen 為真時
    // 提前返回，故每幀呼叫都安全。
    DrawMenuAffordance(renderer_, world, viewportSize_.x, viewportSize_.y);

    // 遊戲內暫停選單疊層——抽到 ui/overlay/PauseMenu.cpp。MenuOpen 為偽時為空操作，
    // 故每幀呼叫都安全。
    DrawPauseMenu(renderer_, world, viewportSize_.x, viewportSize_.y);

    // 遊戲內說明（玩法說明）疊層——抽到 ui/overlay/HelpOverlay.cpp。除非 MenuOpen
    // 與 HelpOpen 同時為真，否則為空操作，故每幀呼叫都安全。
    DrawHelpOverlay(renderer_, world, viewportSize_.x, viewportSize_.y);

    // 章節起訖大字卡，最後繪製，使其短暫的全神貫注節拍位於世界／HUD／對話／選單之上。
    // 先推進其具決定性的計時器（在 harness 下 Time::DeltaSeconds() 是固定的 1/60
    // 步長，故每次重播字卡顯示的幀數相同），再渲染。未啟用時為空操作。純 View 狀態——
    // 絕不進入存檔。減少動畫時它會立即不透明出現、並在結束時硬切（無亮度漸變）。字卡會
    // 在 ChapterCardState::kTotal 秒後自動清除。
    chapterCard_.Step(nccu::engine::platform::Time::DeltaSeconds());
    DrawChapterCard(renderer_, chapterCard_, viewportSize_.x, viewportSize_.y,
                    world.ReducedMotion());
}

} // namespace nccu
