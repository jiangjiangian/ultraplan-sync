#include "entities/Player.h"
#include "controller/EventBus.h"
#include "gfx/IRenderer.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

// Pipoya 32x32 walk-strip cycle: idle column (1), left foot (0), idle (1),
// right foot (2). Stepping every kFrameDuration seconds while moving.
constexpr int kSpriteCell = 32;
constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2};
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
    // Direct base is WithRoles<Player, Character>; its `using Base::Base`
    // inherits Character's ctor so this 3-arg form still resolves.
    : WithRoles(position, nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f}, 180.0f),
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

void Player::Render(nccu::gfx::IRenderer& renderer) const {
    using nccu::gfx::Rect;
    if (!sprite_ || !sprite_->IsValid()) {
        renderer.DrawRect(hitBox_, nccu::gfx::Colors::Blue);
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
    renderer.DrawSprite(*sprite_, src, dest, tint_);
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

Player& Player::AddKarma(int delta) {
    karma_ = std::clamp(karma_ + delta, -100, 100);
    // Cycle 9.B H5: publish KarmaChanged so the bus' karma-toast
    // subscriber can mirror a transient "業力 ±N" banner into the HUD.
    // Before this hook every karma mutation flowed through AddKarma but
    // never reached the EventBus, leaving EventType::KarmaChanged a
    // dead channel (1 publisher in CursedUmbrella, 0 subscribers). The
    // payload text is the signed integer literal exactly so the
    // subscriber can prefix it ("業力 +5"). A delta of 0 still
    // publishes (the clamp may have ignored it, but a caller of
    // AddKarma(0) is so rare it isn't worth special-casing — emitting
    // 0 is harmless: subscribers can filter, and "業力 +0" is a no-op
    // toast that fades the same as any other).
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%+d", delta);
    EventBus::Instance().Publish(
        Event{EventType::KarmaChanged, std::string{buf}});
    return *this;
}

Player& Player::decreaseKarma(int amount) {
    return AddKarma(-amount);
}

Player& Player::resetRainMeter() noexcept {
    rainMeter_ = 0.0f;
    return *this;
}

bool Player::DeductMoney(int amount) noexcept {
    if (amount > money_) {
        return false;
    }
    money_ -= amount;
    return true;
}

Player& Player::ApplyRain(float dt, bool lethal) {
    if (hasUmbrella_) {
        return *this;  // umbrella nullifies exposure
    }
    rainMeter_ = std::clamp(rainMeter_ + 5.0f * dt, 0.0f, 100.0f);
    if (lethal && rainMeter_ >= 100.0f) {
        RespawnAtGate();
    }
    return *this;
}

Player& Player::DrainRain(float dt) noexcept {
    rainMeter_ = std::clamp(rainMeter_ - 10.0f * dt, 0.0f, 100.0f);
    return *this;
}

Player& Player::DrainRainBy(float amount) noexcept {
    // G4: a FIXED rain reduction (not rate-based) for a consumable used
    // from the bag — distinct from DrainRain's -10 u/s sheltered recovery.
    // Clamped to [0,100]; never teleports. A negative `amount` would add
    // rain, which no caller wants, but the clamp keeps it bounded either
    // way. Used by ApplyConsumableEffect / the entity Consume bodies so a
    // 暖暖包 / 噴霧 / 飲料 / 小吃 buys back a chunk of the meter on use.
    rainMeter_ = std::clamp(rainMeter_ - amount, 0.0f, 100.0f);
    return *this;
}

Player& Player::ApplyRainSheltered(float dt, bool lethal) {
    // REQUIREMENT #5: an umbrella in heavy 山下 rain SLOWS the soak, it
    // does not stop it (chapter2.md: still accrues, reduced rate). 1.5
    // u/s ≈ 30 % of the exposed 5 u/s — felt over a chapter but, given
    // the spine's long in-building dialog stretches drain at -10 u/s, a
    // competent run never reaches 100 (the ending scripts stay winnable
    // & deterministic — regression-pinned). Deliberately does NOT honour
    // hasUmbrella_ (this IS the umbrella-but-still-exposed case); the
    // exposed-rate ApplyRain keeps its own umbrella no-op untouched so
    // every pinned ApplyRain/DrainRain unit contract is byte-unchanged.
    rainMeter_ = std::clamp(rainMeter_ + 1.5f * dt, 0.0f, 100.0f);
    if (lethal && rainMeter_ >= 100.0f) {
        RespawnAtGate();
    }
    return *this;
}

void Player::RespawnAtGate() {
    // 正門 gate spawn — half-day passes, no karma penalty per design doc.
    position_ = nccu::gfx::Vec2{500.0f, 1860.0f};
    hitBox_.x = position_.x;
    hitBox_.y = position_.y;
    resetRainMeter();
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你淋成落湯雞了，被傳送回正門。半天就這樣過去了。" });
}
