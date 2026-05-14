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

// Wires the default UI / log / state-transition subscribers onto the
// shared EventBus instance. Pulled out of main() so the game-loop file
// is not 50 lines of lambda noise. The Subscribe chain itself is fluent
// because each call returns EventBus&.
inline void WireDefaultSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName,
    const std::unordered_map<std::string, SemesterState>&      enterTrigger)
{
    using nccu::gfx::Rect;
    using nccu::gfx::Renderer;
    namespace Colors = nccu::gfx::Colors;

    bus.Subscribe(EventType::ShowMessage,
            [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; })
       .Subscribe(EventType::UmbrellaClaimed,
            [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; })
       .Subscribe(EventType::EnteredBuilding,
            [&currentBuildingName, &semester, &enterTrigger](const Event& e) {
                currentBuildingName = e.text;
                std::cout << "[Game] Entered: " << e.text << '\n';
                if (auto it = enterTrigger.find(e.text); it != enterTrigger.end()) {
                    semester.Transition(it->second);
                }
            })
       .Subscribe(EventType::RenderRequested,
            [](const Event& e) {
                // 3-rect umbrella glyph inside the item's 20x20 footprint:
                // tapered canopy in the tint colour + dark handle. Lets
                // the four umbrella subclasses read at a glance via
                // their distinct umbrellaTint_.
                const float x = e.position.x;
                const float y = e.position.y;
                Renderer{}
                    .Rect(Rect{x +  2.0f, y +  4.0f, 16.0f, 3.0f}, e.color)
                    .Rect(Rect{x +  0.0f, y +  7.0f, 20.0f, 3.0f}, e.color)
                    .Rect(Rect{x +  9.0f, y + 10.0f,  2.0f, 9.0f}, Colors::DarkGray);
            });
}

} // namespace nccu

#endif // EVENT_WIRING_H_
