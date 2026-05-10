#include "Player.h"
#include "gfx/Renderer.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Color.h"

Player::Player(nccu::gfx::Vec2 position)
    // 180 px/sec ≈ original 3 px/frame at 60 FPS, frame-rate independent.
    : Character(position, nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f}, 180.0f),
      rainMeter_(0.0f), karma_(50), hasUmbrella_(false) {}

void Player::Update(float deltaTime) {
    HandleInput(deltaTime);
}

void Player::Draw() const {
    nccu::gfx::Renderer{}.Rect(hitBox_, nccu::gfx::Colors::Blue);
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

void Player::decreaseKarma(int amount) {
    karma_ -= amount;
}

void Player::resetRainMeter() {
    rainMeter_ = 0.0f;
}
