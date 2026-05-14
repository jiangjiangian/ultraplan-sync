#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "NPC.h"
#include "EventBus.h"
#include "WorldConfig.h"
#include "BuildingTracker.h"
#include "Buildings.h"
#include "CharacterSelect.h"
#include "Obstacles.h"
#include "Physics.h"
#include "SemesterStateMachine.h"
#include "SemesterState.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/CameraScope.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Bounds.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_map>

// Building-entry → semester transition table. Walking into the named
// landmark advances the state machine to the listed chapter. Data-driven
// so adding a new trigger building does not reopen main().
static const std::unordered_map<std::string, nccu::SemesterState> kEnterTrigger = {
    {"正門",       nccu::SemesterState::Chapter1_AddDrop},
    {"樂活小舖",   nccu::SemesterState::Interlude_Market},
    {"中正圖書館", nccu::SemesterState::Chapter2_Midterms},
    {"操場",       nccu::SemesterState::Chapter3_SportsDay},
    {"行政大樓",   nccu::SemesterState::Chapter4_Finals},
};

int main() {
    using namespace nccu::gfx;

    constexpr int kWinW = 800;
    constexpr int kWinH = 450;
    const Vec2    kScreenCenter{kWinW / 2.0f, kWinH / 2.0f};
    const Vec2    kWorldSize{world::kSize, world::kSize};
    const Vec2    kViewportSize{static_cast<float>(kWinW), static_cast<float>(kWinH)};
    const Vec2    kPlayerSize{world::kPlayerWidth, world::kPlayerHeight};

    auto win = Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    // Pre-gameplay: pick gender, then pick a figure from the
    // school_uniform_3 pack. Closing the window here is an early exit.
    auto selection = nccu::RunCharacterSelect(win);
    if (selection.closed) return 0;

    auto worldmap = nccu::gfx::Texture::Load("resources/assets/maps/worldmap.png");

    nccu::SemesterStateMachine semester;
    std::string currentBuildingName;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName, &semester](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
            if (auto it = kEnterTrigger.find(e.text); it != kEnterTrigger.end()) {
                semester.Transition(it->second);
            }
        });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            // 3-rect umbrella glyph inside the item's 20x20 footprint:
            // tapered canopy in the tint colour + dark handle. Lets the
            // four umbrella subclasses (True / Fragile / ProfessorTrap /
            // Cursed) read at a glance via their distinct umbrellaTint_.
            const float x = e.position.x;
            const float y = e.position.y;
            Renderer r;
            r.Rect(Rect{x +  2.0f, y +  4.0f, 16.0f, 3.0f}, e.color);
            r.Rect(Rect{x +  0.0f, y +  7.0f, 20.0f, 3.0f}, e.color);
            r.Rect(Rect{x +  9.0f, y + 10.0f,  2.0f, 9.0f}, Colors::DarkGray);
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    // Player spawns on Zhinan Rd east of 正門 and east of 苦主, clear of
    // every building wall and NPC hitbox so the AABB resolver never has
    // to rescue them at frame 0.
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{500, 1860}));
    // 4 umbrellas scattered across the central strip between the plaza
    // (y ~ 1080) and Si Wei Blvd (y ~ 1340) — clear of every building.
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{ 320, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{ 750, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1180, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    // The 5 NPC archetypes from the design doc, each parked at the south
    // edge of their anchor building's trigger rect so they read as standing
    // at the entrance. Sprite paths point at the curated Pipoya subset
    // under resources/assets/sprites/ (see sprites/ATTRIBUTIONS.md).
    struct NpcSpawn {
        Vec2                      pos;
        const char*               spritePath;
        std::vector<std::string>  dialog;
        bool                      isQuestGiver;
    };
    // Each NPC sits just OUTSIDE its anchor building's collision rect
    // (trigger-rect shrunk by kBuildingInset, see below) so a player who
    // walks up to talk does not get pushed away by the wall.
    const std::vector<NpcSpawn> npcSpawns = {
        // 苦主 — quest-giver, on Zhinan Rd east of the 正門 gate footprint.
        {Vec2{380, 1860}, "resources/assets/sprites/school_uniform_3/male_02.png",
         {"我的傘…不見了…",
          "明明剛剛還放在傘架上的。",
          "如果你撿到別人的傘，記得物歸原主。"},
         true},
        // 西裝學長 — stern senior just north of 行政大樓 entrance.
        {Vec2{240, 1320}, "resources/assets/sprites/npc/suit_senior.png",
         {"學弟，山下校園的雨季比山上更難熬。",
          "撐傘的姿勢，決定別人怎麼看你。",
          "別把傘當成累贅，那是你身為大學生最後的體面。"},
         false},
        // 學霸 — south of 中正圖書館, in the open between library and the gym row.
        {Vec2{560, 1280}, "resources/assets/sprites/school_uniform_3/female_03.png",
         {"下次小考的範圍我已經整理好了。",
          "圖書館 G 層的座位最安靜，記得避開窗邊。",
          "下雨天背書效率最高，這是科學。"},
         false},
        // 助教 — south of 學思樓, towards Zhinan Rd.
        {Vec2{1730, 1790}, "resources/assets/sprites/npc/ta.png",
         {"助教不是萬能，但你的程式還是得交。",
          "Office Hour 是我最後的救贖。",
          "下個禮拜小考，先把書讀完再來聊雨。"},
         false},
        // 福利社阿姨 — outside 樂活小舖, inside its trigger but clear of the wall.
        {Vec2{460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         {"歡迎光臨！今天的茶葉蛋還熱著呢。",
          "孩子，傘要記得帶啊，淋濕了會感冒的。",
          "缺什麼來阿姨這裡看看，雜貨都有。"},
         false},
    };
    for (const auto& spawn : npcSpawns) {
        auto npc = std::make_unique<NPC>(spawn.pos, spawn.dialog, spawn.isQuestGiver);
        npc->LoadSprite(spawn.spritePath);
        objects.push_back(std::move(npc));
    }

    Player* player = dynamic_cast<Player*>(objects.front().get());
    if (player) player->LoadSprite(selection.spritePath);
    nccu::gfx::Camera2D cam;
    nccu::BuildingTracker tracker;

    // Static collider list: each building's trigger-rect shrunk uniformly
    // by kBuildingInset so the outermost frame stays walkable (and still
    // fires the entry trigger via BuildingTracker), while the interior
    // becomes a wall the player bumps off. 32 px is wide enough that a
    // 24x24 player can squeeze into the trigger band without immediately
    // overlapping the wall. Open-ground "buildings" (track, plaza) are
    // skipped here and handled by Obstacles.h strip colliders instead.
    constexpr float kBuildingInset = 32.0f;
    std::vector<Rect> staticColliders;
    staticColliders.reserve(nccu::buildings::kAll.size() + nccu::obstacles::kAll.size());
    for (const auto& b : nccu::buildings::kAll) {
        bool skip = false;
        for (const auto& n : nccu::obstacles::kBuildingCollisionSkip) {
            if (b.name == n) { skip = true; break; }
        }
        if (skip) continue;
        staticColliders.push_back(Rect{
            b.triggerRect.x + kBuildingInset,
            b.triggerRect.y + kBuildingInset,
            b.triggerRect.width  - 2.0f * kBuildingInset,
            b.triggerRect.height - 2.0f * kBuildingInset
        });
    }
    // Append river segments + track perimeter strips.
    for (const auto& r : nccu::obstacles::kAll) {
        staticColliders.push_back(r);
    }

    while (!win.ShouldClose()) {
        float dt = Time::DeltaSeconds();
        const Vec2 prevPlayerPos = player ? player->GetPosition() : Vec2{0.0f, 0.0f};

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
        }

        // Phase B: clamp player to world AABB right after Update().
        if (player) {
            Vec2 clamped = ClampToWorld(player->GetPosition(), kPlayerSize, kWorldSize);
            if (clamped.x != player->GetPosition().x || clamped.y != player->GetPosition().y) {
                player->SetPosition(clamped);
            }

            // Phase B2: axis-separated collision resolution. Static building
            // walls + every active NPC's hitbox push the player back to
            // prevPlayerPos on the blocked axis only — diagonal movement
            // slides naturally along walls. Items are deliberately not
            // colliders (player walks over them to pick up).
            std::vector<Rect> frameColliders = staticColliders;
            for (const auto& obj : objects) {
                if (!obj || !obj->IsActive() || obj.get() == player) continue;
                if (dynamic_cast<NPC*>(obj.get()) != nullptr) {
                    const Vec2 p = obj->GetPosition();
                    frameColliders.push_back(Rect{
                        p.x, p.y, world::kPlayerWidth, world::kPlayerHeight});
                }
            }
            const Vec2 resolved = nccu::physics::ResolveMove(
                prevPlayerPos, player->GetPosition(), kPlayerSize, frameColliders);
            if (resolved.x != player->GetPosition().x || resolved.y != player->GetPosition().y) {
                player->SetPosition(resolved);
            }
        }

        if (Input::IsPressed(Key::E) && player) {
            for (auto& obj : objects) {
                if (!obj || !obj->IsActive() || obj.get() == player) continue;
                Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
                if (obj->CheckCollision(pHit)) {
                    obj->Interact(player);
                }
            }
        }

        if (player) {
            // Phase C: detect building entry (transition only).
            Vec2 playerCentre{
                player->GetPosition().x + kPlayerSize.x * 0.5f,
                player->GetPosition().y + kPlayerSize.y * 0.5f
            };
            if (tracker.Update(playerCentre) == nullptr) {
                currentBuildingName.clear();
            }

            cam.Follow(player->GetPosition(), kScreenCenter)
               .ClampToWorld(kWorldSize, kViewportSize);
        }

        {
            DrawScope frame;
            Renderer{}.Clear(Colors::RayWhite);

            // World-space pass: worldmap + game objects scroll with the camera.
            {
                CameraScope view{cam};
                Renderer{}.Texture(worldmap, Vec2{0.0f, 0.0f});
                for (auto& obj : objects) {
                    if (obj && obj->IsActive()) obj->Draw();
                }
            }

            // Screen-space HUD: stays pinned, does not scroll with the world.
            TextBuilder{"WASD: move    E: pick up"}
                .At(Vec2{10, 10}).Size(16).Color(Colors::DarkGray).Draw();
            if (player) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "karma: %d   umbrella: %s",
                    player->GetKarma(), player->HasUmbrella() ? "yes" : "no");
                TextBuilder{buf}
                    .At(Vec2{10, 30}).Size(16).Color(Colors::DarkGray).Draw();
            }
            if (!currentBuildingName.empty()) {
                std::string line = "Inside: " + currentBuildingName;
                TextBuilder{line}
                    .At(Vec2{10, 50}).Size(16).Color(Colors::Black).Draw();
            }
            {
                std::string chapter{semester.CurrentName()};
                TextBuilder{chapter}
                    .At(Vec2{10, 70}).Size(16).Color(Colors::Blue).Draw();
            }
        }

        // End-of-frame sweep: deferred deletion to avoid iterator invalidation.
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [](const std::unique_ptr<GameObject>& o) {
                    return !o || !o->IsActive();
                }),
            objects.end());
        if (player && std::find_if(objects.begin(), objects.end(),
                [player](const std::unique_ptr<GameObject>& o) { return o.get() == player; })
            == objects.end()) {
            player = nullptr;
        }
    }

    return 0;
}
