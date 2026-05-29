#include "game/world/BuildingTracker.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/math/Color.h"

#include <string>

/**
 * @file BuildingTracker.cpp
 * @brief 建築進入追蹤實作：僅在所在建築改變的那一幀發布進入事件（單邊觸發）。
 */

namespace nccu {

const buildings::Building* BuildingTracker::Update(nccu::engine::math::Vec2 playerCenter) {
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
