#include "controller/screens/DialogScreen.h"
#include "controller/DialogChoiceApply.h"   // ApplyDialogChoice
#include "controller/InputHandler.h"
#include "controller/SceneRouter.h"
#include "controller/VendorMenu.h"         // kVendorContext
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogOpener.h"            // kDialogExitLabel
#include "quest/ChapterGate.h"
#include "quest/Chapter1Quest.h"            // TryBuyAuntieUglyUmbrella
#include "quest/Chapter4Quest.h"            // TryGrantTaFinaleUmbrella
#include "quest/Flags.h"
#include "state/EndingGate.h"
#include "state/SemesterStateMachine.h"
#include "state/SemesterState.h"
#include "vendor/Vendor.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include <cstddef>
#include <string>

namespace nccu {

bool HandleDialog(EventBus& bus, World& world, Vendor*& pendingVendor,
                  InputHandler& input, SceneRouter& sceneRouter) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    using nccu::gfx::Time;
    DialogState& dlg = world.Dialog();
    // I5: drop the pending-vendor target the instant its menu is no
    // longer the open conversation (closed greeting-only, advanced
    // past with no stock, replaced by an NPC dialog, swept by a
    // chapter transition). Keeps the non-owning Vendor* from ever
    // outliving the World object it points at — once cleared, the
    // confirm branch's `pendingVendor && ...` guard is inert.
    if (pendingVendor &&
        (!dlg.Active() || dlg.NpcId() != kVendorContext))
        pendingVendor = nullptr;
    if (dlg.Active()) {
        Player* p = world.GetPlayer();
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
        const bool advanceE = input.TickDialogAdvance(ddt);
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
                if (npc == kVendorContext && pendingVendor &&
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
                        pendingVendor->Config().stock.size();
                    if (stockIdx >= stockN) {        // decline
                        pendingVendor = nullptr;
                        dlg.Close();
                        // Cycle 10.P0b: roster settle picks up any
                        // FSM transition (none expected here, but
                        // cheap insurance against a future side
                        // effect inside Close()). View-only — does
                        // not mutate player.pos / flags / events,
                        // so the harness state.jsonl observable
                        // timeline is unchanged.
                        sceneRouter.SettleRoster(world);
                        return true;
                    }
                    (void)pendingVendor->TryBuy(p, stockIdx);
                    pendingVendor = nullptr;
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
                    CheckChapterGates(bus, *p, world.Semester(), dlg);
                    // Cycle 10.P0b (L8 fix): the gate calls above
                    // can Transition() — settle the roster NOW so
                    // the frame the player sees has a coherent
                    // npcs[]. Pre-fix, the respawn waited for the
                    // NEXT frame's top-of-Update() check. View-only:
                    // SettleSideEffects (player pos, consumables,
                    // events) still runs at top of NEXT Update so
                    // the harness state.jsonl is byte-identical.
                    sceneRouter.SettleRoster(world);
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
                    world.Semester().Current() ==
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
                        *p, npc, world.Semester().Current());
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
                        bus, *p, npc, c->label, world.Semester().Current());
                // Ending gates first, then chapter gates (existing
                // precedent: EndingGate predates this). Order is safe
                // either way — once an ending fires, Current() is
                // Ending_X and none of CheckChapterGates' Ch2/Ch3/
                // Interlude sibling-ifs can match.
                CheckEndingGates(bus, *p, world.Semester(), dlg);
                CheckChapterGates(bus, *p, world.Semester(), dlg);
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
        sceneRouter.SettleRoster(world);
        return true;
    } else {
        // Dialog not active this tick: drop any stale hold-E
        // accumulation so the next conversation starts fresh.
        // Cheap; idempotent — InputHandler tracks IsDown(E) itself
        // and would reset on release anyway, but an explicit drop
        // here keeps the contract obvious.
        input.ResetDialogAdvance();
    }
    return false;   // no dialog active — fall through
}

} // namespace nccu
