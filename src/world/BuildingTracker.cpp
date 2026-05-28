#include "world/BuildingTracker.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/math/Color.h"

#include <string>

namespace nccu {

const buildings::Building* BuildingTracker::Update(gfx::Vec2 playerCenter) {
    const buildings::Building* found =
        detail::NearestContaining(playerCenter, buildings::kAll);

    if (found != current_) {
        current_ = found;
        if (found) {
            nccu::events::Sink().Publish(Event{ EventType::EnteredBuilding, std::string{found->name} });
        }
    }
    return current_;
}

} // namespace nccu
