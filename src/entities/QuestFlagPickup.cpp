#include "entities/QuestFlagPickup.h"
#include "engine/events/EventBus.h"
#include "entities/Player.h"
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "gfx/UmbrellaGlyph.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
// A quest pickup's on-ground appearance is decided by WHAT it is, read
// from the flag it sets (data-driven — no per-instance hack). The Ch1 苦主
// transparent umbrella sets Flag_HasVictimUmbrella, so it must read as the
// BLUE 真傘 umbrella (not a yellow square / 紙張). Everything else this
// pickup models — the 申請書 (Flag_FoundForm) and the 散落筆記
// (Flag_FoundNote*) — is a sheet of paper, drawn WHITE (the owner: 紙張請用
// 白色). A flag substring keeps it robust to the kFlag* string constants.
bool IsUmbrellaFlag(std::string_view flag) {
    return flag.find("Umbrella") != std::string_view::npos;
}
}  // namespace

void QuestFlagPickup::Render(nccu::gfx::IRenderer& renderer) const {
    using nccu::gfx::Rect;
    namespace C = nccu::gfx::Colors;

    // Type-aware ground marker so the player reads what the item IS while
    // exploring (the reported bug: the Ch1 transparent umbrella showed as a
    // yellow square / looked like 紙張). Decided from the flag, kept
    // data-driven. Rect-only, no sprite/text — an Item must not call
    // DrawText/DrawTexture (architecture rule); no direct raylib.
    if (IsUmbrellaFlag(flagName_)) {
        // The 苦主's transparent umbrella — drawn with the SAME shared glyph
        // the in-world umbrellas and the ending card use, in 真傘 blue, so
        // it is unmistakably "the umbrella you came to find".
        nccu::gfx::DrawUmbrellaGlyph(renderer, nccu::gfx::UmbrellaLook::TrueBlue,
                                     hitBox_);
        return;
    }

    // A quest PAPER (申請書 / 筆記): a small WHITE sheet with a folded
    // corner + a couple of faint text rules, so it reads as a dropped page.
    const float x = hitBox_.x;
    const float y = hitBox_.y;
    const float w = hitBox_.width;
    const float h = hitBox_.height;
    auto rc = [&](float fx, float fy, float fw, float fh, nccu::gfx::Color col) {
        renderer.DrawRect(Rect{x + fx * w, y + fy * h, fw * w, fh * h}, col);
    };
    rc(0.18f, 0.10f, 0.64f, 0.80f, C::White);          // the sheet
    rc(0.60f, 0.10f, 0.22f, 0.22f, C::DarkGray);       // folded top-right corner
    rc(0.30f, 0.38f, 0.40f, 0.06f, C::DarkGray);       // text rule
    rc(0.30f, 0.56f, 0.40f, 0.06f, C::DarkGray);       // text rule
}

QuestFlagPickup::QuestFlagPickup(nccu::gfx::Vec2 position,
                                 std::string flagName,
                                 std::string message,
                                 std::vector<std::string> completionFlags,
                                 int completionKarma,
                                 std::vector<std::string> countMessages)
    // Direct base is WithRoles<QuestFlagPickup, Item>; its `using Base::Base`
    // inherits Item's ctor so this 3-arg form still resolves.
    : WithRoles(position,
           nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f},
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

    // Count-based message (this pickup's flag was just set above, so the
    // tally already includes it): how many of the completion set the
    // player now holds picks the line. 1st collected -> countMessages_[0],
    // etc. This is keyed on the COUNT, not on which note — so any pickup
    // order prints the right "1st / 2nd / last" sentence. Clamp to the
    // last entry defensively (never out-of-range even if the set grows).
    // Empty countMessages_ -> the single message_ (申請書 / non-set items).
    std::string toShow = message_;
    if (!countMessages_.empty() && !completionFlags_.empty()) {
        std::size_t held = 0;
        for (const auto& f : completionFlags_)
            if (player->HasFlag(f)) ++held;
        if (held == 0) held = 1;                       // defensive floor
        const std::size_t idx =
            held - 1 < countMessages_.size() ? held - 1
                                             : countMessages_.size() - 1;
        toShow = countMessages_[idx];
    }
    EventBus::Instance().Publish(
        Event{ EventType::ShowMessage, toShow });

    // Set-completion reward (S5c-2): grant the bonus iff every sibling
    // flag is now satisfied. flagName_ was just set, so the pickup that
    // closes the set sees `all` true; the earlier ones see a gap and
    // skip, and they have already deactivated — fires exactly once.
    if (completionKarma_ != 0 && !completionFlags_.empty()) {
        bool all = true;
        for (const auto& f : completionFlags_)
            if (!player->HasFlag(f)) { all = false; break; }
        if (all) player->AddKarma(completionKarma_);
    }
}
