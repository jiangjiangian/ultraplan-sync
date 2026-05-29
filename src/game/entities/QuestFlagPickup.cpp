#include "game/entities/QuestFlagPickup.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/entities/Player.h"
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "game/gfx/UmbrellaGlyph.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
// 任務拾取物的地面外觀依「它是什麼」決定，從它所設立的旗標讀出（資料驅動，不做逐實例
// 的特例）。Ch1 苦主的透明傘設立 Flag_HasVictimUmbrella，故須讀作藍色真傘（而非黃色方
// 塊／紙張）。此拾取物所建模的其餘對象——申請書（Flag_FoundForm）與散落筆記
// （Flag_FoundNote*）——皆為紙張，繪成白色。以旗標子字串比對，使其對 kFlag* 字串常數保持穩健。
bool IsUmbrellaFlag(std::string_view flag) {
    return flag.find("Umbrella") != std::string_view::npos;
}
}  // namespace

void QuestFlagPickup::Render(nccu::engine::render::IRenderer& renderer) const {
    using nccu::engine::math::Rect;
    namespace C = nccu::engine::math::Colors;

    // 依種類繪製地面標記，使玩家探索時能讀出道具「是什麼」（曾回報的缺陷：Ch1 透明傘顯示為
    // 黃色方塊／看起來像紙張）。由旗標決定、保持資料驅動。僅用矩形、不畫 sprite／文字——
    // Item 不得呼叫 DrawText／DrawTexture（架構規則），不直接使用 raylib。
    if (IsUmbrellaFlag(flagName_)) {
        // 苦主的透明傘——以地圖內雨傘與結局卡片共用的同一份字符繪製，採真傘藍色，使其
        // 毫不含糊就是「你前來尋找的那把傘」。
        nccu::game::gfx::DrawUmbrellaGlyph(renderer, nccu::game::gfx::UmbrellaLook::TrueBlue,
                                     hitBox_);
        return;
    }

    // 任務紙張（申請書／筆記）：一張小巧的白色紙頁，帶折角與幾道淡淡的文字線條，讀來像掉落的紙頁。
    const float x = hitBox_.x;
    const float y = hitBox_.y;
    const float w = hitBox_.width;
    const float h = hitBox_.height;
    auto rc = [&](float fx, float fy, float fw, float fh, nccu::engine::math::Color col) {
        renderer.DrawRect(Rect{x + fx * w, y + fy * h, fw * w, fh * h}, col);
    };
    rc(0.18f, 0.10f, 0.64f, 0.80f, C::White);          // 紙頁本體
    rc(0.60f, 0.10f, 0.22f, 0.22f, C::DarkGray);       // 右上折角
    rc(0.30f, 0.38f, 0.40f, 0.06f, C::DarkGray);       // 文字線條
    rc(0.30f, 0.56f, 0.40f, 0.06f, C::DarkGray);       // 文字線條
}

QuestFlagPickup::QuestFlagPickup(nccu::engine::math::Vec2 position,
                                 std::string flagName,
                                 std::string message,
                                 std::vector<std::string> completionFlags,
                                 int completionKarma,
                                 std::vector<std::string> countMessages)
    // 直接基底為 WithRoles<QuestFlagPickup, Item>，其 `using Base::Base` 繼承 Item 的
    // 建構子，故此 3 參數形式仍可解析。
    : WithRoles(position,
           nccu::engine::math::Rect{position.x, position.y, 16.0f, 16.0f},
           "QuestItem"),
      flagName_(std::move(flagName)),
      message_(std::move(message)),
      completionFlags_(std::move(completionFlags)),
      completionKarma_(completionKarma),
      countMessages_(std::move(countMessages)) {}

void QuestFlagPickup::OnPickup(Player* player) {
    if (!player) return;
    player->SetFlag(flagName_);
    isActive_ = false;

    // 依數量挑選訊息（此拾取物的旗標已於上方設立，故計數已含它）：由玩家此刻持有完成集中
    // 的「幾個」決定該行文字。第 1 個撿到 -> countMessages_[0]，依此類推。這以「數量」為鍵
    // 而非以哪一張筆記，故任意拾取順序都能印出正確的「第一／第二／最後一張」句子。防禦性地
    // 夾制到最後一筆（即使集合擴增也絕不越界）。countMessages_ 為空時改用單一 message_
    // （申請書／非整組道具）。
    std::string toShow = message_;
    if (!countMessages_.empty() && !completionFlags_.empty()) {
        std::size_t held = 0;
        for (const auto& f : completionFlags_)
            if (player->HasFlag(f)) ++held;
        if (held == 0) held = 1;                       // 防禦性下限
        const std::size_t idx =
            held - 1 < countMessages_.size() ? held - 1
                                             : countMessages_.size() - 1;
        toShow = countMessages_[idx];
    }
    nccu::events::Sink().Publish(
        Event{ EventType::ShowMessage, toShow });

    // 整組完成獎勵：唯有每個姊妹旗標皆已滿足時才發放加成。flagName_ 已於上方設立，故收尾
    // 那次拾取會看到 all 為 true；較早者尚有缺口而略過，且早已失效——故恰好觸發一次。
    if (completionKarma_ != 0 && !completionFlags_.empty()) {
        bool all = true;
        for (const auto& f : completionFlags_)
            if (!player->HasFlag(f)) { all = false; break; }
        if (all) player->AddKarma(completionKarma_);
    }
}
