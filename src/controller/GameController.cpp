#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogOpener.h"
#include "state/EndingGate.h"
#include "quest/ChapterGate.h"
#include "ui/ChapterToast.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"
#include "quest/ItemCatalog.h"
#include "quest/NpcSpawns.h"     // G-2: IsChapter1FlavorNpc routing
#include "ui/EndingView.h"        // A-T3: IsEndingState + the ending-menu map
#include "ui/GameHelp.h"          // kGameHelpPageCount (paged 說明 nav)
#include "ui/InventoryView.h"     // kInventoryRowsPerPage (paged bag nav)
#include "vendor/Vendor.h"
#include "state/InterludeExit.h"
#include "controller/GameObjectQueries.h"
#include "engine/events/EventBus.h"
#include "controller/EventWiring.h"
#include "controller/SimSystem.h"
#include "quest/QuestHookTable.h"
#include "world/Physics.h"
#include "world/WorldConfig.h"
#include "gfx/Bounds.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
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
    // Item 5a — once-only self-flagging choices. A choice that grants a
    // ONE-OFF reward expresses it as "karma +N AND set this flag true";
    // the Ch1 福利社阿姨 (d) 請咖啡 (karma +5, Flag_BoughtCoffeeForAuntie_
    // Ch1) is exactly this shape. shop_auntie is a re-enterable choice-
    // opener (its (b)/(c) 詢問傘/醜傘 are inert flavour with karma +0 / no
    // flag, so re-talking them is harmless and stays allowed), but WITHOUT
    // this guard a player could re-pick 請咖啡 every visit and farm +5
    // each time. So: if the choice would SET a flag the player ALREADY
    // holds, the reward has already been collected — skip BOTH the karma
    // and the (idempotent) flag write. This is the karma-application spot
    // the suit_senior / ta self-locks (Flag_SuitSeniorChoiceMade /
    // Flag_TaFinaleChoiceMade) protect structurally by never re-presenting
    // their menus; shop_auntie's menu IS re-presented, so the guard lives
    // here instead. A clearing choice (flagValue=false) or a flag the
    // player does not yet hold applies normally (first pick still rewards).
    if (!choice.setsFlag.empty() && choice.flagValue &&
        player.HasFlag(choice.setsFlag)) {
        return;   // reward already collected — no double-dip
    }
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
      sceneRouter_(world.Semester().Current()) {
    frameColliders_.reserve(64);  // dynamic actors only; terrain is the mask
    WireDefaultSubscribers(EventBus::Instance(), world_.Semester(),
                           world_.CurrentBuildingName());
    // Additional ShowMessage subscriber: mirrors the event text into
    // world_ for the transient HUD banner. Same lifetime as the
    // subscribers above — torn down by the ~GameController Clear() below.
    WireHudMessageSubscriber(EventBus::Instance(), world_);
    // Cycle 9.B H5: karma changes feed the same HUD banner via a
    // KarmaChanged -> ShowMessage hop. Wired AFTER the HUD subscriber
    // so the order of registration matches the intent (KarmaChanged
    // re-publishes ShowMessage; the HUD subscriber sees the resulting
    // ShowMessage on the next dispatch — both subscribers live for the
    // controller's lifetime and are torn down by the same Clear()).
    WireKarmaToastSubscriber(EventBus::Instance());

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
    EventBus::Instance().Clear();
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
            MaybeAnnounceInterludeExit(sceneRouter_.InterludeExitLatchMut());
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
        LiftChapter1Clear(*player, world_.Semester().Current(),
                          world_.Dialog());
        // S5c-2: lift Flag_Ch2Cleared only once 學霸 is recovered AND
        // the (d) thanks dialog has closed (deferred so the gate does
        // not close that dialog the frame it opens). Runs BEFORE
        // CheckChapterGates so the lifted flag is consumed the same
        // frame. No-op outside Ch2 / before recovery.
        LiftChapter2Clear(*player, world_.Semester().Current(),
                          world_.Dialog());
        CheckChapterGates(*player, world_.Semester(), world_.Dialog());
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
        CheckEndingGates(*player, world_.Semester(), world_.Dialog());
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

// A-T3: the ENDING SCREEN is a stable, interactive screen with a bottom
// 3-option menu (回首頁 / 重新開始 / 結束). Handled FIRST and the world is
// fully FROZEN here (the orchestrator returns before the pipeline /
// interact / sweep, exactly like the dialog/inventory/pause freezes) —
// once an ending fires the player's ONLY agency is this menu. ←/→ move the
// cursor; E/Enter confirm the highlighted choice and route it through
// World::PendingAppAction (the SINGLE place a full World teardown+rebuild
// can safely happen — BUGLEDGER B2/H1; the controller never tears itself
// down). 回首頁 and 重新開始 both request Restart (main.cpp tears the run
// down to the title — a full state reset; for 重新開始 the player then
// re-enters from the title, the accepted "Restart→title→select" flow);
// 結束 requests Quit (the ONLY path that closes the window). ESC is NOT
// read here — it stays the program's quit key owned by main.cpp. This
// block is reached only in an Ending_* state, so normal play AND every
// pre-A-T3 scripted harness run that never reaches an ending are
// byte-identical (the smoke gate stays in Ch1).
bool GameController::HandleEndingMenu() {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    if (!IsEndingState(world_.Semester().Current())) return false;
    if (Input::IsPressed(Key::Left))  world_.MoveEndingMenuCursor(-1);
    if (Input::IsPressed(Key::Right)) world_.MoveEndingMenuCursor(1);
    if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
        switch (EndingMenuChoiceAt(world_.EndingMenuCursor())) {
            case EndingMenuChoice::BackToTitle:
                // Back to the title screen (full teardown → title).
                world_.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::RestartGame:
                // A fresh new game: same Restart teardown → title, from
                // which the player starts Ch1 anew (state fully reset by
                // the World rebuild; CLAUDE.md "must fully reset state").
                world_.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::Quit:
                // True quit — the only path that closes the canvas.
                world_.RequestAppAction(World::AppAction::Quit);
                break;
        }
    }
    return true;   // frozen on the ending screen until an option is chosen
}

// In-game pause menu (top-right). Opens with M; while open the world is
// fully frozen (the orchestrator returns before the object tick /
// movement / rain / sweep, exactly like the dialog/inventory freezes).
// Checked AFTER the ending screen so the pause takes precedence over a
// dialog or the Tab inventory. M also closes it (toggle / Resume), so it
// is reachable and dismissable with one key. ESC is deliberately NOT a
// menu key — it is the program's direct-quit key (raylib's default exit
// key drives main.cpp's WindowShouldClose loops), so it must never be
// consumed here. Restart/Quit only RECORD an intent on the World — the
// actual World teardown+rebuild happens in main.cpp's outer loop, the
// single place that can re-run the RAII-ordered composition root without
// leaking EventBus subscribers (BUGLEDGER B2/H1). The controller never
// destroys itself. Returns true while the menu (or its help overlay) is
// up, OR on the frame M opens it; false when no menu is involved.
bool GameController::HandlePauseMenu() {
    {
        using nccu::gfx::Input;
        using nccu::gfx::Key;
        const bool toggle = Input::IsPressed(Key::M);
        if (world_.MenuOpen()) {
            // REQUIREMENT #9: the 說明 help overlay sits ON TOP of the
            // paused menu. While it is up, M / E / Enter dismisses it
            // back to the menu and the menu cursor / sim stay frozen.
            // (ESC is not a dismiss key — it quits the program.) Handled
            // FIRST so a key meant for "close help" never also moves the
            // menu cursor or triggers an AppAction.
            if (world_.HelpOpen()) {
                // U2-T4: the 說明 overlay is paged — ←/→ flip between the
                // 操作+目標 page and the 雨傘外觀+道具須知+結局 page (the
                // page index wraps; the View draws a 「第 N／M 頁」 indicator).
                // Pure UI state (World::HelpPage, NOT serialized — see
                // Harness.cpp), so a paged help leaves state.jsonl byte-
                // identical. M / E / Enter still dismisses back to the menu.
                constexpr int n = nccu::kGameHelpPageCount;
                if (Input::IsPressed(Key::Right))
                    world_.SetHelpPage((world_.HelpPage() + 1) % n);
                if (Input::IsPressed(Key::Left))
                    world_.SetHelpPage((world_.HelpPage() - 1 + n) % n);
                if (Input::IsPressed(Key::M) ||
                    Input::IsPressed(Key::Enter) ||
                    Input::IsPressed(Key::E))
                    world_.SetHelpOpen(false);
                return true;                    // frozen behind help
            }
            if (Input::IsPressed(Key::Up))   world_.MoveMenuCursor(-1);
            if (Input::IsPressed(Key::Down)) world_.MoveMenuCursor(1);
            if (toggle) {                       // M = quick Resume
                world_.SetMenuOpen(false);
                return true;
            }
            if (Input::IsPressed(Key::Enter)) {
                switch (world_.MenuCursor()) {
                    case 0:                     // 繼續 (Resume)
                        world_.SetMenuOpen(false);
                        break;
                    case 1:                     // 說明 (Help) — overlay
                        world_.SetHelpOpen(true);
                        break;
                    case 2:                     // 減少動畫 (toggle)
                        // Cycle 9.E.3: pause-menu UI for the
                        // ReducedMotion accessibility flag added in
                        // 9.E.1. Flip in place; the menu stays open so
                        // the player can see the [開]/[關] state update
                        // on the same row their cursor is on. Pure
                        // World mutation — no AppAction, no menu close.
                        world_.SetReducedMotion(!world_.ReducedMotion());
                        break;
                    case 3:                     // 擴大目標 (toggle)
                        // Cycle 9.E.3: pause-menu UI for the
                        // LargeTargets accessibility flag added in
                        // 9.E.2. Same in-place toggle shape as row 2 —
                        // the next gameplay frame's E-probe reach picks
                        // up the new value via World::LargeTargets().
                        world_.SetLargeTargets(!world_.LargeTargets());
                        break;
                    case 4:                     // 重新開始 (Restart)
                        world_.RequestAppAction(
                            World::AppAction::Restart);
                        break;
                    default:                    // 離開 (Quit)
                        world_.RequestAppAction(
                            World::AppAction::Quit);
                        break;
                }
            }
            return true;   // frozen while the menu is up
        }
        if (toggle) {                           // open from gameplay
            world_.SetMenuOpen(true);
            return true;
        }
    }
    return false;   // no menu involved this frame — fall through
}

// Dialog freeze: while a conversation is open the world is paused — the
// orchestrator runs ONLY this handler and skips the object tick /
// movement / collision / building-entry / sweep. IsKeyPressed is edge-
// triggered, so the E that opened the box (handled in DispatchInteract
// last frame) does not auto-advance it this frame. Also routes a confirmed
// vendor stock line to Vendor::TryBuy. Returns true while a conversation
// is open (the world stays frozen), false when no dialog is active.
bool GameController::HandleDialog() {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    using nccu::gfx::Time;
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
            // Cycle 9.E (audit H2 / D5 / SC 2.2.2): hold-E to fast-advance
            // dialog. Tap-press (edge IsPressed) keeps its existing
            // mash-once-per-press semantics; HOLDING E for >= 300 ms then
            // also fires the same advance branch every ~4 frames so a
            // long-winded NPC can be skimmed without finger pain. The
            // gameplay E-probe (out of this dialog branch, line ~432) is
            // edge-only and untouched — held-E only auto-advances dialog,
            // never re-triggers interactions / vendor menus.
            //
            // Cycle 10.P0a: the edge/hold edge timing now lives on
            // InputHandler::TickDialogAdvance — the exact same edge OR
            // ≥300 ms held + 4-frame cooldown contract, pinned by
            // test_input_handler. The Controller asks "should advance
            // this frame?" and acts.
            const float ddt = Time::DeltaSeconds();
            const bool advanceE = input_.TickDialogAdvance(ddt);
            if (advanceE) {
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
                            // Cycle 10.P0b: roster settle picks up any
                            // FSM transition (none expected here, but
                            // cheap insurance against a future side
                            // effect inside Close()). View-only — does
                            // not mutate player.pos / flags / events,
                            // so the harness state.jsonl observable
                            // timeline is unchanged.
                            sceneRouter_.SettleRoster(world_);
                            return true;
                        }
                        (void)pendingVendor_->TryBuy(p, stockIdx);
                        pendingVendor_ = nullptr;
                        // G2: do NOT resolve the ending here. A confirmed
                        // stock pick has no nextLines, so the vendor box is
                        // already CLOSED at this point — calling
                        // CheckEndingGates now would snap Ending C the same
                        // frame the 醜傘 is bought, with no closing beat
                        // (the owner's abrupt-ending complaint). Instead the
                        // non-dialog poll (end of Update) runs
                        // TryOpenEndingConfession first → opens the 務實 自白
                        // → CheckEndingGates defers behind it → C fires once
                        // the player closes the monologue. CheckChapterGates
                        // stays (a buy is never a chapter-clear trigger, so
                        // it is the same cheap no-op insurance as before).
                        CheckChapterGates(*p, world_.Semester(), dlg);
                        // Cycle 10.P0b (L8 fix): the gate calls above
                        // can Transition() — settle the roster NOW so
                        // the frame the player sees has a coherent
                        // npcs[]. Pre-fix, the respawn waited for the
                        // NEXT frame's top-of-Update() check. View-only:
                        // SettleSideEffects (player pos, consumables,
                        // events) still runs at top of NEXT Update so
                        // the harness state.jsonl is byte-identical.
                        sceneRouter_.SettleRoster(world_);
                        return true;
                    }
                    ApplyDialogChoice(*p, *c);
                    // 1c: the trailing "我再想想…" exit (kDialogExitLabel)
                    // is a no-commit back-out — it must NOT trip any
                    // menu's post-choice bookkeeping below. ApplyDialog
                    // Choice was already a no-op for it (zero karma /
                    // empty flag); the guard here additionally stops the
                    // 助教 finale self-lock from firing, so the moral
                    // choice stays UNMADE and re-approachable (mirrors the
                    // vendor decline's "no side effect" contract).
                    const bool exitChoice = c->label == kDialogExitLabel;
                    // C.3(b): a confirmed 西裝學長 choice locks the
                    // branch menu so re-talking can't stack mutually-
                    // exclusive ripple flags. DialogOpener reads this
                    // flag and recaps line-only thereafter.
                    if (!exitChoice && npc == "suit_senior")
                        p->SetFlag(kFlagSuitSeniorChoiceMade);
                    // S5e-2d: a confirmed 助教 (d) 結算 choice in Ch4
                    // locks the menu (one-shot, like C.3(b)) so the
                    // moral choice (體諒 → Flag_ConsoledTA, +15) can't
                    // be flipped/re-applied on a re-talk. ApplyDialog
                    // Choice already set Flag_ConsoledTA + karma; the
                    // CheckEndingGates below routes Ending A if its
                    // karma>80 + TrueUmbrella conditions also hold. The
                    // 1c exit is excluded so backing out does NOT set
                    // Flag_TaFinaleChoiceMade (no premature Ending C/B).
                    if (!exitChoice && npc == "ta" &&
                        world_.Semester().Current() ==
                            SemesterState::Chapter4_Finals) {
                        p->SetFlag(kFlagTaFinaleChoiceMade);
                        // T4: the gentle finale returns YOUR umbrella. When
                        // the player chose 體諒 (ApplyDialogChoice just set
                        // Flag_ConsoledTA), the 助教 presses the true
                        // umbrella back — TryGrantTaFinaleUmbrella sets
                        // Flag_HasTrueUmbrella + HasUmbrella so the gentle
                        // path can reach Ending A WITHOUT also finding the
                        // hidden Ch4 umbrella (both routes now reach A;
                        // EndingGate keeps the karma>80 gate). The harsh
                        // 質問 branch never sets Flag_ConsoledTA, so the
                        // helper no-ops and that path resolves to Ending B
                        // (coldFinale). The spoken "拿回你的傘" beat lives in
                        // the 體諒 choice's nextLines (DialogOpener T4).
                        TryGrantTaFinaleUmbrella(
                            *p, npc, world_.Semester().Current());
                    }
                    // B3: the Ch1 福利社阿姨 (c) 購買醜綠傘 is a REAL buy.
                    // The DialogChoice itself carries no money/umbrella (it
                    // stays a pure choice-opener entry — setsFlag "", karma
                    // 0, so the existing test_dialog_opener assertions hold);
                    // the economy lands HERE, attributed by npc + the chosen
                    // label, mirroring how the 助教 finale grants the true
                    // umbrella on confirm. Deducts 80 元 + grants the held
                    // ugly umbrella with a 花費/餘額 toast; does NOT set
                    // Flag_BoughtUglyUmbrella (that is the Ch4 Vendor's
                    // Ending-C lock). No-op for the 阿姨's other choices and
                    // for the 1c exit (label mismatch). Idempotent (already
                    // holding Ugly → no re-deduct) and fund-guarded inside.
                    if (!exitChoice)
                        (void)TryBuyAuntieUglyUmbrella(
                            *p, npc, c->label, world_.Semester().Current());
                    // Ending gates first, then chapter gates (existing
                    // precedent: EndingGate predates this). Order is safe
                    // either way — once an ending fires, Current() is
                    // Ending_X and none of CheckChapterGates' Ch2/Ch3/
                    // Interlude sibling-ifs can match.
                    CheckEndingGates(*p, world_.Semester(), dlg);
                    CheckChapterGates(*p, world_.Semester(), dlg);
                }
            }
            // Cycle 10.P0b (L8 fix): any transition the dialog branch
            // produced (CheckEndingGates / CheckChapterGates) must
            // roll its roster into View BEFORE the frame draws. The
            // early-return paths above already called SettleRoster
            // themselves; this one covers the fall-through path (a
            // non-purchase, non-terminal advance). View-only: the
            // harness-observable side effects (player pos, consumables,
            // events) wait until top-of-next-Update's SettleSideEffects
            // so state.jsonl stays byte-identical.
            sceneRouter_.SettleRoster(world_);
            return true;
        } else {
            // Dialog not active this tick: drop any stale hold-E
            // accumulation so the next conversation starts fresh.
            // Cheap; idempotent — InputHandler tracks IsDown(E) itself
            // and would reset on release anyway, but an explicit drop
            // here keeps the contract obvious.
            input_.ResetDialogAdvance();
        }
    }
    return false;   // no dialog active — fall through
}

// Tab inventory overlay (S5b-5 + Item 2). Edge-triggered toggle, then —
// while open — freeze the sim exactly like the dialog box (the
// orchestrator returns before the pipeline / interact / sweep). Checked
// AFTER the dialog handler so a conversation has priority and Tab can't
// pop the panel mid-dialog. Returns true while the bag is open, false
// otherwise.
bool GameController::HandleInventory() {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    if (Input::IsPressed(Key::Tab))
        world_.SetInventoryOpen(!world_.InventoryOpen());
    if (world_.InventoryOpen()) {
        // Item 2(b): the bag is a hold-and-use list. ↑/↓ move the cursor;
        // E/Enter on a CONSUMABLE row uses it (applies the SAME effect the
        // pickup used to fire, then decrements the count); on a view-only
        // row (金幣 / 雨傘 / 任務紙張) E/Enter is inert — the View already
        // shows that row's description. The rows are rebuilt from the
        // Player each frame the bag is open (BuildInventoryRows), so a row
        // that hits 0 after use disappears next frame and the cursor is
        // re-clamped. Normal movement / interact never run here (the early
        // return below freezes the sim), so opening the bag can't move the
        // player or re-trigger NPCs — only Tab (handled above) re-closes
        // it. Build the rows ONCE; act on the captured snapshot.
        if (Player* invP = world_.GetPlayer()) {
            const std::vector<InventoryRow> rows = BuildInventoryRows(*invP);
            const int n = static_cast<int>(rows.size());
            int cur = world_.InventoryCursor();
            if (n > 0) {
                if (cur < 0)   cur = 0;
                if (cur >= n)  cur = n - 1;
                if (Input::IsPressed(Key::Up))   cur = (cur - 1 + n) % n;
                if (Input::IsPressed(Key::Down)) cur = (cur + 1) % n;
                // U2-T1: ←/→ jump a whole PAGE (the View pages the bag once
                // rows exceed kInventoryRowsPerPage). The page index is
                // DERIVED from the cursor render-side, so moving the cursor
                // by ±a page is all that is needed — the shown page follows.
                // Up/Down already flip the page when the selection crosses a
                // boundary; ←/→ are the explicit fast path. Clamped (no
                // wrap) so a page-jump can't skip past the ends. No new
                // serialized state — InventoryCursor is not in state.jsonl.
                if (Input::IsPressed(Key::Right))
                    cur = std::min(n - 1, cur + nccu::kInventoryRowsPerPage);
                if (Input::IsPressed(Key::Left))
                    cur = std::max(0, cur - nccu::kInventoryRowsPerPage);
                world_.SetInventoryCursor(cur);
                if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
                    const InventoryRow& sel = rows[static_cast<std::size_t>(cur)];
                    if (sel.usable && IsUsableConsumable(sel.itemId) &&
                        invP->ConsumableCount(sel.itemId) > 0) {
                        // Apply the effect, THEN spend one. Order matters
                        // for nothing here (the effect doesn't read the
                        // count), but spending after keeps the "use → it's
                        // gone" reading obvious. ApplyConsumableEffect
                        // publishes the same flavour ShowMessage the pickup
                        // path used to.
                        ApplyConsumableEffect(*invP, sel.itemId);
                        (void)invP->ConsumeOne(sel.itemId);
                    }
                }
            } else {
                world_.SetInventoryCursor(0);
            }
        }
        return true;
    }
    return false;   // bag closed — fall through
}

// E-interact dispatch (talk / pick up / open a shop). Reads the E edge
// and the player's reach box, so it stays in the controller layer. For a
// non-flavor NPC it runs the registered QuestHook table (RunInteractHooks)
// — the ~14 inline TryXxx calls are now DATA (quest/QuestHookTable.cpp),
// in the SAME order and with the SAME self-gating semantics — then opens
// the per-(npcId, state) dialog. A flavor NPC short-circuits to its own
// line-cycling Interact; a non-NPC (pickup / Vendor) routes through its
// IInteractable role. Same operations, same order as the inline block.
void GameController::DispatchInteract() {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    using nccu::gfx::Rect;
    using nccu::queries::ForEachActiveExcept;
    Player* player = world_.GetPlayer();
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
        // Cycle 9.E (audit M2 / D7 / SC 2.5.8): a "larger targets"
        // accessibility profile (World::LargeTargets(),
        // UMBRELLA_LARGE_TARGETS=1) widens the reach to 16 px on each
        // side — effective talk box 56x56 instead of 40x40 — so a
        // tremor-affected player can still trigger NPC dialog without
        // pixel-precise alignment. The MOVEMENT collider above is
        // unchanged (the player still cannot walk through an NPC); ONLY
        // the E-probe reach grows. Default off ⇒ byte-equivalent to
        // pre-9.E behaviour and to the prior `kInteractReach = 8.0f`.
        const float kInteractReach = world_.LargeTargets() ? 16.0f : 8.0f;
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
                    // G-2: a Ch1 stationary FLAVOR NPC (搶課同學 / 撐傘路人 /
                    // 揹書包學生) cycles its own chapter1.md line pool one line
                    // per talk via NPC::Interact() — a deterministic,
                    // reproducible pick (no RNG on the state path). Route it
                    // HERE, BEFORE any spine hook, and return: a flavor NPC
                    // must never reach TryReturnVictimUmbrella / OpenNpcDialog
                    // etc., so it provably sets NO quest flag and cannot
                    // perturb the hard-gated 苦主→學長→苦主 spine. (The dialog
                    // it shows is a one-line ShowMessage toast, same channel
                    // as an ambient-pedestrian Interact, not the dialog box.)
                    if (nccu::IsChapter1FlavorNpc(id)) {
                        if (auto* it = o.AsInteractable()) it->Interact(player);
                        return;
                    }
                    // Run the registered Ch1-Ch4 quest hooks, IN ORDER,
                    // BEFORE the dialog opener — exactly the inline TryXxx
                    // sequence this replaces (TryReturnVictimUmbrella ->
                    // TryRescueBookworm -> TryMeetLibrarian ->
                    // TryLendLibrarianUmbrella -> TryReturnLibrarianUmbrella
                    // -> TryApplyCh2Ripple -> TryAdvanceCh3Trade ->
                    // TryApplyCh3Ripple -> TryApplyCh4Ripple). Each hook
                    // self-gates on (state, npcId) and is a cheap no-op
                    // outside its chapter, so running the whole table per
                    // interact is correct and order-stable. The 4th arg is
                    // the Interlude return-target (only the librarian-return
                    // hook scopes itself with it). Adding a chapter/NPC is
                    // now a RegisterHook line, not an edit here (OCP).
                    RunInteractHooks(*player, id,
                                     world_.Semester().Current(),
                                     world_.Semester().InterludeReturnTo());
                    OpenNpcDialog(world_.Dialog(), *player, id,
                                  world_.Semester().Current());     // talk
                } else if (auto* it = o.AsInteractable()) {
                    it->Interact(player);                            // pick up / Vendor
                }
            });
    }
}

} // namespace nccu
