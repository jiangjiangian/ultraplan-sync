#ifndef EVENT_BUS_H_
#define EVENT_BUS_H_
#include "engine/events/HudSlot.h"
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

enum class EventType {
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
    EnteredBuilding,
    // Emitted whenever the player gains a non-umbrella inventory item —
    // currently published by Vendor::TryBuy on a successful purchase. The
    // event payload's `text` field carries the item id (e.g. "HotPack").
    PickupAcquired,
};

// Cycle 9.G — `slot` routes a ShowMessage to one of two independent HUD
// channels so a chapter-clear / ending toast can coexist with a regular
// pickup / karma / arrival-hint toast on the same frame (was: single
// slot; later publish clobbered the earlier — cycle9f §B). Default
// HudSlot::Bottom keeps every existing publisher's behaviour byte-
// identical; only the three ChapterToast / EndingGate sites opt in to
// HudSlot::Top. Ignored for non-ShowMessage event types.
struct Event {
    EventType        type;
    std::string      text;
    nccu::HudSlot    slot = nccu::HudSlot::Bottom;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    static EventBus& Instance();

    // RAII unsubscribe token (H1). Returned by ScopedSubscribe(); its
    // destructor removes EXACTLY the one handler it owns from the bus, so
    // a subscription's lifetime is tied to a scope/owner and a handler
    // can never outlive the state it captured (the B1/B2 footgun). The
    // token is movable (ownership transfers; no double-unsubscribe) and
    // non-copyable. A default-constructed / moved-from token owns nothing
    // and its destruction is a no-op. Removal is by stable id, NOT by
    // iterator/pointer — Publish snapshots the handler list before
    // dispatch, so unsubscribing (even from inside a handler) only
    // affects subsequent Publish calls, preserving the deferred-dispatch
    // / re-entrancy contract.
    class Subscription {
    public:
        Subscription() noexcept = default;
        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;
        Subscription(Subscription&& other) noexcept;
        Subscription& operator=(Subscription&& other) noexcept;
        ~Subscription();

        // True iff this token currently owns a live subscription.
        bool Active() const noexcept { return bus_ != nullptr; }

        // Remove the owned handler now (idempotent); destruction afterward
        // is a no-op. Safe to call from within a handler.
        void Reset() noexcept;

    private:
        friend class EventBus;
        Subscription(EventBus* bus, std::uint64_t id) noexcept
            : bus_(bus), id_(id) {}

        EventBus*     bus_ = nullptr;
        std::uint64_t id_  = 0;
    };

    // Fluent — chain subscriptions:
    //   EventBus::Instance()
    //     .Subscribe(EventType::ShowMessage, ...)
    //     .Subscribe(EventType::EnteredBuilding, ...);
    // Backward-compatible: this handler lives until the next Clear().
    EventBus& Subscribe(EventType type, Handler handler);

    // RAII variant of Subscribe (H1). Identical delivery semantics, but
    // returns a scoped token; let it die (scope exit / owner dtor) and
    // the handler is unsubscribed automatically — no manual Clear()
    // bookkeeping, no dangling-capture use-after-free. Existing
    // Subscribe()/Clear() keep working unchanged; this is additive.
    [[nodiscard]] Subscription ScopedSubscribe(EventType type,
                                               Handler   handler);

    void      Publish(const Event& event) const;
    EventBus& Clear();

private:
    EventBus() = default;

    // Remove the single id-keyed handler. No-op if already gone (Clear()
    // ran, or it was unsubscribed). Safe to call mid-dispatch: Publish
    // works off a snapshot taken before this can run.
    void Unsubscribe(std::uint64_t id) noexcept;

    struct Slot {
        std::uint64_t id;
        Handler       handler;
    };
    std::unordered_map<EventType, std::vector<Slot>> handlers_;
    std::uint64_t                                    nextId_ = 1;
    // Reader-writer mutex guarding the handler list ONLY — not handler
    // bodies. Subscribe/ScopedSubscribe/Clear/Unsubscribe take
    // unique_lock; Publish takes shared_lock to copy the snapshot then
    // drops it before dispatch. Handler bodies still race if Publish is
    // called from multiple threads — DO NOT publish off the main thread:
    // subscriber bodies may touch the GL context via the View, which is
    // single-threaded.
    mutable std::shared_mutex mutex_;
};

#endif // EVENT_BUS_H_
