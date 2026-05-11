#pragma once
#include "Buildings.h"
#include "gfx/Vec2.h"

namespace nccu {

// Single-edge transition detector: the FIRST frame the player walks
// into a building's trigger rect, publishes EventType::EnteredBuilding
// on EventBus with the building's name. Subsequent frames inside the
// same building are silent. Walking into empty space clears Current()
// without firing an event.
class BuildingTracker {
public:
    const buildings::Building* Update(gfx::Vec2 playerCenter);

    const buildings::Building* Current() const noexcept { return current_; }

private:
    const buildings::Building* current_{nullptr};
};

} // namespace nccu
