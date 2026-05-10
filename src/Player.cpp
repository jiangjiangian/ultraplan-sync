#include "Player.h"
#include "raylib.h"

Player::Player(Vector2 position)
    // 180 px/sec ≈ original 3 px/frame at 60 FPS, but frame-rate independent now
    : Character(position, {position.x, position.y, 24.0f, 24.0f}, 180.0f),
      rainMeter_(0.0f), karma_(50), hasUmbrella_(false) {}

void Player::Update(float deltaTime) {
    HandleInput(deltaTime);
}

void Player::Draw() const {
    DrawRectangleRec(hitBox_, BLUE);
}

void Player::Interact(Player* /*initiator*/) {
    // Player does not respond to other Players in MVP
}

void Player::HandleInput(float deltaTime) {
    Vector2 dir{0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) dir.y += 1.0f;
    if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) dir.x += 1.0f;
    Move(dir, deltaTime);
}

void Player::decreaseKarma(int amount) {
    karma_ -= amount;
}

void Player::resetRainMeter() {
    rainMeter_ = 0.0f;
}
