#include "GameController.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "DialogOpener.h"
#include "EndingGate.h"
#include "ChapterGate.h"
#include "ChapterToast.h"
#include "Chapter2Quest.h"
#include "Chapter3Quest.h"
#include "Chapter4Quest.h"
#include "Vendor.h"
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

namespace {

// I5: a Vendor has an empty NpcId(), so its open conversation is tagged
// with this sentinel context (NOT a real npcId — DialogOpener never
// produces it, the C.3 suit_senior / ta one-shot guards compare against
// real ids only, so this is inert for every existing routing). The
// dialog confirm branch reads dlg.NpcId() == kVendorContext to know the
// confirmed choice is a stock line and route it to Vendor::TryBuy.
constexpr std::string_view kVendorContext = "__vendor__";

// The trailing "不買" (decline) label appended to every vendor menu.
// REQUIREMENT #4: a purchase must never be forced — the player can
// always walk away without spending money or setting a flag. The
// confirm branch recognises a chosen index == cfg.stock.size() (this
// entry, always the LAST choice) as "decline": it closes the dialog
// and does NOT call Vendor::TryBuy, so no DeductMoney / AddConsumable /
// item.setsFlag / EventBus purchase event fires.
constexpr std::string_view kVendorDeclineLabel = "先不買，謝謝";

// Build the shop conversation: the greeting line(s) then one choice per
// in-stock item, then a final "不買" decline choice. The stock choice
// label is the vendor's own formatted stock line (NPC::Interact already
// cycles those, so this reuses the exact same browsing text). No
// karma/flag on the DialogChoice itself — the economy side-effects
// (DeductMoney, AddConsumable, the EventBus purchase events, the
// soft-cap, item.setsFlag) ALL stay inside Vendor::TryBuy, untouched;
// the stock choice only carries the index (its position in Choices()).
// The decline choice carries nothing and is detected positionally
// (index == stock size) so it can never mutate state — REQUIREMENT #4.
void OpenVendorMenu(DialogState& dlg, const Vendor& vendor) {
    const VendorConfig& cfg = vendor.Config();
    std::vector<std::string> greeting;
    if (!cfg.greetingLines.empty()) greeting = cfg.greetingLines;
    else                            greeting.push_back(cfg.greeting);

    std::vector<DialogChoice> choices;
    for (const auto& item : cfg.stock) {
        // Sold-out lines are still shown (TryBuy answers "賣完了" on
        // confirm) so the stock count stays visible; an unlimited or
        // in-stock line is buyable. Label mirrors NPC::Interact's
        // "<id> - <price> 元" preview.
        choices.push_back(DialogChoice{
            item.itemId + std::string(" - ") +
                std::to_string(item.price) + std::string(" 元"),
            0, std::string{}, false, {}});
    }
    // REQUIREMENT #4: the always-present decline option. Appended LAST
    // so a stock choice keeps its 0-based index == its stock slot (the
    // pinned TryBuy(stockIdx) contract is unchanged); the decline index
    // is exactly cfg.stock.size(). Present even when stock is empty so a
    // greeting-only stall is still a real, exitable choice (no forced
    // dead-end). karmaDelta 0 / setsFlag "" — it carries no side effect.
    choices.push_back(DialogChoice{
        std::string(kVendorDeclineLabel), 0, std::string{}, false, {}});
    dlg.Open(std::move(greeting), std::move(choices));
    dlg.SetNpcContext(std::string(kVendorContext));
}

}  // namespace

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
            // H3 (cycle9): the previous chapter's clear toast lands the
            // same frame as the FSM hop; this hint overwrites it ~1
            // frame later so the player sees BOTH (the snap, then the
            // direction) on the same arrival. Reset the south-band latch
            // so the exit toast fires once per visit (and again on
            // re-entry, never twice in a row).
            EventBus::Instance().Publish(
                Event{EventType::ShowMessage, kInterludeArrivalHint});
            interludeExitZoneLatched_ = false;
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

    // In-game pause menu (top-right). Opens with Esc or M; while open the
    // world is fully frozen (we early-return before the object tick /
    // movement / rain / sweep, exactly like the dialog/inventory
    // freezes). Placed FIRST so the pause takes precedence over a dialog
    // or the Tab inventory. Esc/M also closes it (toggle / Resume), so it
    // is reachable and dismissable with one key. Restart/Quit only RECORD
    // an intent on the World — the actual World teardown+rebuild happens
    // in main.cpp's outer loop, the single place that can re-run the
    // RAII-ordered composition root without leaking EventBus subscribers
    // (BUGLEDGER B2/H1). The controller never destroys itself.
    {
        using nccu::gfx::Input;
        using nccu::gfx::Key;
        const bool toggle =
            Input::IsPressed(Key::Escape) || Input::IsPressed(Key::M);
        if (world_.MenuOpen()) {
            // REQUIREMENT #9: the 說明 help overlay sits ON TOP of the
            // paused menu. While it is up, ESC / E / Enter dismisses it
            // back to the menu and the menu cursor / sim stay frozen.
            // Handled FIRST so a key meant for "close help" never also
            // moves the menu cursor or triggers an AppAction.
            if (world_.HelpOpen()) {
                if (Input::IsPressed(Key::Escape) ||
                    Input::IsPressed(Key::M) ||
                    Input::IsPressed(Key::Enter) ||
                    Input::IsPressed(Key::E))
                    world_.SetHelpOpen(false);
                return;                         // frozen behind help
            }
            if (Input::IsPressed(Key::Up))   world_.MoveMenuCursor(-1);
            if (Input::IsPressed(Key::Down)) world_.MoveMenuCursor(1);
            if (toggle) {                       // Esc/M = quick Resume
                world_.SetMenuOpen(false);
                return;
            }
            if (Input::IsPressed(Key::Enter)) {
                switch (world_.MenuCursor()) {
                    case 0:                     // 繼續 (Resume)
                        world_.SetMenuOpen(false);
                        break;
                    case 1:                     // 說明 (Help) — overlay
                        world_.SetHelpOpen(true);
                        break;
                    case 2:                     // 重新開始 (Restart)
                        world_.RequestAppAction(
                            World::AppAction::Restart);
                        break;
                    default:                    // 離開 (Quit)
                        world_.RequestAppAction(
                            World::AppAction::Quit);
                        break;
                }
            }
            return;   // frozen while the menu is up
        }
        if (toggle) {                           // open from gameplay
            world_.SetMenuOpen(true);
            return;
        }
    }

    // Dialog freeze: while a conversation is open the world is paused —
    // we run ONLY the dialog input and skip the object tick / movement /
    // collision / building-entry / sweep below. IsKeyPressed is edge-
    // triggered, so the E that opened the box (handled in the normal path
    // last frame) does not auto-advance it this frame.
    {
        DialogState& dlg = world_.Dialog();
        // I5: drop the pending-vendor target the instant its menu is no
        // longer the open conversation (closed greeting-only, advanced
        // past with no stock, replaced by an NPC dialog, swept by a
        // chapter transition). Keeps the non-owning Vendor* from ever
        // outliving the World object it points at — once cleared, the
        // confirm branch's `pendingVendor_ && ...` guard is inert.
        if (pendingVendor_ &&
            (!dlg.Active() || dlg.NpcId() != kVendorContext))
            pendingVendor_ = nullptr;
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
                // I5: capture the highlighted stock index BEFORE Advance()
                // — confirming resets choiceCursor_ (Close/next-lines).
                const bool atChoice = dlg.AtChoice();
                const std::size_t stockIdx =
                    static_cast<std::size_t>(dlg.ChoiceCursor());
                if (const DialogChoice* c = dlg.Advance(); c && p) {
                    // I5: a confirmed Vendor stock line drives the real
                    // purchase. ALL economy side-effects (DeductMoney,
                    // AddConsumable, the ShowMessage + PickupAcquired
                    // EventBus events, the money soft-cap, item.setsFlag
                    // → e.g. Flag_BoughtUglyUmbrella for Ending C) stay
                    // inside Vendor::TryBuy exactly as the pinned
                    // test_vendor contract asserts — the DialogChoice
                    // carries no karma/flag of its own, so the generic
                    // ApplyDialogChoice below is a no-op for it. The Ch2
                    // EnergyDrink reaches the count inventory through
                    // this same TryBuy → AddConsumable path, so
                    // TryRescueBookworm's ConsumeOne("EnergyDrink") can
                    // now succeed in-engine (Flag_Ch2Cleared reachable).
                    if (npc == kVendorContext && pendingVendor_ &&
                        atChoice) {
                        // REQUIREMENT #4: the LAST choice is always the
                        // "不買" decline (OpenVendorMenu appends it after
                        // every stock line), so its index is exactly the
                        // stock size. Picking it must NOT buy — close the
                        // conversation and drop the pending vendor with
                        // ZERO economy mutation (no DeductMoney, no
                        // AddConsumable, no item.setsFlag, no EventBus
                        // purchase event). Only a real stock index
                        // (< stock size) reaches Vendor::TryBuy, whose
                        // pinned side-effect contract is unchanged.
                        const std::size_t stockN =
                            pendingVendor_->Config().stock.size();
                        if (stockIdx >= stockN) {        // decline
                            pendingVendor_ = nullptr;
                            dlg.Close();
                            return;
                        }
                        (void)pendingVendor_->TryBuy(p, stockIdx);
                        pendingVendor_ = nullptr;
                        CheckEndingGates(*p, world_.Semester(), dlg);
                        CheckChapterGates(*p, world_.Semester(), dlg);
                        return;
                    }
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

    // BUGLEDGER I8 / Cycle 4 + REQUIREMENT #5: the GDD rain-survival
    // loop, live & lethal, in EVERY chapter. Driven once per frame here
    // so it is naturally frozen during dialog/inventory like the rest of
    // the sim (both early-return above). Skipped only in the market
    // interlude and endings (safe, non-gameplay states). THREE states
    // (was two — the old "has umbrella ⇒ full shelter" made the Ch1
    // umbrella permanent rain-immunity, so only Ch1 had any pressure;
    // a full-spine trace showed Ch3 == 0.0 every frame, Ch2/Ch4 avg
    // ≈2.5):
    //   INSIDE A BUILDING             -> DrainRain          (-10 u/s,
    //                                    the true haven, full recovery)
    //   OUTDOORS, HOLDING AN UMBRELLA -> ApplyRainSheltered (+1.5 u/s,
    //                                    lethal-armed — an umbrella
    //                                    SLOWS the soak, doesn't stop
    //                                    it; chapter2.md's explicit
    //                                    "still accrues, reduced rate")
    //   OUTDOORS, NO UMBRELLA         -> ApplyRain           (+5 u/s,
    //                                    lethal — UNCHANGED; Ch1 is
    //                                    umbrella-less so Ch1 rain is
    //                                    byte-identical to before)
    // CurrentBuildingName() is non-empty when the player center is in a
    // building trigger zone (refreshed at the end of the previous frame
    // ~line 360; one-frame latency on a ≤10 u/s rate is imperceptible
    // and deterministic). The umbrella now buys TIME, not immunity — you
    // must still periodically duck inside to fully recover — so rain is
    // a live manage-your-exposure concern EVERY chapter; competent play
    // (the ending scripts' long in-building dialog stretches drain at
    // -10 u/s) never reaches 100, so the spine stays winnable &
    // deterministic (regression-pinned, re-verified 2× byte-identical).
    if (player) {
        const SemesterState ss = world_.Semester().Current();
        const bool inEnding = ss == SemesterState::Ending_A ||
                              ss == SemesterState::Ending_B ||
                              ss == SemesterState::Ending_C;
        if (ss != SemesterState::Interlude_Market && !inEnding) {
            if (!world_.CurrentBuildingName().empty())
                player->DrainRain(dt);                  // full recovery
            else if (player->HasUmbrella())
                player->ApplyRainSheltered(dt, /*lethal=*/true);
            else
                player->ApplyRain(dt, /*lethal=*/true); // full exposure
        }
    }

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
        // I3 fix: the movement collider for a BlocksMovement() NPC is a
        // player-sized box at the NPC origin, and Rect::Intersects is
        // strict, so physics::ResolveMove halts the player EXACTLY flush
        // against a static NPC (touching, never strictly overlapping). A
        // 24x24 E-probe at the player origin therefore never collides —
        // dialog could never open for a walked-up player (harness OR
        // human). Give the E-probe an explicit interaction reach: inflate
        // it by kInteractReach on every side so a flush-blocked player
        // still overlaps the NPC's hitbox. The MOVEMENT collider is left
        // exactly as-is (frameColliders_ above, unchanged) — the player
        // still cannot walk through an NPC; only the talk reach grows.
        // The margin (8 px, a third of the 24 px box) is far smaller than
        // the world spacing between NPCs/items, so it can only reach an
        // object the player is already standing flush against.
        constexpr float kInteractReach = 8.0f;
        const Rect pHit{player->GetPosition().x - kInteractReach,
                        player->GetPosition().y - kInteractReach,
                        24.0f + 2.0f * kInteractReach,
                        24.0f + 2.0f * kInteractReach};
        ForEachActiveExcept(world_.Objects(), player,
            [this, player, pHit](GameObject& o) {
                if (!o.CheckCollision(pHit)) return;
                // I5: a Vendor's NpcId() is empty, so without this it
                // would fall to o.Interact() (NPC line-cycling) and
                // Vendor::TryBuy would have no runtime caller — Ending C
                // / the Ch2 EnergyDrink were unreachable. Route a shop
                // interaction to a buy-choice dialog instead; the purchase
                // itself (money / inventory / EventBus events / soft-cap /
                // setsFlag) stays entirely inside Vendor::TryBuy, invoked
                // on confirm in the dialog branch above. One menu per E
                // tap: skip if a dialog already opened this pass.
                if (o.IsVendor()) {
                    if (world_.Dialog().Active()) return;
                    auto* vendor = static_cast<Vendor*>(&o);
                    OpenVendorMenu(world_.Dialog(), *vendor);
                    pendingVendor_ = vendor;
                    return;
                }
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
        if (InInterludeExitZone(c)) {
            // H3 (cycle9): publish "準備離開市集" the FIRST frame the
            // player steps into the south band on this visit; stay silent
            // thereafter. Helper owns the latch flip + Publish so the
            // production path and the regression test exercise the same
            // code (the test directly drives MaybeAnnounceInterludeExit
            // against its own latch). The latch is reset in the
            // Interlude-arrival branch above, so a future re-visit
            // reissues the cue exactly once.
            MaybeAnnounceInterludeExit(interludeExitZoneLatched_);
            player->SetFlag("Flag_LeaveInterlude");
        }
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
