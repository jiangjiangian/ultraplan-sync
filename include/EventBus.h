#pragma once
#include "raylib.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

enum class EventType {
    RenderRequested,
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
};

struct Event {
    EventType type;
    Vector2 position;
    Color color;
    std::string text;
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
