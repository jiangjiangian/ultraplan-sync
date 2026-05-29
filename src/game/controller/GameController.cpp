#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/state/EndingGate.h"
#include "game/quest/ChapterGate.h"
#include "game/state/ChapterToast.h"
#include "game/controller/screens/EndingScreen.h"    // P3 step 1a: HandleEndingMenu free fn
#include "game/controller/screens/PauseScreen.h"     // P3 step 1b: HandlePauseMenu free fn
#include "game/controller/screens/InventoryScreen.h" // P3 step 1c: HandleInventory free fn
#include "game/controller/InteractDispatch.h"        // P3 step 1d: DispatchInteract free fn
#include "game/controller/screens/DialogScreen.h"    // P3 step 1e: HandleDialog free fn
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/NpcSpawns.h"     // G-2: IsChapter1FlavorNpc routing
#include "game/state/EndingMenuModel.h"  // IsEndingState + EndingMenuChoiceAt
#include "game/state/GameHelpPages.h"    // kGameHelpPageCount
#include "game/quest/InventoryPaging.h"  // kInventoryRowsPerPage
#include "game/vendor/Vendor.h"
#include "game/state/InterludeExit.h"
#include "game/controller/GameObjectQueries.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/controller/SimSystem.h"
#include "game/quest/QuestHookTable.h"
#include "game/world/Physics.h"
#include "game/world/WorldConfig.h"
#include "game/gfx/Bounds.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include <algorithm>
#include <memory>
#include <string_view>

namespace nccu {

// P3 step 1d: the file-local OpenVendorMenu + kVendorContext +
// kVendorDeclineLabel moved into `controller/VendorMenu.{h,cpp}`.
// P3 step 1e: ApplyDialogChoice moved into
// `controller/DialogChoiceApply.{h,cpp}` so DialogScreen.cpp consumes
// it directly without depending on GameController.h.

GameController::GameController(World& world, EventBus& bus)
    : world_(world),
      bus_(bus),
      worldSize_{::world::kSize, ::world::kSize},
      playerSize_{::world::kPlayerWidth, ::world::kPlayerHeight},
      sceneRouter_(world.Semester().Current()) {
    frameColliders_.reserve(64);  // dynamic actors only; terrain is the mask
    // Plan P2 — subscribe/wire side already takes EventBus&; the publish
    // side starts paying it through too. Every subscriber registered here
    // and every Publish() reachable from this controller's frame goes via
    // bus_ from this point on (the chapter-transition stack is the first
    // to be fully cleaned up; the entity/quest publishes still call
    // Instance() for now and will be threaded in follow-up steps).
    WireDefaultSubscribers(bus_, world_.Semester(),
                           world_.CurrentBuildingName());
    // Additional ShowMessage subscriber: mirrors the event text into
    // world_ for the transient HUD banner. Same lifetime as the
    // subscribers above — torn down by the ~GameController Clear() below.
    WireHudMessageSubscriber(bus_, world_);
    // Cycle 9.B H5: karma changes feed the same HUD banner via a
    // KarmaChanged -> ShowMessage hop. Wired AFTER the HUD subscriber
    // so the order of registration matches the intent (KarmaChanged
    // re-publishes ShowMessage; the HUD subscriber sees the resulting
    // ShowMessage on the next dispatch — both subscribers live for the
    // controller's lifetime and are torn down by the same Clear()).
    WireKarmaToastSubscriber(bus_);

    // Build the ordered model-advance pipeline once (awsome_cpp.md §6/§7).
    // The order here IS the per-frame simulation order — Survival ->
    // Movement -> Collision -> Spawn — and must mirror the original inline
    // sequence exactly (the byte-identical state.jsonl gate proves it).
    // SweepSystem is the terminal stage, run separately after the
    // interact/gate logic (see Update), so it is held as a plain member.
    advanceSystems_.push_back(std::make_unique<SurvivalSystem>());
    advanceSystems_.push_back(std::make_unique<MovementSystem>());
    advanceSystems_.push_back(std::make_unique<CollisionSystem>());
    advanceSystems_.push_back(std::make_unique<SpawnSystem>());
}

GameController::~GameController() {
    // Drop subscribers before the World refs they captured die — closes
    // the static-destruction UB window where a singleton-bound lambda
    // would otherwise reference freed currentBuildingName / semester.
    // Plan P2: use the injected bus, not Instance(). Same instance in
    // production (main.cpp passes Instance()) — the change is making
    // the dependency explicit.
    bus_.Clear();
}

void GameController::Update() {
    using namespace nccu::gfx;

    // Roster + side effects follow the FSM. Any trigger (EndingGate,
    // EventWiring, ChapterGate, future) mutates the pure state machine;
    // the SceneRouter observes the change here at TOP-of-Update and
    // applies the harness-observable side effects (player position
    // move, consumable wipe, Ch4 flag clear, arrival hint, latch reset).
    // The view-only roster swap is hoisted forward to END-of-Update
    // (sceneRouter_.SettleRoster below) so the same frame the
    // transition fires paints with the new chapter's NPCs — closes the
    // L8 1-frame npcs[] lag without touching the harness state.jsonl
    // observable timeline.
    sceneRouter_.SettleSideEffects(world_);

    // Per-screen INPUT handlers, in priority order. Each freezes the world
    // and returns true when it owns this frame (ending screen > pause menu
    // > dialog box > Tab inventory). The order and the freeze-then-return
    // semantics are exactly the original inline blocks' — only the bodies
    // moved into focused methods (awsome_cpp.md §7 SRP).
    if (HandleEndingMenu()) return;
    if (HandlePauseMenu())  return;
    if (HandleDialog())     return;
    if (HandleInventory())  return;

    const float dt = Time::DeltaSeconds();
    // Cycle 9.E (audit H2 / D5 / SC 2.2.2): skip-toast key. While a
    // banner is on screen (HudMessage non-empty AND not yet expired),
    // Backspace force-expires it so a player who has read the line can
    // dismiss it on demand instead of waiting out the 4 s TTL. Read here
    // (AFTER the menu / dialog / inventory early-returns above so menu-
    // typed Backspace can't escape the pause, BEFORE TickHud so a one-
    // frame skip lands the same frame as the press) keeps the keystroke
    // strictly a HUD-only side effect — no movement, no rain change,
    // no event publish. SC 2.2.2 expects auto-updating content to be
    // hide-able by the player; this is the smallest such input.
    if (Input::IsPressed(Key::Backspace) && !world_.HudMessage().empty() &&
        !world_.HudExpired()) {
        world_.DismissHud();
    }
    // Age the transient banner with the sim. Reached only AFTER the
    // dialog AND Tab-inventory early-returns above, so it freezes while
    // a conversation or the inventory is open — a chapter-clear notice
    // fired by a dialog choice then survives to be read once the box
    // closes, instead of burning its TTL behind it.
    world_.TickHud(dt);

    // The ordered model-advance pipeline (awsome_cpp.md §6/§7): Survival
    // (rain) -> Movement (object tick + capture prev pos) -> Collision
    // (player AABB + terrain) -> Spawn (lap + deferred spawns). MVC-pure
    // — each ISystem mutates only the model. The same sequence the inline
    // blocks ran in; SimContext threads the pre-tick player position from
    // Movement to Collision and reuses the frameColliders_ scratch.
    SimContext ctx{world_, worldSize_, playerSize_, frameColliders_, {}};
    for (const std::unique_ptr<ISystem>& sys : advanceSystems_)
        sys->Run(ctx, dt);

    // E-interact dispatch (talk / pick up / open a shop). Reads input, so
    // it stays in the controller; the quest side-effects route through the
    // registered QuestHook table.
    DispatchInteract();

    Player* player = world_.GetPlayer();
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
            //
            // Cycle 10.P0a: the latch lives on SceneRouter now —
            // the latch reset on Interlude arrival is in
            // SceneRouter::Settle(), so the GameController only needs
            // to forward the mutable reference here.
            MaybeAnnounceInterludeExit(bus_, sceneRouter_.InterludeExitLatchMut());
            player->SetFlag(kFlagLeaveInterlude);
        }
    }
    if (player) {
        // T2: fire the Ch1 clear (UmbrellaClaimed → Interlude) only AFTER
        // the 苦主's (d) 重逢致謝 exchange dialogue has closed, so the
        // player reads the exchange scene before Ch1 snaps to the Interlude.
        // Sibling of LiftChapter2Clear; runs here (a non-dialog frame, after
        // the dialog early-return above) so the deferred publish lands once
        // the exchange box is dismissed. No-op outside Ch1 / before the
        // grant / while the dialogue is up.
        LiftChapter1Clear(bus_, *player, world_.Semester().Current(),
                          world_.Dialog());
        // S5c-2: lift Flag_Ch2Cleared only once 學霸 is recovered AND
        // the (d) thanks dialog has closed (deferred so the gate does
        // not close that dialog the frame it opens). Runs BEFORE
        // CheckChapterGates so the lifted flag is consumed the same
        // frame. No-op outside Ch2 / before recovery.
        LiftChapter2Clear(*player, world_.Semester().Current(),
                          world_.Dialog());
        CheckChapterGates(bus_, *player, world_.Semester(), world_.Dialog());
        // G2: defer-then-resolve the ending. FIRST open any pending Ch4
        // ending 自白 (inner monologue) — it makes world_.Dialog() active,
        // so the CheckEndingGates poll right after sees the open box and
        // returns (the ending waits until the player reads + closes the
        // narration). On a later non-dialog poll the confession is already
        // played (once-key), the box is closed, and CheckEndingGates fires
        // the transition. This is the same deferred shape as LiftChapter1/
        // 2Clear above, generalised to all four endings: the gate is no
        // longer polled ONLY at the dialog-choice confirm sites, so a
        // closing beat (the 助教 finale nextLines OR these 自白) is always
        // read before the ending snaps (the owner's 「不要突然按下選項後跳
        // 結局」). No-op outside Ch4 / while a box is up / once resolved.
        TryOpenEndingConfession(*player, world_.Dialog(),
                                world_.Semester().Current());
        CheckEndingGates(bus_, *player, world_.Semester(), world_.Dialog());
    }

    // End-of-frame deferred deletion (the terminal pipeline stage). Same
    // operation, same frame timing as the prior inline sweep — runs AFTER
    // the interact / gate logic so an object a gate just marked dead is
    // reaped this frame.
    sweep_.Run(ctx, dt);

    // Cycle 10.P0b (L8 fix): close the same-frame transition window
    // for the npcs[] list. Any FSM transition produced by the
    // position-trigger branches above (CheckChapterGates on
    // Flag_LeaveInterlude consumed -> Transition; Chapter2/3 Clear
    // flags consumed -> Transition; future) must be rolled into the
    // ROSTER BEFORE View::Draw paints this frame. Pre-fix, state.jsonl
    // for transition frames N had semester=NEW but npcs[]=OLD
    // (cycle9f §G.1 / §J): one of the 7 spine transitions per ending.
    //
    // SettleSideEffects (player pos, consumables, arrival hint, etc.)
    // intentionally stays at TOP-of-Update so the harness's lastWorld
    // snapshot sees the OLD player position for one tick — matches
    // the pre-fix harness contract exactly, which the script's
    // `interact <coord>` "arrived" detection depends on (a player
    // teleport on the SAME frame as the umbrella claim would make
    // the next frame's plan resolution think the target is still far
    // away, stalling the script). View-only here.
    sceneRouter_.SettleRoster(world_);
}

// P3 step 1a (was a 25-LOC inline body): the ending screen's bottom
// 3-option menu lives in `controller/screens/EndingScreen.{h,cpp}` now.
// Method retained as a thin delegate so the orchestrator early-return
// chain at the top of Update() reads unchanged.
bool GameController::HandleEndingMenu() {
    return nccu::HandleEndingMenu(world_);
}

// P3 step 1b (was an 80-LOC inline body): the M pause menu + 說明 help
// overlay live in `controller/screens/PauseScreen.{h,cpp}` now. Method
// retained as a thin delegate so the orchestrator early-return chain at
// the top of Update() reads unchanged.
bool GameController::HandlePauseMenu() {
    return nccu::HandlePauseMenu(world_);
}

// P3 step 1e (was a 200-LOC inline body): the dialog freeze + vendor
// confirm + finale choice locks + per-choice gates live in
// `controller/screens/DialogScreen.{h,cpp}` now. Method retained as a
// thin delegate so the orchestrator early-return chain at the top of
// Update() reads unchanged; pendingVendor_, input_, and sceneRouter_
// are passed by reference so the free function can mutate them.
bool GameController::HandleDialog() {
    return nccu::HandleDialog(bus_, world_, pendingVendor_, input_, sceneRouter_);
}


// P3 step 1c (was a 60-LOC inline body): the Tab inventory overlay lives
// in `controller/screens/InventoryScreen.{h,cpp}` now. Method retained
// as a thin delegate so the orchestrator early-return chain at the top
// of Update() reads unchanged.
bool GameController::HandleInventory() {
    return nccu::HandleInventory(bus_, world_);
}

// P3 step 1d (was a 94-LOC inline body): the E-interact dispatch lives in
// `controller/InteractDispatch.{h,cpp}` now. Method retained as a thin
// delegate so Update()'s call site reads unchanged; `pendingVendor_` is
// passed by reference so the free function can set it when an E tap
// opens a shop menu.
void GameController::DispatchInteract() {
    nccu::DispatchInteract(bus_, world_, pendingVendor_);
}

} // namespace nccu
