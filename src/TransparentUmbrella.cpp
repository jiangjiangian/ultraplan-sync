#include "TransparentUmbrella.h"
#include "EventBus.h"

void TransparentUmbrella::Draw() const {
    // UI/Data decoupling: emit a render request, do not call raylib here
    EventBus::Instance().Publish(Event{
        EventType::RenderRequested,
        position_,
        umbrellaTint_,
        itemName_
    });
}
