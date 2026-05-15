#include "World.h"
#include "GameObjectQueries.h"
#include "Player.h"
#include "EventBus.h"
#include "EventWiring.h"
#include "WorldConfig.h"
#include "CharacterSelect.h"
#include "Physics.h"
#include "SemesterState.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/CameraScope.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Bounds.h"
#include "gfx/Renderer.h"
#include "gfx/RaylibRenderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <algorithm>
#include <cstdio>
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

    nccu::World world{selection.spritePath};
    nccu::WireDefaultSubscribers(EventBus::Instance(), world.Semester(),
                                 world.CurrentBuildingName(), kEnterTrigger);

    nccu::gfx::Camera2D cam;

    // Per-frame collider scratch: hoisted out of the loop so the heap
    // allocation is paid once. clear() preserves capacity, insert() does
    // a single memcpy of the static set, and the NPC append fits inside
    // the headroom (16 slots) without reallocating.
    std::vector<Rect> frameColliders;
    frameColliders.reserve(world.StaticColliders().size() + 16);

    // Concrete draw service for this round. Round 3 moves ownership into
    // the View; for now main holds it and hands it to each Render() call.
    RaylibRenderer renderer;

    while (!win.ShouldClose()) {
        const float dt = Time::DeltaSeconds();
        Player* player = world.GetPlayer();
        const Vec2 prevPlayerPos = player ? player->GetPosition() : Vec2{0.0f, 0.0f};

        ForEachActive(world.Objects(), [dt](GameObject& o) { o.Update(dt); });

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
            frameColliders.clear();
            frameColliders.insert(frameColliders.end(),
                                  world.StaticColliders().begin(),
                                  world.StaticColliders().end());
            ForEachActiveExcept(world.Objects(), player,
                                [&frameColliders, kPlayerSize](GameObject& o) {
                if (!o.BlocksMovement()) return;
                const Vec2 p = o.GetPosition();
                frameColliders.push_back(
                    Rect{p.x, p.y, kPlayerSize.x, kPlayerSize.y});
            });
            const Vec2 resolved = nccu::physics::ResolveMove(
                prevPlayerPos, player->GetPosition(), kPlayerSize, frameColliders);
            if (resolved.x != player->GetPosition().x || resolved.y != player->GetPosition().y) {
                player->SetPosition(resolved);
            }
        }

        if (Input::IsPressed(Key::E) && player) {
            const Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
            ForEachActiveExcept(world.Objects(), player, [player, pHit](GameObject& o) {
                if (o.CheckCollision(pHit)) o.Interact(player);
            });
        }

        if (player) {
            // Phase C: detect building entry (transition only).
            const Vec2 playerCentre{
                player->GetPosition().x + kPlayerSize.x * 0.5f,
                player->GetPosition().y + kPlayerSize.y * 0.5f
            };
            if (world.Tracker().Update(playerCentre) == nullptr) {
                world.CurrentBuildingName().clear();
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
                ForEachActive(world.Objects(),
                              [&renderer](const GameObject& o) { o.Render(renderer); });
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
            if (!world.CurrentBuildingName().empty()) {
                TextBuilder{"Inside: " + world.CurrentBuildingName()}
                    .At(Vec2{10, 50}).Size(16).Color(Colors::Black).Draw();
            }
            TextBuilder{std::string{world.Semester().CurrentName()}}
                .At(Vec2{10, 70}).Size(16).Color(Colors::Blue).Draw();
        }

        // End-of-frame sweep: deferred deletion avoids iterator
        // invalidation inside the Update loops above. Capture the
        // player's "about to die" status BEFORE the erase: once the
        // unique_ptr is destroyed the cached Player* dangles, so we
        // cannot legally compare it against post-erase vector contents
        // (heap-use-after-free per [basic.stc.dynamic.deallocation]/4).
        // The bool snapshot sidesteps that.
        const bool playerWillDie = player && !player->IsActive();
        auto& objects = world.Objects();
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [](const std::unique_ptr<GameObject>& o) {
                    return !o || !o->IsActive();
                }),
            objects.end());
        if (playerWillDie) world.ClearPlayer();
    }

    // Drop subscribers before main()'s stack frames go out of scope —
    // closes the static-destruction UB window where a singleton-bound
    // lambda would otherwise capture freed World members.
    EventBus::Instance().Clear();
    return 0;
}
