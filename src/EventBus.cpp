#include "EventBus.h"

EventBus& EventBus::Instance() {
    static EventBus instance;
    return instance;
}

void EventBus::Subscribe(EventType type, Handler handler) {
    handlers_[type].push_back(std::move(handler));
}

void EventBus::Publish(const Event& event) const {
    auto it = handlers_.find(event.type);
    if (it == handlers_.end()) return;
    // Snapshot the handler list before dispatch: a handler may call
    // Subscribe / Clear during the call, which would otherwise invalidate
    // this iterator. Synchronous bus + recursive publish is undefined
    // behavior on the live vector.
    const std::vector<Handler> snapshot = it->second;
    for (const auto& h : snapshot) h(event);
}

void EventBus::Clear() {
    handlers_.clear();
}
