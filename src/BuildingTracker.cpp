#include "BuildingTracker.h"
#include "EventBus.h"
#include "gfx/Color.h"

#include <limits>
#include <string>

namespace nccu {

const buildings::Building* BuildingTracker::Update(gfx::Vec2 playerCenter) {
    // Some trigger rects in Buildings.h overlap by construction
    // (e.g. 行政大樓/新聞館, 風雩樓/風雩走廊, 集英樓/果夫樓). Resolve
    // overlap zones deterministically: when more than one rect contains
    // playerCenter, pick the one whose CENTRE is nearest. First-match
    // scanning was order-dependent and could permanently shadow later
    // buildings.
    const buildings::Building* found = nullptr;
    float bestDistSq = std::numeric_limits<float>::max();

    for (const auto& b : buildings::kAll) {
        if (!b.triggerRect.Contains(playerCenter)) continue;
        const float cx = b.triggerRect.x + b.triggerRect.width  * 0.5f;
        const float cy = b.triggerRect.y + b.triggerRect.height * 0.5f;
        const float dx = cx - playerCenter.x;
        const float dy = cy - playerCenter.y;
        const float d2 = dx * dx + dy * dy;
        // On exact ties (player on the perpendicular bisector between two
        // centres) break lexicographically by name rather than array order,
        // so the result does not silently depend on Buildings.h ordering.
        if (d2 < bestDistSq ||
            (found != nullptr && d2 == bestDistSq && b.name < found->name)) {
            bestDistSq = d2;
            found = &b;
        }
    }

    if (found != current_) {
        current_ = found;
        if (found) {
            const auto& r = found->triggerRect;
            EventBus::Instance().Publish(Event{
                EventType::EnteredBuilding,
                gfx::Vec2{r.x + r.width * 0.5f, r.y + r.height * 0.5f},
                gfx::Colors::White,
                std::string{found->name}
            });
        }
    }
    return current_;
}

} // namespace nccu
