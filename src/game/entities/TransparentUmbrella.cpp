#include "game/entities/TransparentUmbrella.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/entities/Player.h"
#include "game/quest/Flags.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"

#include <string>

namespace {
// 兩條拾取入口共用的任務閘。主線必須已啟動（玩家已接下苦主的請託——
// Flag_PromisedVictim）才能認領任何雨傘；否則應引導玩家去推進任務，而非什麼都不做
// （無聲略過會被誤認為壞掉的拾取）。
bool QuestGateOpen(Player* player) {
    if (!player) return true;                  // 保留先前的 null 路徑
    if (player->HasFlag(nccu::kFlagPromisedVictim)) return true;
    nccu::events::Sink().Publish(Event{
        EventType::ShowMessage,
        std::string("這把傘不是你的——先去找那位掉了傘的同學問問吧。")});
    return false;
}
}  // namespace

void TransparentUmbrella::Interact(Player* initiator) {
    if (QuestGateOpen(initiator)) BeClaimed(initiator);
}

void TransparentUmbrella::OnPickup(Player* player) {
    if (QuestGateOpen(player)) BeClaimed(player);
}

void TransparentUmbrella::Render(nccu::engine::render::IRenderer& renderer) const {
    // 每把傘的外觀（真傘＝藍／破傘＝剩手柄／詛咒傘＝暗紫／醜傘＝綠，ProfessorTrap 為
    // 危險紅陷阱）全部集中於 nccu::game::gfx::DrawUmbrellaGlyph 一處，地面拾取物與結局
    // 卡片也共用它——故同一把傘無論出現在何處都讀來一致。每個子類別的 UmbrellaStyle
    // 僅選擇外觀；顏色現由共用字符決定（umbrellaTint_ 仍保留在物件上供需要色調的呼叫者
    // 使用，但 Render 已不需要它）。僅用矩形、不畫 sprite／文字——MVC 乾淨（View 依物件
    // 自身資料繪製；不觸碰模擬狀態）。
    nccu::game::gfx::DrawUmbrellaGlyph(renderer, LookForStyle(style_),
                                 nccu::engine::math::Rect{position_.x, position_.y,
                                                 20.0f, 20.0f});
}
