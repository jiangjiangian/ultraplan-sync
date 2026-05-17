#include "GameController.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "DialogOpener.h"
#include "EndingGate.h"
#include "ChapterGate.h"
#include "Chapter2Quest.h"
#include "Chapter3Quest.h"
#include "Chapter4Quest.h"
#include "InterludeExit.h"
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
      worldSize_{::world::kSize, ::world::kSize},
      playerSize_{::world::kPlayerWidth, ::world::kPlayerHeight},
      lastRosterState_(world.Semester().Current()) {
    frameColliders_.reserve(64);  // dynamic actors only; terrain is the mask
    WireDefaultSubscribers(EventBus::Instance(), world_.Semester(),
                           world_.CurrentBuildingName());
    // Additional ShowMessage subscriber: mirrors the event text into
    // world_ for the transient HUD banner. Same lifetime as the
    // subscribers above — torn down by the ~GameController Clear() below.
    WireHudMessageSubscriber(EventBus::Instance(), world_);
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

    // Roster follows the FSM. Any trigger (EndingGate, EventWiring,
    // future) only mutates the pure state machine; we observe the change
    // here and ask World to swap the chapter NPCs. A transition fired
    // inside CheckEndingGates (in the dialog branch below) is picked up
    // at the top of the NEXT frame — robust, no new EventType.
    if (const SemesterState cur = world_.Semester().Current();
        cur != lastRosterState_) {
        world_.RespawnChapterRoster(cur);
        // Arriving at the market: place the player at its entrance, well
        // north of the south exit band. Without this a chapter that ended
        // in the south would leave the player already inside the exit
        // zone, instantly bouncing them straight back out (a skipped
        // market). Chapter entry points are an S5c/d/e concern; the
        // Interlude is the only state S5b-2 owns.
        if (cur == SemesterState::Interlude_Market) {
            if (Player* ip = world_.GetPlayer()) {
                ip->SetPosition(nccu::kInterludeEntry);
                // S5b-4 "消耗品當章用完": re-entering the market wipes
                // the consumable inventory, so what was bought for one
                // chapter can't be hoarded across the market boundary
                // into the next — every market visit is a fresh "buy
                // for the chapter ahead" decision (the loop's tension).
                ip->ClearConsumables();
            }
        }
        // Ch4 entry (chapter4.md L6「傘再度失蹤」): the player walks
        // out of 集英樓 with no umbrella. Reset both the generic
        // HasUmbrella bool AND the TrueUmbrella-specific marker, so
        // Ending A's 持-TrueUmbrella condition only holds if the
        // player RE-claims the Ch4 TrueUmbrella (not a leftover from
        // Ch1/Ch3 nor a stray ctor umbrella).
        if (cur == SemesterState::Chapter4_Finals) {
            if (Player* ip = world_.GetPlayer()) {
                ip->SetHasUmbrella(false);
                ip->ClearFlag("Flag_HasTrueUmbrella");
            }
        }
        lastRosterState_ = cur;
    }

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
                // Capture the npc BEFORE Advance() — confirming the last
                // choice can Close() the dialog, which clears NpcId().
                const std::string npc = dlg.NpcId();
                if (const DialogChoice* c = dlg.Advance(); c && p) {
                    ApplyDialogChoice(*p, *c);
                    // C.3(b): a confirmed 西裝學長 choice locks the
                    // branch menu so re-talking can't stack mutually-
                    // exclusive ripple flags. DialogOpener reads this
                    // flag and recaps line-only thereafter.
                    if (npc == "suit_senior")
                        p->SetFlag("Flag_SuitSeniorChoiceMade");
                    // S5e-2d: a confirmed 助教 (d) 結算 choice in Ch4
                    // locks the menu (one-shot, like C.3(b)) so the
                    // moral choice (體諒 → Flag_ConsoledTA, +15) can't
                    // be flipped/re-applied on a re-talk. ApplyDialog
                    // Choice already set Flag_ConsoledTA + karma; the
                    // CheckEndingGates below routes Ending A if its
                    // karma>80 + TrueUmbrella conditions also hold.
                    if (npc == "ta" && world_.Semester().Current() ==
                            SemesterState::Chapter4_Finals)
                        p->SetFlag("Flag_TaFinaleChoiceMade");
                    // Ending gates first, then chapter gates (existing
                    // precedent: EndingGate predates this). Order is safe
                    // either way — once an ending fires, Current() is
                    // Ending_X and none of CheckChapterGates' Ch2/Ch3/
                    // Interlude sibling-ifs can match.
                    CheckEndingGates(*p, world_.Semester(), dlg);
                    CheckChapterGates(*p, world_.Semester(), dlg);
                }
            }
            return;
        }
    }

    // Tab inventory overlay (S5b-5). Edge-triggered toggle, then — while
    // open — freeze the sim exactly like the dialog box above (no tick /
    // movement / collision / sweep). Placed AFTER the dialog block so a
    // conversation has priority and Tab can't pop the panel mid-dialog.
    if (Input::IsPressed(Key::Tab))
        world_.SetInventoryOpen(!world_.InventoryOpen());
    if (world_.InventoryOpen()) return;

    const float dt = Time::DeltaSeconds();
    // Age the transient banner with the sim. Reached only AFTER the
    // dialog AND Tab-inventory early-returns above, so it freezes while
    // a conversation or the inventory is open — a chapter-clear notice
    // fired by a dialog choice then survives to be read once the box
    // closes, instead of burning its TTL behind it.
    world_.TickHud(dt);
    Player* player = world_.GetPlayer();
    const Vec2 prevPlayerPos = player ? player->GetPosition() : Vec2{0.0f, 0.0f};

    ForEachActive(world_.Objects(), [dt](GameObject& o) { o.Update(dt); });

    if (player) {
        // Phase B: clamp player to world AABB right after Update().
        const Vec2 clamped = ClampToWorld(player->GetPosition(), playerSize_, worldSize_);
        if (clamped.x != player->GetPosition().x || clamped.y != player->GetPosition().y) {
            player->SetPosition(clamped);
        }

        // Phase B2: axis-separated collision resolution. The terrain mask
        // (building wall bases, river, painted props) plus every
        // BlocksMovement()-true object's hitbox push the player back on
        // the blocked axis only — diagonal slides along walls. Items are
        // deliberately not colliders.
        frameColliders_.clear();
        ForEachActiveExcept(world_.Objects(), player, [this](GameObject& o) {
            if (!o.BlocksMovement()) return;
            const Vec2 p = o.GetPosition();
            frameColliders_.push_back(
                Rect{p.x, p.y, playerSize_.x, playerSize_.y});
        });
        const Vec2 resolved = nccu::physics::ResolveMove(
            prevPlayerPos, player->GetPosition(), playerSize_,
            frameColliders_, &world_.TerrainMask());
        if (resolved.x != player->GetPosition().x || resolved.y != player->GetPosition().y) {
            player->SetPosition(resolved);
        }
    }

    if (Input::IsPressed(Key::E) && player) {
        const Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
        ForEachActiveExcept(world_.Objects(), player,
            [this, player, pHit](GameObject& o) {
                if (!o.CheckCollision(pHit)) return;
                if (const std::string_view id = o.NpcId(); !id.empty()) {
                    // S5c-2: talking to 學霸 at the rescue moment
                    // consumes the EnergyDrink + sets Flag_Bookworm
                    // Recovered BEFORE the opener runs, so the opener
                    // then routes to 學霸 (d) thanks. No-op for every
                    // other NPC / state (early-returns inside).
                    TryRescueBookworm(*player, id,
                                      world_.Semester().Current());
                    // S5c-3: land the Ch1->Ch2 ripple karma the opener
                    // cannot (once per Ch2, per NPC). No-op elsewhere.
                    TryApplyCh2Ripple(*player, id,
                                      world_.Semester().Current());
                    // S5d-2: advance the Ch3 物物交換鏈 one link, in
                    // order, before the opener routes to (b) recap.
                    TryAdvanceCh3Trade(*player, id,
                                       world_.Semester().Current());
                    // S5d-3: land the Ch3 ProfessorTrap ripple (-10
                    // once). npcId-agnostic — the first Ch3 talk while
                    // holding the trap umbrella triggers it.
                    TryApplyCh3Ripple(*player,
                                      world_.Semester().Current());
                    // S5e-2c: land the Ch4 peak ripple karma (學長/
                    // 學霸/助教 +; ProfTrap -15) so karma>80 (Ending A)
                    // is reachable. Per-NPC, once each. No-op elsewhere.
                    TryApplyCh4Ripple(*player, id,
                                      world_.Semester().Current());
                    OpenNpcDialog(world_.Dialog(), *player, id,
                                  world_.Semester().Current());     // talk
                } else {
                    o.Interact(player);                              // pick up / Vendor
                }
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

    // Interlude exit (the position-trigger half of the progression
    // spine; the dialog-choice half runs in the frozen branch above).
    // Walking into the south 觸發區 arms Flag_LeaveInterlude; the
    // existing CheckChapterGates Interlude sibling-if consumes it ->
    // Transition(InterludeReturnTo()). CheckChapterGates is polled every
    // non-dialog frame because the trigger has no event to ride; it is
    // idempotent (Ch2/Ch3 sibling-ifs gate on their quest flags, the
    // Interlude one on Flag_LeaveInterlude, all consumed on use).
    if (player &&
        world_.Semester().Current() == SemesterState::Interlude_Market) {
        const Vec2 c{player->GetPosition().x + playerSize_.x * 0.5f,
                     player->GetPosition().y + playerSize_.y * 0.5f};
        if (InInterludeExitZone(c)) player->SetFlag("Flag_LeaveInterlude");
    }
    if (player) {
        // S5c-2: lift Flag_Ch2Cleared only once 學霸 is recovered AND
        // the (d) thanks dialog has closed (deferred so the gate does
        // not close that dialog the frame it opens). Runs BEFORE
        // CheckChapterGates so the lifted flag is consumed the same
        // frame. No-op outside Ch2 / before recovery.
        LiftChapter2Clear(*player, world_.Semester().Current(),
                          world_.Dialog());
        CheckChapterGates(*player, world_.Semester(), world_.Dialog());
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
