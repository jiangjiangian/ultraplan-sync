#include "entities/NPC.h"
#include "dialog/DialogSource.h"
#include "controller/EventBus.h"
#include "world/Physics.h"
#include "world/WorldConfig.h"
#include "gfx/IRenderer.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"
#include <algorithm>

NPC::NPC(nccu::gfx::Vec2 position,
         std::vector<std::string> dialogLines,
         bool isQuestGiver,
         std::string_view npcId)
    : Character(position,
                nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f},
                0.0f /* archetype NPCs are stationary; EnableWander() opts in */),
      dialogLines_(std::move(dialogLines)),
      currentLineIndex_(0),
      isQuestGiver_(isQuestGiver),
      npcId_(npcId),
      wander_(false),
      retargetTimer_(0.0f),
      wanderDir_{0.0f, 0.0f},
      rng_(0),
      wanderMask_(nullptr) {}

NPC& NPC::EnableWander(float speed, unsigned seed) noexcept {
    wander_        = true;
    speed_         = speed;                       // protected in Character
    rng_           = seed ? seed : 0x9E3779B9u;   // never seed an all-zero state
    retargetTimer_ = 0.0f;                        // pick a heading on frame 1
    return *this;
}

void NPC::Update(float deltaTime) {
    if (!wander_) return;  // archetype NPCs stand at their post

    retargetTimer_ -= deltaTime;
    if (retargetTimer_ <= 0.0f) {
        // xorshift32 — one of 8 compass headings, or idx 8 = pause.
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        static constexpr nccu::gfx::Vec2 kDirs[9] = {
            {0, -1}, {0, 1}, {-1, 0}, {1, 0},
            {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {0, 0}};
        wanderDir_ = kDirs[rng_ % 9u];
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        retargetTimer_ = 1.0f + static_cast<float>(rng_ % 2000u) / 1000.0f;
    }

    const nccu::gfx::Vec2 prev = position_;
    Move(wanderDir_, deltaTime);  // Character::Move integrates + syncs hitBox_

    const float kMaxXY = ::world::kSize - ::world::kPlayerWidth;
    nccu::gfx::Vec2 p = position_;
    p.x = std::clamp(p.x, 0.0f, kMaxXY);
    p.y = std::clamp(p.y, 0.0f, kMaxXY);
    SetPosition(p);

    if (wanderMask_) {
        static const std::vector<nccu::gfx::Rect> kNoDynamic;
        const nccu::gfx::Vec2 resolved = nccu::physics::ResolveMove(
            prev, position_,
            nccu::gfx::Vec2{::world::kPlayerWidth, ::world::kPlayerHeight},
            kNoDynamic, wanderMask_);
        if (resolved.x != position_.x || resolved.y != position_.y) {
            // Bumped a wall — turn away within a beat instead of grinding.
            retargetTimer_ = std::min(retargetTimer_, 0.2f);
        }
        SetPosition(resolved);
    }
}

void NPC::Render(nccu::gfx::IRenderer& renderer) const {
    using nccu::gfx::Rect;
    if (!sprite_ || !sprite_->IsValid()) {
        renderer.DrawRect(hitBox_, nccu::gfx::Colors::Green);
        return;
    }
    // Static full-image sprite (e.g. a 自動販賣機): draw the WHOLE texture
    // scaled to ~48 px tall, bottom-centred on the hitbox — machine art is
    // a single 45×94 image, not a Pipoya 32×32 walk sheet.
    if (staticSprite_) {
        const float tw = static_cast<float>(sprite_->Width());
        const float th = static_cast<float>(sprite_->Height());
        const float dh = 48.0f;
        const float dw = (th > 0.0f) ? tw * (dh / th) : 24.0f;
        const Rect dest{hitBox_.x + (hitBox_.width - dw) * 0.5f,
                        hitBox_.y + hitBox_.height - dh, dw, dh};
        renderer.DrawSprite(*sprite_, Rect{0.0f, 0.0f, tw, th}, dest);
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
    renderer.DrawSprite(*sprite_, src, dest);
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
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, line });
}

std::string_view NPC::NpcId() const noexcept { return npcId_; }

const std::string& NPC::CurrentLineText() const {
    static const std::string empty;
    if (dialogLines_.empty()) return empty;
    return dialogLines_[currentLineIndex_];
}

NPC& NPC::SetDialogLines(std::vector<std::string> lines) {
    dialogLines_ = std::move(lines);
    currentLineIndex_ = 0;
    return *this;
}

NPC& NPC::LoadDialog(std::string_view npcId, nccu::SemesterState state,
                     int subState) {
    for (const auto& e : nccu::dialog::Entries(npcId, state)) {
        if (e.subState == subState) {
            return SetDialogLines(e.lines);
        }
    }
    return SetDialogLines({});
}
