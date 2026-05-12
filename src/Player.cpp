#include "Player.h"
#include "EventBus.h"
#include "gfx/Renderer.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"

#include <cmath>

namespace {

// Pipoya 32x32 walk-strip cycle: idle column (1), left foot (0), idle (1),
// right foot (2). Stepping every kFrameDuration seconds while moving.
constexpr int kSpriteCell = 32;
constexpr int kWalkColumns[4] = {1, 0, 1, 2};
constexpr float kFrameDuration = 0.15f;

// Pipoya row order: 0=down, 1=left, 2=right, 3=up.
int RowForFacing(nccu::gfx::Vec2 facing) {
    const float ax = std::fabs(facing.x);
    const float ay = std::fabs(facing.y);
    if (ax > ay) return facing.x < 0.0f ? 1 : 2;
    return facing.y < 0.0f ? 3 : 0;
}

} // namespace

Player::Player(nccu::gfx::Vec2 position)
    // 180 px/sec ≈ original 3 px/frame at 60 FPS, frame-rate independent.
    : Character(position, nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f}, 180.0f),
      rainMeter_(0.0f), karma_(50), hasUmbrella_(false), money_(100) {}

void Player::LoadSprite(const std::string& path) {
    sprite_ = nccu::gfx::Texture::Load(path);
}

void Player::Update(float deltaTime) {
    HandleInput(deltaTime);

    // Animation: advance only while actually moving so idle pose holds the
    // middle column. Save lastFacing_ so we keep the correct row at rest.
    const bool moving = direction_.x != 0.0f || direction_.y != 0.0f;
    if (moving) {
        lastFacing_ = direction_;
        animTimer_ += deltaTime;
        if (animTimer_ >= kFrameDuration) {
            animTimer_ -= kFrameDuration;
            animStep_ = (animStep_ + 1) % 4;
        }
    } else {
        animTimer_ = 0.0f;
        animStep_ = 0;  // kWalkColumns[0] = 1 = idle column
    }
}

void Player::Draw() const {
    using nccu::gfx::Rect;
    using nccu::gfx::Renderer;
    if (!sprite_ || !sprite_->IsValid()) {
        Renderer{}.Rect(hitBox_, nccu::gfx::Colors::Blue);
        return;
    }
    const int col = kWalkColumns[animStep_];
    const int row = RowForFacing(lastFacing_);
    const Rect src{
        static_cast<float>(col * kSpriteCell),
        static_cast<float>(row * kSpriteCell),
        static_cast<float>(kSpriteCell),
        static_cast<float>(kSpriteCell)};
    // Bottom-centre the 32x32 sprite on the 24x24 hitbox so feet sit at the
    // hitbox base — matches the JRPG convention of collision-at-feet.
    const Rect dest{
        hitBox_.x + (hitBox_.width  - kSpriteCell) * 0.5f,
        hitBox_.y +  hitBox_.height - kSpriteCell,
        static_cast<float>(kSpriteCell),
        static_cast<float>(kSpriteCell)};
    Renderer{}.TextureRect(*sprite_, src, dest);
}

void Player::Interact(Player* /*initiator*/) {
    // Player does not respond to other Players in MVP
}

void Player::HandleInput(float deltaTime) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    nccu::gfx::Vec2 dir{0.0f, 0.0f};
    if (Input::IsDown(Key::W)) dir.y -= 1.0f;
    if (Input::IsDown(Key::S)) dir.y += 1.0f;
    if (Input::IsDown(Key::A)) dir.x -= 1.0f;
    if (Input::IsDown(Key::D)) dir.x += 1.0f;
    Move(dir, deltaTime);
}

void Player::AddKarma(int delta) {
    karma_ += delta;
    if (karma_ > 100) karma_ = 100;
    if (karma_ < -100) karma_ = -100;
}

void Player::decreaseKarma(int amount) {
    // TODO: migrate callers to AddKarma; kept as a thin wrapper for now.
    AddKarma(-amount);
}

void Player::resetRainMeter() {
    rainMeter_ = 0.0f;
}

bool Player::DeductMoney(int amount) {
    if (amount > money_) {
        return false;
    }
    money_ -= amount;
    return true;
}

void Player::ApplyRain(float dt) {
    if (hasUmbrella_) {
        return;  // umbrella nullifies exposure
    }
    rainMeter_ += 5.0f * dt;
    if (rainMeter_ > 100.0f) rainMeter_ = 100.0f;
    if (rainMeter_ < 0.0f)   rainMeter_ = 0.0f;
    if (rainMeter_ >= 100.0f) {
        RespawnAtGate();
    }
}

void Player::RespawnAtGate() {
    // 正門 gate spawn — half-day passes, no karma penalty per design doc.
    position_ = nccu::gfx::Vec2{400.0f, 1850.0f};
    hitBox_.x = position_.x;
    hitBox_.y = position_.y;
    resetRainMeter();
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::Black,
        "你淋成落湯雞了，被傳送回正門。半天就這樣過去了。"
    });
}
