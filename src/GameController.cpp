#include "GameController.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "DialogOpener.h"
#include "GameObjectQueries.h"
#include "EventBus.h"
#include "EventWiring.h"
#include "Physics.h"
#include "WorldConfig.h"
#include "gfx/Bounds.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include <algorithm>
#include <memory>
#include <string_view>

namespace nccu {

void ApplyDialogChoice(Player& player, const DialogChoice& choice) {
    player.AddKarma(choice.karmaDelta);
    if (!choice.setsFlag.empty()) {
        if (choice.flagValue) player.SetFlag(choice.setsFlag);
        else                  player.ClearFlag(choice.setsFlag);
    }
}

GameController::GameController(World& world)
    : world_(world),
      enterTrigger_{
          {"正門",       SemesterState::Chapter1_AddDrop},
          {"樂活小舖",   SemesterState::Interlude_Market},
          {"中正圖書館", SemesterState::Chapter2_Midterms},
          {"操場",       SemesterState::Chapter3_SportsDay},
          {"行政大樓",   SemesterState::Chapter4_Finals},
      },
      worldSize_{::world::kSize, ::world::kSize},
      playerSize_{::world::kPlayerWidth, ::world::kPlayerHeight} {
    frameColliders_.reserve(world_.StaticColliders().size() + 16);
    WireDefaultSubscribers(EventBus::Instance(), world_.Semester(),
                           world_.CurrentBuildingName(), enterTrigger_);
}

GameController::~GameController() {
    // Drop subscribers before the World refs they captured die — closes
    // the static-destruction UB window where a singleton-bound lambda
    // would otherwise reference freed currentBuildingName / semester.
    EventBus::Instance().Clear();
}

void GameController::Update() {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;
    using nccu::queries::ForEachActiveExcept;

    // Dialog freeze: while a conversation is open the world is paused —
    // we run ONLY the dialog input and skip the object tick / movement /
    // collision / building-entry / sweep below. IsKeyPressed is edge-
    // triggered, so the E that opened the box (handled in the normal path
    // last frame) does not auto-advance it this frame.
    {
        DialogState& dlg = world_.Dialog();
        if (dlg.Active()) {
            Player* p = world_.GetPlayer();
            if (dlg.AtChoice()) {
                if (Input::IsPressed(Key::Up))   dlg.MoveChoice(-1);
                if (Input::IsPressed(Key::Down)) dlg.MoveChoice(1);
            }
            if (Input::IsPressed(Key::E)) {
                if (const DialogChoice* c = dlg.Advance(); c && p)
                    ApplyDialogChoice(*p, *c);
            }
            return;
        }
    }

    const float dt = Time::DeltaSeconds();
    Player* player = world_.GetPlayer();
    const Vec2 prevPlayerPos = player ? player->GetPosition() : Vec2{0.0f, 0.0f};

    ForEachActive(world_.Objects(), [dt](GameObject& o) { o.Update(dt); });

    if (player) {
        // Phase B: clamp player to world AABB right after Update().
        const Vec2 clamped = ClampToWorld(player->GetPosition(), playerSize_, worldSize_);
        if (clamped.x != player->GetPosition().x || clamped.y != player->GetPosition().y) {
            player->SetPosition(clamped);
        }

        // Phase B2: axis-separated collision resolution. Static building
        // walls + every BlocksMovement()-true object's hitbox push the
        // player back on the blocked axis only — diagonal slides along
        // walls. Items are deliberately not colliders.
        frameColliders_.clear();
        frameColliders_.insert(frameColliders_.end(),
                               world_.StaticColliders().begin(),
                               world_.StaticColliders().end());
        ForEachActiveExcept(world_.Objects(), player, [this](GameObject& o) {
            if (!o.BlocksMovement()) return;
            const Vec2 p = o.GetPosition();
            frameColliders_.push_back(
                Rect{p.x, p.y, playerSize_.x, playerSize_.y});
        });
        const Vec2 resolved = nccu::physics::ResolveMove(
            prevPlayerPos, player->GetPosition(), playerSize_, frameColliders_);
        if (resolved.x != player->GetPosition().x || resolved.y != player->GetPosition().y) {
            player->SetPosition(resolved);
        }
    }

    if (Input::IsPressed(Key::E) && player) {
        const Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
        ForEachActiveExcept(world_.Objects(), player,
            [this, player, pHit](GameObject& o) {
                if (!o.CheckCollision(pHit)) return;
                if (const std::string_view id = o.NpcId(); !id.empty())
                    OpenNpcDialog(world_.Dialog(), *player, id,
                                  world_.Semester().Current());     // talk
                else
                    o.Interact(player);                              // pick up / Vendor
            });
    }

    if (player) {
        // Phase C: detect building entry (transition only).
        const Vec2 playerCentre{
            player->GetPosition().x + playerSize_.x * 0.5f,
            player->GetPosition().y + playerSize_.y * 0.5f
        };
        if (world_.Tracker().Update(playerCentre) == nullptr) {
            world_.CurrentBuildingName().clear();
        }
    }

    // End-of-frame sweep: deferred deletion avoids iterator invalidation
    // inside the Update loops above. Snapshot the player's death BEFORE
    // erase — the cached Player* dangles once its unique_ptr is
    // destroyed (heap-use-after-free per [basic.stc.dynamic
    // .deallocation]/4). The bool snapshot sidesteps that.
    const bool playerWillDie = player && !player->IsActive();
    auto& objects = world_.Objects();
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [](const std::unique_ptr<GameObject>& o) {
                return !o || !o->IsActive();
            }),
        objects.end());
    if (playerWillDie) world_.ClearPlayer();
}

} // namespace nccu
