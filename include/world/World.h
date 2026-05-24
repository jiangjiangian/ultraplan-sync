#ifndef WORLD_H_
#define WORLD_H_
#include "entities/GameObject.h"
#include "ui/HudSlot.h"
#include "ui/MessageView.h"
#include "entities/Player.h"
#include "state/SemesterState.h"
#include "state/SemesterStateMachine.h"
#include "world/BuildingTracker.h"
#include "world/CollisionMask.h"
#include "dialog/DialogState.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nccu {

// The game's data model — owns every GameObject, the semester FSM, the
// building tracker and the terrain collision mask. No rendering, no input:
// the View reads it const, the GameController mutates it. player_ is a
// non-owning cache of the front object, dropped by the end-of-frame
// sweep when the player is removed.
class World {
public:
    // loadSprites: production passes the default true. A headless unit
    // test (no GL context) passes false so chapter-NPC sprite loads skip
    // the GPU upload that would otherwise SIGSEGV — same kind of
    // headless accommodation as LoadTerrainMask() degrading to an empty
    // mask without assets. The Player/umbrella loads are already safe
    // (their texture path file-not-found-early-outs before any GPU call).
    explicit World(const std::string& playerSpritePath,
                   bool loadSprites = true);

    World(const World&)            = delete;
    World& operator=(const World&) = delete;

    using ObjectList = std::vector<std::unique_ptr<GameObject>>;

    [[nodiscard]] ObjectList&       Objects()       noexcept { return objects_; }
    [[nodiscard]] const ObjectList& Objects() const noexcept { return objects_; }

    [[nodiscard]] Player*       GetPlayer()       noexcept { return player_; }
    [[nodiscard]] const Player* GetPlayer() const noexcept { return player_; }
    void                        ClearPlayer()     noexcept { player_ = nullptr; }

    [[nodiscard]] SemesterStateMachine&       Semester()       noexcept { return semester_; }
    [[nodiscard]] const SemesterStateMachine& Semester() const noexcept { return semester_; }
    [[nodiscard]] BuildingTracker&            Tracker()        noexcept { return tracker_; }

    // 操場 校慶 lap (Ch3): tick once per frame from the controller. Reads
    // the player position, accumulates the angle swept around the track
    // centre, and sets Flag_SportsLapDone after a full lap. Self-gates on
    // Chapter3 / not-yet-done, so it is a cheap no-op elsewhere.
    void UpdateSportsLap() noexcept;
    // Lap completion fraction [0,1] for the track-ring + HUD-ring render.
    [[nodiscard]] float SportsLapProgress() const noexcept;
    // True while the lap ring should be drawn (Ch3 and not yet completed).
    [[nodiscard]] bool  SportsLapActive() const noexcept;

    // Ch2 散落筆記 deferred spawn (sibling of UpdateSportsLap): tick once
    // per frame from the controller. The 3 notes are NOT spawned at
    // chapter entry — they appear ONLY after the player wakes the 學霸
    // (Flag_Bookworm), which is the moment he asks for them. Self-gates on
    // Chapter2 / wake-flag / not-yet-spawned, so it is a cheap no-op
    // otherwise and fires its spawn pass exactly once per Ch2 visit
    // (ch2NotesSpawned_ guard). The notes are roster-tracked exactly like
    // an entry spawn, so an uncollected note is still swept on the next
    // state change. Returns true on the frame it spawns (for the test).
    bool MaybeSpawnChapter2Notes();

    // A1: Ch1 victim's-umbrella reveal-after-choice deferred spawn (sibling
    // of MaybeSpawnChapter2Notes / MaybeSpawnChapter3Umbrella): tick once per
    // frame from the controller. The 苦主's transparent umbrella is NOT
    // spawned at chapter entry — it appears ONLY after the player has
    // confronted the 西裝學長 and committed a choice (Flag_SuitSeniorChoiceMade),
    // i.e. once the 學長 reveals where he dropped it. Self-gates on
    // Chapter1 / choice-flag / not-yet-spawned, a cheap no-op otherwise, and
    // fires exactly once per Ch1 visit (ch1VictimUmbrellaSpawned_). Roster-
    // tracked so it is swept on the next state change. Returns true on the
    // frame it spawns (for the test). Before the flag it does NOT exist in the
    // world, so the player cannot grab it before the 學長 step.
    bool MaybeSpawnChapter1VictimUmbrella();

    // T5: Ch3 TrueUmbrella reveal-after-clue deferred spawn (sibling of
    // MaybeSpawnChapter2Notes): tick once per frame from the controller. The
    // umbrella is NOT spawned at chapter entry — it appears ONLY after the
    // C-系 學姊 reveals its location (Flag_KnowsUmbrellaLoc). Self-gates on
    // Chapter3 / clue-flag / not-yet-spawned, a cheap no-op otherwise, and
    // fires exactly once per Ch3 visit (ch3UmbrellaSpawned_). Roster-tracked
    // so it is swept on the next state change. Spawned LEFT of the gym
    // (kChapter3UmbrellaPos) so it is no longer occluded. Returns true on the
    // frame it spawns (for the test).
    bool MaybeSpawnChapter3Umbrella();

    [[nodiscard]] DialogState&       Dialog()       noexcept { return dialog_; }
    [[nodiscard]] const DialogState& Dialog() const noexcept { return dialog_; }

    [[nodiscard]] std::string&       CurrentBuildingName()       noexcept { return currentBuildingName_; }
    [[nodiscard]] const std::string& CurrentBuildingName() const noexcept { return currentBuildingName_; }

    [[nodiscard]] const CollisionMask& TerrainMask() const noexcept {
        return terrainMask_;
    }

    // Tab inventory overlay (S5b-5): pure UI state on the World so View
    // reacts to it and GameController freezes the sim while it is open
    // (same model as the dialog box). Player owns the actual counts.
    // Closing the bag resets the cursor to the top so the next open starts
    // on 金幣 (deterministic; mirrors SetMenuOpen resetting menuCursor_).
    [[nodiscard]] bool InventoryOpen() const noexcept { return inventoryOpen_; }
    void SetInventoryOpen(bool v) noexcept {
        inventoryOpen_ = v;
        if (!v) inventoryCursor_ = 0;
    }
    // Item 2(b): the highlighted bag row. GameController moves it with
    // ↑/↓ while the bag is open and reads it to use the selected
    // consumable on E/Enter; the View reads it to draw the caret +
    // description panel. Pure UI state — no raylib, no input here. The
    // row count is dynamic (it shrinks as items are used), so the cursor
    // is clamped by the consumer (controller clamps before reading, View
    // clamps before drawing) rather than stored against a fixed bound.
    [[nodiscard]] int  InventoryCursor() const noexcept { return inventoryCursor_; }
    void SetInventoryCursor(int v) noexcept { inventoryCursor_ = v; }

    // In-game pause menu (top-right affordance, opened with Esc/M). Pure
    // UI state on the World — exactly the InventoryOpen idiom — so the
    // View renders it and GameController freezes the sim while it is
    // open. World stays pure data: no raylib, no input here. Items:
    // 0=繼續(Resume) 1=重新開始(Restart) 2=離開(Quit). The Restart/Quit
    // intent is surfaced to main.cpp's outer loop via PendingAppAction
    // (the only place a full World teardown+rebuild can safely happen,
    // so the EventBus subscriber lifetime stays controlled — BUGLEDGER
    // B2/H1). GameController NEVER tears itself down.
    enum class AppAction { None, Restart, Quit };
    [[nodiscard]] bool MenuOpen() const noexcept { return menuOpen_; }
    void SetMenuOpen(bool v) noexcept {
        menuOpen_ = v;
        // reopen on Resume: clear the cursor, the help latch AND its page.
        if (!v) { menuCursor_ = 0; helpOpen_ = false; helpPage_ = 0; }
    }
    [[nodiscard]] int  MenuCursor() const noexcept { return menuCursor_; }
    // REQUIREMENT #9 + Cycle 9.E.3: 6 items — 0=繼續 1=說明
    // 2=減少動畫(toggle) 3=擴大目標(toggle) 4=重新開始 5=離開. 說明
    // opens an in-place help overlay (HelpOpen) rather than an AppAction.
    // Rows 2/3 are non-destructive a11y toggles intentionally wedged
    // between 說明 and 重新開始 so the destructive items (Restart/Quit)
    // sit farthest from the cursor's starting position (audit M2/M3).
    static constexpr int kMenuItemCount = 6;
    void MoveMenuCursor(int delta) noexcept {
        menuCursor_ = (menuCursor_ + delta + kMenuItemCount) %
                      kMenuItemCount;
    }

    // REQUIREMENT #9: the in-game 說明 (how-to-play) overlay. Pure UI
    // state on the World — exactly the InventoryOpen / MenuOpen idiom —
    // so the View renders it and GameController keeps the sim frozen
    // while it (and the menu) is up. Opened from the pause-menu 說明
    // item; M/E/Enter closes it back to the menu. No raylib here.
    [[nodiscard]] bool HelpOpen() const noexcept { return helpOpen_; }
    void SetHelpOpen(bool v) noexcept {
        helpOpen_ = v;
        if (v) helpPage_ = 0;   // each open starts on the first help page
    }

    // U2-T4: the in-game 說明 overlay is now PAGED (the help text grew past
    // one panel — a 【道具須知】 economy/tips section was added). The current
    // page index is pure UI state on the World — exactly the MenuCursor /
    // InventoryCursor idiom — so the View reads it to draw the right page +
    // the 「第 N／M 頁」 indicator, and GameController flips it with ←/→ while
    // help is up. NOT serialized (the harness emits no cursor/page — see
    // Harness.cpp), so a paged help leaves state.jsonl byte-identical. The
    // page COUNT lives in the UI layer (GameHelp.h kGameHelpPageCount); the
    // controller wraps the index against it (World stays free of UI headers).
    // No raylib here. Reset to 0 on each SetHelpOpen(true) / SetMenuOpen
    // (false) so a re-open always starts on page 1.
    [[nodiscard]] int  HelpPage() const noexcept { return helpPage_; }
    void SetHelpPage(int v) noexcept { helpPage_ = v; }

    // Cycle 9.E (audit D8 / SC 2.3.3): the reduced-motion accessibility
    // preference. Default false; set true by either the ctor (when
    // UMBRELLA_REDUCED_MOTION=1 is in the environment) or by a future
    // pause-menu UI. Read by View / MessageView via the pure helpers in
    // include/ReducedMotion.h so the interlude marker sweep, the
    // ending-card fade-in and the toast fade-out collapse to instant
    // changes when on. Pure data — no raylib here.
    [[nodiscard]] bool ReducedMotion() const noexcept { return reducedMotion_; }
    void SetReducedMotion(bool v) noexcept { reducedMotion_ = v; }

    // Cycle 9.E (audit M2 / D7 / SC 2.5.8): the "larger targets"
    // accessibility profile. Default false; set true by either the ctor
    // (when UMBRELLA_LARGE_TARGETS=1 is in the environment) or by a
    // future pause-menu UI. Read by GameController::Update and
    // ScriptInput::ResolvePlan to widen `kInteractReach` from 8 px to
    // 16 px on every side of the player's E-probe — i.e. the effective
    // talk-with-NPC box grows from 40x40 to 56x56. The MOVEMENT collider
    // (frameColliders_) is intentionally unchanged: the player still
    // cannot walk through an NPC; only the talk reach grows. Pure data
    // — no raylib here. Mirrors the same shape as ReducedMotion above.
    [[nodiscard]] bool LargeTargets() const noexcept { return largeTargets_; }
    void SetLargeTargets(bool v) noexcept { largeTargets_ = v; }
    [[nodiscard]] AppAction PendingAppAction() const noexcept {
        return pendingAppAction_;
    }
    void RequestAppAction(AppAction a) noexcept { pendingAppAction_ = a; }
    void ClearAppAction() noexcept { pendingAppAction_ = AppAction::None; }

    // A-T3: the ending-screen bottom menu cursor (回首頁 / 重新開始 / 結束).
    // Pure UI state on the World — exactly the MenuCursor idiom — so the
    // EndingView reads it to draw the highlighted option and GameController
    // moves it with ←/→ while an ending is on screen, then maps the chosen
    // index to a World::AppAction (Restart / Quit) via the pure
    // EndingMenuActionAt() helper. The menu is implicitly "open" whenever the
    // semester is an ending state (IsEndingState); there is no separate open
    // flag (the ending screen has no other mode). 0=回首頁 1=重新開始
    // 2=結束. NOT serialized (the harness emits no cursor — see Harness.cpp),
    // so the ending menu leaves state.jsonl byte-identical. No raylib here.
    static constexpr int kEndingMenuItemCount = 3;
    [[nodiscard]] int EndingMenuCursor() const noexcept { return endingMenuCursor_; }
    void SetEndingMenuCursor(int v) noexcept { endingMenuCursor_ = v; }
    void MoveEndingMenuCursor(int delta) noexcept {
        endingMenuCursor_ = (endingMenuCursor_ + delta + kEndingMenuItemCount) %
                            kEndingMenuItemCount;
    }

    // Transient on-screen notice driven by EventType::ShowMessage.
    //
    // Cycle 9.G — TWO independent channels:
    //   HudSlot::Top    -> chapter / ending major-progress toasts
    //                      (ChapterToast / EndingGate publish here).
    //   HudSlot::Bottom -> everything else (pickup / karma / arrival
    //                      hint / vendor / exit prep). DEFAULT slot so
    //                      every pre-9.G publisher stays behaviour-
    //                      identical on the Bottom channel.
    //
    // Before 9.G, a single hudMessage_ slot meant the chapter-clear
    // toast at every Ch->IL transition lived 0.02 s (1 frame) — the IL
    // arrival hint published right after overwrote it (cycle9f §B).
    // Plan A (publish-order swap from 9.B) only solved the TrueUmbrella
    // vs chapter-clear race; the IL arrival hint clobber stayed live
    // until this Plan B split. Both channels age independently via
    // TickHud(dt); the View renders both with their own fade. Pure
    // data — no raylib, no timing source here.
    void SetHudMessage(HudSlot slot, std::string text) {
        if (slot == HudSlot::Top) {
            topHudMessage_ = std::move(text);
            topHudAge_     = 0.0f;
        } else {
            bottomHudMessage_ = std::move(text);
            bottomHudAge_     = 0.0f;
        }
    }
    // Backward-compat overload — default channel is Bottom so call sites
    // that pre-date 9.G (tests, ad-hoc SetHudMessage in restart probes)
    // keep landing on the Bottom slot exactly as they did pre-split.
    void SetHudMessage(std::string text) {
        SetHudMessage(HudSlot::Bottom, std::move(text));
    }
    void TickHud(float dt) noexcept {
        if (!topHudMessage_.empty())    topHudAge_    += dt;
        if (!bottomHudMessage_.empty()) bottomHudAge_ += dt;
    }
    // Cycle 9.E (audit H2 / D5 / SC 2.2.2): force-expire the on-screen
    // HUD toast NOW. Used by the Backspace skip-toast input path so an
    // auto-updating banner is dismissable on demand (SC 2.2.2 expects
    // the player to be able to pause/stop/hide auto-updating content).
    // Snapping hudAge_ to kHudTtl is the same boundary HudExpired()
    // gates on and that DrawHudMessage early-returns above, so no
    // rendering / harness contract changes — the next View pass simply
    // paints nothing for that slot. Pure data; no raylib.
    //
    // Cycle 9.G: dismisses BOTH slots in one call so the SC 2.2.2 skip-
    // toast input stays a single keystroke regardless of how many
    // channels are live. A per-slot DismissHud(slot) overload kept
    // around so a future caller can dismiss only one if needed.
    void DismissHud(HudSlot slot) noexcept {
        if (slot == HudSlot::Top) {
            if (!topHudMessage_.empty())    topHudAge_    = kHudTtl;
        } else {
            if (!bottomHudMessage_.empty()) bottomHudAge_ = kHudTtl;
        }
    }
    void DismissHud() noexcept {
        DismissHud(HudSlot::Top);
        DismissHud(HudSlot::Bottom);
    }
    [[nodiscard]] const std::string& HudMessage(HudSlot slot) const noexcept {
        return slot == HudSlot::Top ? topHudMessage_ : bottomHudMessage_;
    }
    [[nodiscard]] float HudAge(HudSlot slot) const noexcept {
        return slot == HudSlot::Top ? topHudAge_ : bottomHudAge_;
    }
    // Default-slot accessors (Bottom) — keep pre-9.G test code compiling
    // unchanged. Production callers (View, Harness, GameController)
    // pass the slot explicitly so they read BOTH channels.
    [[nodiscard]] const std::string& HudMessage() const noexcept {
        return bottomHudMessage_;
    }
    [[nodiscard]] float HudAge() const noexcept { return bottomHudAge_; }
    // Cycle 9.B L9: an aged-out toast (age >= kHudTtl) is invisible
    // (MessageView's DrawHudMessage already early-returns above the
    // TTL) but its text is still held so the View's fade-out animation
    // contract stays intact (no abrupt clear). HudExpired() lets non-
    // View consumers — primarily the autoplay harness writing
    // state.jsonl — emit an empty hud line for expired toasts instead
    // of echoing a string that hasn't been visible on screen for
    // seconds. Pure read-only predicate; no state mutation, no
    // dependency on rendering.
    [[nodiscard]] bool HudExpired(HudSlot slot) const noexcept {
        if (slot == HudSlot::Top) {
            return !topHudMessage_.empty()    && topHudAge_    >= kHudTtl;
        }
        return     !bottomHudMessage_.empty() && bottomHudAge_ >= kHudTtl;
    }
    // Default-slot HudExpired() — Bottom channel, pre-9.G semantic
    // (test_hud_reset's existing assertions key off this).
    [[nodiscard]] bool HudExpired() const noexcept {
        return HudExpired(HudSlot::Bottom);
    }

    // Make the chapter-NPC roster follow the semester FSM. Removes ONLY
    // the chapter NPCs this method last spawned (tracked by raw pointer
    // in chapterRoster_) with a single deferred remove-erase pass —
    // never mid-iteration — then spawns ChapterNpcSpawns(state). The Player
    // (index 0), the 4 umbrellas, the QuestFlagPickup and the ambient
    // students are never touched, so objects_.front() stays the Player
    // and the cached player_ pointer stays valid. GameController calls
    // this once per detected SemesterState change.
    void RespawnChapterRoster(nccu::SemesterState state);

private:
    // Shared spawn path so the ctor and RespawnChapterRoster build
    // chapter NPCs identically (no construction drift). Appends one NPC
    // per spawn at the back of objects_ and records its raw pointer in
    // chapterRoster_.
    void SpawnChapterNpcs(nccu::SemesterState state);

    // Spawns the ChapterQuestItems(state) QuestFlagPickups (roster-
    // tracked). Shared by the entry path (SpawnChapterNpcs, for any future
    // chapter whose items spawn at entry) and the Ch2 deferred path
    // (MaybeSpawnChapter2Notes). Ch2's notes are deliberately NOT spawned
    // at entry — see MaybeSpawnChapter2Notes.
    void SpawnChapterQuestItems(nccu::SemesterState state);

    ObjectList                  objects_;
    Player*                     player_{nullptr};
    std::vector<GameObject*>    chapterRoster_;
    bool                        loadSprites_{true};
    SemesterStateMachine        semester_;
    BuildingTracker             tracker_;
    DialogState                 dialog_;
    std::string                 currentBuildingName_;
    // Cycle 9.G — two independent HUD channels (see SetHudMessage above
    // for the why). Top: chapter/ending major-progress toasts. Bottom:
    // every other ShowMessage. Each ages independently via TickHud.
    std::string                 topHudMessage_;
    float                       topHudAge_{0.0f};
    std::string                 bottomHudMessage_;
    float                       bottomHudAge_{0.0f};
    CollisionMask               terrainMask_;
    // 操場 lap progress (Ch3): cumulative signed angle swept around the
    // track centre; |Σ| ≥ ~2π (one lap) → Flag_SportsLapDone.
    bool                        lapStarted_{false};
    float                       lapPrevAngle_{0.0f};
    float                       lapSwept_{0.0f};
    // Ch2 散落筆記 one-shot guard: true once MaybeSpawnChapter2Notes has
    // dropped the 3 notes this Ch2 visit, so it never double-spawns. Reset
    // by RespawnChapterRoster (a fresh chapter visit re-arms it), exactly
    // like the lap fields above are conceptually per-Ch3-visit.
    bool                        ch2NotesSpawned_{false};
    // T5: Ch3 TrueUmbrella one-shot guard — true once
    // MaybeSpawnChapter3Umbrella has dropped it this Ch3 visit. Reset by
    // RespawnChapterRoster (a fresh Ch3 visit re-arms it), exactly like
    // ch2NotesSpawned_ above.
    bool                        ch3UmbrellaSpawned_{false};
    // A1: Ch1 victim's-umbrella one-shot guard — true once
    // MaybeSpawnChapter1VictimUmbrella has dropped it this Ch1 visit. Reset
    // by RespawnChapterRoster (a fresh Ch1 visit re-arms it), exactly like
    // ch2NotesSpawned_ / ch3UmbrellaSpawned_ above.
    bool                        ch1VictimUmbrellaSpawned_{false};
    bool                        inventoryOpen_{false};
    int                         inventoryCursor_{0};
    bool                        menuOpen_{false};
    int                         menuCursor_{0};
    bool                        helpOpen_{false};
    int                         helpPage_{0};
    bool                        reducedMotion_{false};
    bool                        largeTargets_{false};
    AppAction                   pendingAppAction_{AppAction::None};
    // A-T3 ending-screen menu cursor (0=回首頁 1=重新開始 2=結束). Pure UI
    // state; not serialized. Starts on 回首頁 so an accidental confirm the
    // frame the ending lands routes to the (non-destructive) title return,
    // not 結束 — same "destructive item farthest from start" principle as
    // the pause menu (audit M2/M3).
    int                         endingMenuCursor_{0};
};

} // namespace nccu

#endif // WORLD_H_
