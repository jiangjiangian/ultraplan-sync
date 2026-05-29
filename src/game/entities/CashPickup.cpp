#include "game/entities/CashPickup.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/entities/Player.h"
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

#include <string>

void CashPickup::Render(nccu::engine::render::IRenderer& renderer) const {
    // 可見的地面標記，使散落的金錢在探索時可被發現——隱形的拾取物無從尋獲。先前以填滿
    // 碰撞盒的綠色方塊呈現，在試玩時被當成不明雜物；改用小型置中金幣（金色＝金錢是普世
    // 認知），並與黃色任務標記區隔。僅用 DrawRect——Item 不得呼叫 DrawText／DrawTexture
    // （架構規則），不含 raylib。
    constexpr float kCoin = 10.0f;                 // 小於 16px 的碰撞盒
    const float inset = (hitBox_.width - kCoin) * 0.5f;
    renderer.DrawRect(
        nccu::engine::math::Rect{hitBox_.x + inset, hitBox_.y + inset, kCoin, kCoin},
        nccu::engine::math::Colors::Gold);
}

CashPickup::CashPickup(nccu::engine::math::Vec2 position, int value)
    // 直接基底為 WithRoles<CashPickup, Item>，其 `using Base::Base` 繼承 Item 的建構子，
    // 故此 3 參數形式仍可解析。
    : WithRoles(position,
           // 錨定於世界座標的 16×16 碰撞盒——與 ConsumableItem 同尺寸，故走同一道碰撞掃描拾取。
           nccu::engine::math::Rect{position.x, position.y, 16.0f, 16.0f},
           "Cash"),
      value_(value) {}

void CashPickup::OnPickup(Player* player) {
    // 空 player 視為無動作而非崩潰——地圖管理器偶爾會在 Update 迴圈之外清掃拾取物，寧可
    // 什麼都不做，也不要解參考無效指標。
    if (!player) return;

    player->AddMoney(value_);
    isActive_ = false;

    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, std::string("撿到 ") + std::to_string(value_) + " 元" });
}
