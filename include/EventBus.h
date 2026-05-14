#ifndef EVENT_BUS_H_
#define EVENT_BUS_H_
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

enum class EventType {
    RenderRequested,
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
    EnteredBuilding,
    // Emitted whenever the player gains a non-umbrella inventory item —
    // currently published by Vendor::TryBuy on a successful purchase. The
    // event payload's `text` field carries the item id (e.g. "HotPack").
    PickupAcquired,
};

struct Event {
    EventType            type;
    nccu::gfx::Vec2      position;
    nccu::gfx::Color     color;
    std::string          text;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    static EventBus& Instance();

    void Subscribe(EventType type, Handler handler);
    void Publish(const Event& event) const;
    void Clear();

private:
    EventBus() = default;
    std::unordered_map<EventType, std::vector<Handler>> handlers_;
};

#endif // EVENT_BUS_H_
