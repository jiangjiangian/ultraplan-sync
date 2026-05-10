#include "raylib.h"
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    InitWindow(800, 450, "Lost Umbrella - MVP");
    SetTargetFPS(60);

    // Subscribe a console-print handler to demonstrate UI/Data separation.
    // In production, a UIManager would render dialogs from these events.
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            DrawRectangle((int)e.position.x, (int)e.position.y, 20, 20, e.color);
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                {400, 225}));
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          {150, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       {300, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, {500, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        {650, 100}));

    Player* player = dynamic_cast<Player*>(objects.front().get());

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
        }

        if (IsKeyPressed(KEY_E) && player) {
            for (auto& obj : objects) {
                if (!obj || !obj->IsActive() || obj.get() == player) continue;
                Rectangle pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
                if (obj->CheckCollision(pHit)) {
                    obj->Interact(player);
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Draw();
        }
        DrawText("WASD: move    E: pick up", 10, 10, 16, DARKGRAY);
        if (player) {
            DrawText(TextFormat("karma: %d   umbrella: %s",
                player->GetKarma(), player->HasUmbrella() ? "yes" : "no"),
                10, 30, 16, DARKGRAY);
        }
        EndDrawing();

        // End-of-frame sweep: deferred deletion to avoid iterator invalidation
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

    CloseWindow();
    return 0;
}
