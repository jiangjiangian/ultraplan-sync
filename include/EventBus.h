#ifndef EVENT_BUS_H_
#define EVENT_BUS_H_
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include <functional>
#include <shared_mutex>
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

    // Fluent — chain subscriptions:
    //   EventBus::Instance()
    //     .Subscribe(EventType::ShowMessage, ...)
    //     .Subscribe(EventType::EnteredBuilding, ...);
    EventBus& Subscribe(EventType type, Handler handler);
    void      Publish(const Event& event) const;
    EventBus& Clear();

private:
    EventBus() = default;
    std::unordered_map<EventType, std::vector<Handler>> handlers_;
    // Reader-writer mutex: Publish() is a reader (concurrent dispatch
    // safe), Subscribe/Clear are writers (exclusive). Marked mutable so
    // the const Publish can acquire the shared lock.
    mutable std::shared_mutex mutex_;
};

#endif // EVENT_BUS_H_
