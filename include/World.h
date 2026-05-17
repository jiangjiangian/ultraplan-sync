#ifndef WORLD_H_
#define WORLD_H_
#include "GameObject.h"
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
};

} // namespace nccu

#endif // WORLD_H_
