#include "NPC.h"
#include "EventBus.h"
#include "gfx/Renderer.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"

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
    using nccu::gfx::Rect;
    using nccu::gfx::Renderer;
    if (!sprite_ || !sprite_->IsValid()) {
        Renderer{}.Rect(hitBox_, nccu::gfx::Colors::Green);
        return;
    }
    // Pipoya cell + facing: stationary NPCs always show col 1 (idle), row 0
    // (facing the camera). Bottom-centre the 32x32 sprite on the 24x24 hit
    // box so feet sit at the hitbox base — same convention as Player.
    constexpr int kCell = 32;
    constexpr int kIdleCol = 1;
    constexpr int kDownRow = 0;
    const Rect src{
        static_cast<float>(kIdleCol * kCell),
        static_cast<float>(kDownRow * kCell),
        static_cast<float>(kCell),
        static_cast<float>(kCell)};
    const Rect dest{
        hitBox_.x + (hitBox_.width  - kCell) * 0.5f,
        hitBox_.y +  hitBox_.height - kCell,
        static_cast<float>(kCell),
        static_cast<float>(kCell)};
    Renderer{}.TextureRect(*sprite_, src, dest);
}

void NPC::LoadSprite(const std::string& path) {
    sprite_ = nccu::gfx::Texture::Load(path);
}

void NPC::Interact(Player* /*initiator*/) {
    if (dialogLines_.empty()) return;
    // Snapshot the current line text and advance the index BEFORE
    // dispatching the event. EventBus::Publish is synchronous and
    // supports recursive dispatch (see test_eventbus reentrancy case),
    // so any subscriber that calls back into Interact() must observe
    // the already-advanced state rather than a stale index.
    const std::string line = dialogLines_[currentLineIndex_];
    currentLineIndex_ = (currentLineIndex_ + 1) % dialogLines_.size();
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        line
    });
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
