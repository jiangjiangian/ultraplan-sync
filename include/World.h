#ifndef WORLD_H_
#define WORLD_H_
#include "GameObject.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "BuildingTracker.h"
#include "CollisionMask.h"
#include "DialogState.h"
#include <memory>
#include <string>
#include <vector>

namespace nccu {

// The game's data model — owns every GameObject, the semester FSM, the
// building tracker and the terrain collision mask. No rendering, no input:
// the View reads it const, the GameController mutates it. player_ is a
// non-owning cache of the front object, dropped by the end-of-frame
// sweep when the player is removed.
class World {
public:
    explicit World(const std::string& playerSpritePath);

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

private:
    ObjectList           objects_;
    Player*              player_{nullptr};
    SemesterStateMachine semester_;
    BuildingTracker      tracker_;
    DialogState          dialog_;
    std::string          currentBuildingName_;
    CollisionMask        terrainMask_;
};

} // namespace nccu

#endif // WORLD_H_
