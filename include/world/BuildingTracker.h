#ifndef BUILDING_TRACKER_H_
#define BUILDING_TRACKER_H_
#include "world/Buildings.h"
#include "gfx/Vec2.h"
#include <limits>

namespace nccu {

namespace detail {
// Of every building in `range` whose trigger rect contains `p`, return the
// one whose rect centre is nearest to `p`. Some trigger rects in
// Buildings.h overlap by construction; resolving by nearest centre (and
// breaking an exact equidistant tie lexicographically by name) keeps the
// result deterministic instead of silently depending on container order.
// nullptr when no rect contains `p`. Templated on the range so the
// disambiguation is unit-testable against synthetic fixtures, decoupled
// from the live (Tiled-regenerated) campus layout.
template <typename Range>
const buildings::Building* NearestContaining(gfx::Vec2 p, const Range& range) {
    const buildings::Building* found = nullptr;
    float bestDistSq = std::numeric_limits<float>::max();
    for (const auto& b : range) {
        if (!b.triggerRect.Contains(p)) continue;
        const float cx = b.triggerRect.x + b.triggerRect.width  * 0.5f;
        const float cy = b.triggerRect.y + b.triggerRect.height * 0.5f;
        const float dx = cx - p.x;
        const float dy = cy - p.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < bestDistSq ||
            (found != nullptr && d2 == bestDistSq && b.name < found->name)) {
            bestDistSq = d2;
            found = &b;
        }
    }
    return found;
}
} // namespace detail

// Single-edge transition detector: the FIRST frame the player walks
// into a building's trigger rect, publishes EventType::EnteredBuilding
// on EventBus with the building's name. Subsequent frames inside the
// same building are silent. Walking into empty space clears Current()
// without firing an event.
class BuildingTracker {
public:
    const buildings::Building* Update(gfx::Vec2 playerCenter);

    [[nodiscard]] const buildings::Building* Current() const noexcept { return current_; }

private:
    const buildings::Building* current_{nullptr};
};

} // namespace nccu

#endif // BUILDING_TRACKER_H_
