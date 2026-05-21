#ifndef WORLD_H_
#define WORLD_H_
#include "GameObject.h"
#include "MessageView.h"
#include "Player.h"
#include "SemesterState.h"
#include "SemesterStateMachine.h"
#include "BuildingTracker.h"
#include "CollisionMask.h"
#include "DialogState.h"
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
    [[nodiscard]] bool InventoryOpen() const noexcept { return inventoryOpen_; }
    void SetInventoryOpen(bool v) noexcept { inventoryOpen_ = v; }

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
        if (!v) { menuCursor_ = 0; helpOpen_ = false; }  // reopen on Resume
    }
    [[nodiscard]] int  MenuCursor() const noexcept { return menuCursor_; }
    // REQUIREMENT #9: 4 items — 0=繼續 1=說明 2=重新開始 3=離開. 說明
    // opens an in-place help overlay (HelpOpen) rather than an AppAction.
    static constexpr int kMenuItemCount = 4;
    void MoveMenuCursor(int delta) noexcept {
        menuCursor_ = (menuCursor_ + delta + kMenuItemCount) %
                      kMenuItemCount;
    }

    // REQUIREMENT #9: the in-game 說明 (how-to-play) overlay. Pure UI
    // state on the World — exactly the InventoryOpen / MenuOpen idiom —
    // so the View renders it and GameController keeps the sim frozen
    // while it (and the menu) is up. Opened from the pause-menu 說明
    // item; ESC/E/Enter closes it back to the menu. No raylib here.
    [[nodiscard]] bool HelpOpen() const noexcept { return helpOpen_; }
    void SetHelpOpen(bool v) noexcept { helpOpen_ = v; }

    // Cycle 9.E (audit D8 / SC 2.3.3): the reduced-motion accessibility
    // preference. Default false; set true by either the ctor (when
    // UMBRELLA_REDUCED_MOTION=1 is in the environment) or by a future
    // pause-menu UI. Read by View / MessageView via the pure helpers in
    // include/ReducedMotion.h so the interlude marker sweep, the
    // ending-card fade-in and the toast fade-out collapse to instant
    // changes when on. Pure data — no raylib here.
    [[nodiscard]] bool ReducedMotion() const noexcept { return reducedMotion_; }
    void SetReducedMotion(bool v) noexcept { reducedMotion_ = v; }
    [[nodiscard]] AppAction PendingAppAction() const noexcept {
        return pendingAppAction_;
    }
    void RequestAppAction(AppAction a) noexcept { pendingAppAction_ = a; }
    void ClearAppAction() noexcept { pendingAppAction_ = AppAction::None; }

    // Transient on-screen notice driven by EventType::ShowMessage. The
    // EventBus subscriber (wired by GameController) calls SetHudMessage;
    // GameController::Update ages it via TickHud; the View renders it as
    // a fading banner. Pure data — no raylib, no timing source here.
    void SetHudMessage(std::string text) {
        hudMessage_ = std::move(text);
        hudAge_     = 0.0f;
    }
    void TickHud(float dt) noexcept {
        if (!hudMessage_.empty()) hudAge_ += dt;
    }
    [[nodiscard]] const std::string& HudMessage() const noexcept { return hudMessage_; }
    [[nodiscard]] float              HudAge()     const noexcept { return hudAge_; }
    // Cycle 9.B L9: an aged-out toast (age >= kHudTtl) is invisible
    // (MessageView's DrawHudMessage already early-returns above the
    // TTL) but its text is still held by hudMessage_ so the View's
    // fade-out animation contract stays intact (no abrupt clear).
    // HudExpired() lets non-View consumers — primarily the autoplay
    // harness writing state.jsonl — emit an empty hud line for
    // expired toasts instead of echoing a string that hasn't been
    // visible on screen for seconds. Pure read-only predicate; no
    // state mutation, no dependency on rendering.
    [[nodiscard]] bool HudExpired() const noexcept {
        return !hudMessage_.empty() && hudAge_ >= kHudTtl;
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

    ObjectList                  objects_;
    Player*                     player_{nullptr};
    std::vector<GameObject*>    chapterRoster_;
    bool                        loadSprites_{true};
    SemesterStateMachine        semester_;
    BuildingTracker             tracker_;
    DialogState                 dialog_;
    std::string                 currentBuildingName_;
    std::string                 hudMessage_;
    float                       hudAge_{0.0f};
    CollisionMask               terrainMask_;
    bool                        inventoryOpen_{false};
    bool                        menuOpen_{false};
    int                         menuCursor_{0};
    bool                        helpOpen_{false};
    bool                        reducedMotion_{false};
    AppAction                   pendingAppAction_{AppAction::None};
};

} // namespace nccu

#endif // WORLD_H_
