#ifndef EVENT_WIRING_H_
#define EVENT_WIRING_H_
#include "EventBus.h"
#include "SemesterStateMachine.h"
#include "gfx/Renderer.h"
#include "gfx/Rect.h"
#include "gfx/Color.h"
#include <iostream>
#include <string>
#include <unordered_map>

namespace nccu {

// Wires the cout-only log subscribers (ShowMessage, UmbrellaClaimed).
// Pure I/O — owns no game state.
inline void WireLoggingSubscribers(EventBus& bus) {
    bus.Subscribe(EventType::ShowMessage,
            [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; })
       .Subscribe(EventType::UmbrellaClaimed,
            [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
}

// Wires the EnteredBuilding subscriber that drives chapter transitions.
// Captures references to caller-owned state — caller must outlive the
// subscription, OR call EventBus::Clear() before those refs go out of
// scope (main.cpp does the latter just before return 0).
inline void WireStateTransitionSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName,
    const std::unordered_map<std::string, SemesterState>&      enterTrigger)
{
    bus.Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName, &semester, &enterTrigger](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
            if (auto it = enterTrigger.find(e.text); it != enterTrigger.end()) {
                semester.Transition(it->second);
            }
        });
}

// Wires the RenderRequested subscriber that draws the 3-rect umbrella
// glyph for items emitting visual events. Calls raylib via Renderer —
// MUST run on the main thread (raylib's GL context is single-threaded).
inline void WireRenderSubscribers(EventBus& bus) {
    using nccu::gfx::Rect;
    using nccu::gfx::Renderer;
    namespace Colors = nccu::gfx::Colors;

    bus.Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            // 3-rect umbrella glyph inside the item's 20x20 footprint:
            // tapered canopy in the tint colour + dark handle. Lets the
            // four umbrella subclasses read at a glance via their
            // distinct umbrellaTint_.
            const float x = e.position.x;
            const float y = e.position.y;
            Renderer{}
                .Rect(Rect{x +  2.0f, y +  4.0f, 16.0f, 3.0f}, e.color)
                .Rect(Rect{x +  0.0f, y +  7.0f, 20.0f, 3.0f}, e.color)
                .Rect(Rect{x +  9.0f, y + 10.0f,  2.0f, 9.0f}, Colors::DarkGray);
        });
}

// Convenience aggregator — preserves the original single-call entry
// point so existing call sites in main.cpp do not need to change.
inline void WireDefaultSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName,
    const std::unordered_map<std::string, SemesterState>&      enterTrigger)
{
    WireLoggingSubscribers(bus);
    WireStateTransitionSubscribers(bus, semester, currentBuildingName, enterTrigger);
    WireRenderSubscribers(bus);
}

} // namespace nccu

#endif // EVENT_WIRING_H_
