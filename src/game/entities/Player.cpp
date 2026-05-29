#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/render/IRenderer.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

// Pipoya 32×32 行走條循環：待機欄（1）、左腳（0）、待機（1）、右腳（2）。移動時每隔
// kFrameDuration 秒步進一次。
constexpr int kSpriteCell = 32;
constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2};
constexpr float kFrameDuration = 0.15f;

// Pipoya 列序：0=下、1=左、2=右、3=上。
int RowForFacing(nccu::engine::math::Vec2 facing) {
    const float ax = std::fabs(facing.x);
    const float ay = std::fabs(facing.y);
    if (ax > ay) return facing.x < 0.0f ? 1 : 2;
    return facing.y < 0.0f ? 3 : 0;
}

} // namespace

Player::Player(nccu::engine::math::Vec2 position)
    // 180 px/秒 ≈ 60 FPS 下原本的每幀 3 px，且與幀率無關。直接基底為
    // WithRoles<Player, Character>，其 `using Base::Base` 繼承 Character 的建構子，故此 3
    // 參數形式仍可解析。
    : WithRoles(position, nccu::engine::math::Rect{position.x, position.y, 24.0f, 24.0f}, 180.0f),
      rainMeter_(0.0f), karma_(50), hasUmbrella_(false), money_(100) {}

void Player::LoadSprite(const std::string& path) {
    sprite_ = nccu::engine::render::Texture::Load(path);
}

void Player::Update(float deltaTime) {
    HandleInput(deltaTime);

    // 動畫：僅在實際移動時推進，使待機姿勢停在中間欄。保存 lastFacing_，以在靜止時維持正確的列。
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
        animStep_ = 0;  // kWalkColumns[0] = 1 = 待機欄
    }
}

void Player::Render(nccu::engine::render::IRenderer& renderer) const {
    using nccu::engine::math::Rect;
    if (!sprite_ || !sprite_->IsValid()) {
        renderer.DrawRect(hitBox_, nccu::engine::math::Colors::Blue);
        return;
    }
    const int col = kWalkColumns[animStep_];
    const int row = RowForFacing(lastFacing_);
    const Rect src{
        static_cast<float>(col * kSpriteCell),
        static_cast<float>(row * kSpriteCell),
        static_cast<float>(kSpriteCell),
        static_cast<float>(kSpriteCell)};
    // 將 32×32 sprite 底部置中於 24×24 碰撞盒，使腳部落在碰撞盒底邊——契合 JRPG「碰撞在腳下」的慣例。
    const Rect dest{
        hitBox_.x + (hitBox_.width  - kSpriteCell) * 0.5f,
        hitBox_.y +  hitBox_.height - kSpriteCell,
        static_cast<float>(kSpriteCell),
        static_cast<float>(kSpriteCell)};
    renderer.DrawSprite(*sprite_, src, dest, tint_);
}

void Player::HandleInput(float deltaTime) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    nccu::engine::math::Vec2 dir{0.0f, 0.0f};
    if (Input::IsDown(Key::W)) dir.y -= 1.0f;
    if (Input::IsDown(Key::S)) dir.y += 1.0f;
    if (Input::IsDown(Key::A)) dir.x -= 1.0f;
    if (Input::IsDown(Key::D)) dir.x += 1.0f;
    Move(dir, deltaTime);
}

Player& Player::AddKarma(int delta) {
    karma_ = std::clamp(karma_ + delta, -100, 100);
    // 發布 KarmaChanged，使事件匯流排的業力提示訂閱者能在 HUD 鏡像一則短暫的「業力 ±N」橫幅。
    // 訊息內容正是帶正負號的整數字面，使訂閱者能加上前綴（「業力 +5」）。delta 為 0 時仍會發布
    // （夾制可能忽略了它，但呼叫 AddKarma(0) 極罕見而不值得特例——發出 0 無害：訂閱者可自行過濾，
    // 「業力 +0」是與其他無異會淡出的無動作提示）。
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%+d", delta);
    nccu::events::Sink().Publish(
        Event{EventType::KarmaChanged, std::string{buf}});
    return *this;
}

Player& Player::decreaseKarma(int amount) {
    return AddKarma(-amount);
}

// 與詛咒拾取數成正比的每章業力衰減。透過 AddKarma 走，使 [-100,100] 夾制與 KarmaChanged 提示
// （帶正負號 delta）持續觸發——可見的「業力 -5」橫幅是刻意的：玩家「應」每章感受到累積污點的咬噬。
// 0 污點的流程會完全略過 AddKarma 呼叫，故 KarmaChanged 事件不被發布，非詛咒試玩與基準維持一致。
Player& Player::ApplyCursedTaintDecay() {
    if (cursedTaint_ > 0) AddKarma(-5 * cursedTaint_);
    return *this;
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
        return *this;  // 持傘抵消曝露
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
    // 為背包使用的消耗品而設的「固定量」雨量減免（非速率）——有別於 DrainRain 的每秒 -10 遮蔽
    // 回復。夾制於 [0,100]，從不傳送。負的 amount 會增加雨量（無呼叫者想要），但夾制無論如何皆
    // 使其有界。由 ApplyConsumableEffect 與各實體 Consume 本體呼叫，使暖暖包／噴霧／飲料／小吃
    // 在使用時買回一塊雨量。
    rainMeter_ = std::clamp(rainMeter_ - amount, 0.0f, 100.0f);
    return *this;
}

Player& Player::ApplyRainSheltered(float dt, bool lethal) {
    // 山下大雨中的傘「減緩」浸濕而非阻止（chapter2.md：仍以較低速率累積）。每秒 1.5 點 ≈ 曝露
    // 5 點的 30%——跨越一章可感受到；但因主線冗長的建築內對話會以每秒 -10 拉長消減，稱職的流程
    // 永不抵達 100（結局腳本維持可通關且具決定性，由回歸測試固定）。刻意「不」理會 hasUmbrella_
    // （此正是持傘卻仍曝露的情形）；曝露速率的 ApplyRain 維持其自身的持傘無動作不變，故每一條
    // ApplyRain/DrainRain 的單元契約皆不改變。
    rainMeter_ = std::clamp(rainMeter_ + 1.5f * dt, 0.0f, 100.0f);
    if (lethal && rainMeter_ >= 100.0f) {
        RespawnAtGate();
    }
    return *this;
}

void Player::RespawnAtGate() {
    // 在正門重生——依設計文件，半天流逝，且無業力懲罰。
    position_ = nccu::engine::math::Vec2{500.0f, 1860.0f};
    hitBox_.x = position_.x;
    hitBox_.y = position_.y;
    resetRainMeter();
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你淋成落湯雞了，被傳送回正門。半天就這樣過去了。" });
}
