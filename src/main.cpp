#include "GameObject.h"
#include "GameObjectFactory.h"
#include "GameObjectQueries.h"
#include "Player.h"
#include "NPC.h"
#include "NpcSpawns.h"
#include "EventBus.h"
#include "EventWiring.h"
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
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
    using nccu::queries::ForEachActive;
    using nccu::queries::ForEachActiveExcept;

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

    auto selection = nccu::RunCharacterSelect(win);
    if (selection.closed) return 0;

    auto worldmap = nccu::gfx::Texture::Load("resources/assets/maps/worldmap.png");

    nccu::SemesterStateMachine semester;
    std::string currentBuildingName;
    nccu::WireDefaultSubscribers(EventBus::Instance(), semester,
                                 currentBuildingName, kEnterTrigger);

    std::vector<std::unique_ptr<GameObject>> objects;
    // Player on Zhinan Rd east of 正門 and east of 苦主, clear of every
    // building wall and NPC hitbox so the AABB resolver never has to
    // rescue them at frame 0.
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{500, 1860}));
    // 4 umbrellas across the central strip between the plaza (y ~ 1080)
    // and Si Wei Blvd (y ~ 1340) — clear of every building.
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{ 320, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{ 750, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1180, 1280}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    for (const auto& spawn : nccu::DefaultNpcSpawns()) {
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
    // becomes a wall the player bumps off. Open-ground "buildings"
    // (track, plaza) are skipped here and handled by Obstacles.h strip
    // colliders instead.
    constexpr float kBuildingInset = 32.0f;
    const auto isCollisionBuilding = [](const nccu::buildings::Building& b) noexcept {
        const auto& skips = nccu::obstacles::kBuildingCollisionSkip;
        return std::find(skips.begin(), skips.end(), b.name) == skips.end();
    };
    const auto toCollider = [](const nccu::buildings::Building& b) noexcept {
        return Rect{
            b.triggerRect.x + kBuildingInset,
            b.triggerRect.y + kBuildingInset,
            b.triggerRect.width  - 2.0f * kBuildingInset,
            b.triggerRect.height - 2.0f * kBuildingInset
        };
    };

    std::vector<Rect> staticColliders;
    staticColliders.reserve(nccu::buildings::kAll.size() + nccu::obstacles::kAll.size());
    for (const auto& b : nccu::buildings::kAll) {
        if (isCollisionBuilding(b)) staticColliders.push_back(toCollider(b));
    }
    std::copy(nccu::obstacles::kAll.begin(), nccu::obstacles::kAll.end(),
              std::back_inserter(staticColliders));

    while (!win.ShouldClose()) {
        const float dt = Time::DeltaSeconds();
        const Vec2 prevPlayerPos = player ? player->GetPosition() : Vec2{0.0f, 0.0f};

        ForEachActive(objects, [dt](GameObject& o) { o.Update(dt); });

        if (player) {
            // Phase B: clamp player to world AABB right after Update().
            const Vec2 clamped = ClampToWorld(player->GetPosition(), kPlayerSize, kWorldSize);
            if (clamped.x != player->GetPosition().x || clamped.y != player->GetPosition().y) {
                player->SetPosition(clamped);
            }

            // Phase B2: axis-separated collision resolution. Static
            // building walls + every BlocksMovement()-true object's
            // hitbox push the player back on the blocked axis only —
            // diagonal slides naturally along walls. Items are
            // deliberately not colliders.
            std::vector<Rect> frameColliders = staticColliders;
            ForEachActiveExcept(objects, player, [&frameColliders](GameObject& o) {
                if (!o.BlocksMovement()) return;
                const Vec2 p = o.GetPosition();
                frameColliders.push_back(
                    Rect{p.x, p.y, world::kPlayerWidth, world::kPlayerHeight});
            });
            const Vec2 resolved = nccu::physics::ResolveMove(
                prevPlayerPos, player->GetPosition(), kPlayerSize, frameColliders);
            if (resolved.x != player->GetPosition().x || resolved.y != player->GetPosition().y) {
                player->SetPosition(resolved);
            }
        }

        if (Input::IsPressed(Key::E) && player) {
            const Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
            ForEachActiveExcept(objects, player, [player, pHit](GameObject& o) {
                if (o.CheckCollision(pHit)) o.Interact(player);
            });
        }

        if (player) {
            // Phase C: detect building entry (transition only).
            const Vec2 playerCentre{
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
            {
                CameraScope view{cam};
                Renderer{}.Texture(worldmap, Vec2{0.0f, 0.0f});
                ForEachActive(objects, [](const GameObject& o) { o.Draw(); });
            }

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
                TextBuilder{"Inside: " + currentBuildingName}
                    .At(Vec2{10, 50}).Size(16).Color(Colors::Black).Draw();
            }
            TextBuilder{std::string{semester.CurrentName()}}
                .At(Vec2{10, 70}).Size(16).Color(Colors::Blue).Draw();
        }

        // End-of-frame sweep: deferred deletion avoids iterator
        // invalidation inside the Update loops above.
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
