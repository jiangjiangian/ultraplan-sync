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
    // Reader-writer mutex guarding the handler list ONLY — not handler
    // bodies. Subscribe/Clear take unique_lock; Publish takes shared_lock
    // to copy the snapshot then drops it before dispatch. Handler bodies
    // still race if Publish is called from multiple threads — DO NOT
    // publish off the main thread. raylib's GL context is single-threaded
    // too, so RenderRequested handlers MUST run on the main thread.
    mutable std::shared_mutex mutex_;
};

#endif // EVENT_BUS_H_
