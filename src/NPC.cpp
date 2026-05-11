#include "NPC.h"
#include "EventBus.h"
#include "gfx/Renderer.h"
#include "gfx/Color.h"

NPC::NPC(nccu::gfx::Vec2 position,
         std::vector<std::string> dialogLines,
         bool isQuestGiver)
    : Character(position,
                nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f},
                0.0f /* npcs do not move on their own in this phase */),
      dialogLines_(std::move(dialogLines)),
      currentLineIndex_(0),
      isQuestGiver_(isQuestGiver) {}

void NPC::Update(float /*deltaTime*/) {
    // NPCs are stationary in this phase.
}

void NPC::Draw() const {
    nccu::gfx::Renderer{}.Rect(hitBox_, nccu::gfx::Colors::Green);
}

void NPC::Interact(Player* /*initiator*/) {
    if (dialogLines_.empty()) return;
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        CurrentLineText()
    });
    currentLineIndex_ = (currentLineIndex_ + 1) % dialogLines_.size();
}

const std::string& NPC::CurrentLineText() const {
    static const std::string empty;
    if (dialogLines_.empty()) return empty;
    return dialogLines_[currentLineIndex_];
}

void NPC::SetDialogLines(std::vector<std::string> lines) {
    dialogLines_ = std::move(lines);
    currentLineIndex_ = 0;
}
