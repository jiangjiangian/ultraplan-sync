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
    for (const auto& h : it->second) h(event);
}

void EventBus::Clear() {
    handlers_.clear();
}
