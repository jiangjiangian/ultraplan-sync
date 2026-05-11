#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"
#include "WorldConfig.h"
#include "BuildingTracker.h"
#include "Buildings.h"
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

    auto worldmap = nccu::gfx::Texture::Load("resources/assets/maps/worldmap.png");

    std::string currentBuildingName;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
        });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            Renderer{}.Rect(Rect{e.position.x, e.position.y, 20, 20}, e.color);
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{400, 225}));
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{150, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{300, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{500, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{650, 100}));

    Player* player = dynamic_cast<Player*>(objects.front().get());
    nccu::gfx::Camera2D cam;
    nccu::BuildingTracker tracker;

    while (!win.ShouldClose()) {
        float dt = Time::DeltaSeconds();

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
        }

        // Phase B: clamp player to world AABB right after Update().
        if (player) {
            Vec2 clamped = ClampToWorld(player->GetPosition(), kPlayerSize, kWorldSize);
            if (clamped.x != player->GetPosition().x || clamped.y != player->GetPosition().y) {
                player->SetPosition(clamped);
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
