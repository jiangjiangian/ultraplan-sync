#include "entities/QuestFlagPickup.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include "gfx/Color.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

void QuestFlagPickup::Render(nccu::gfx::IRenderer& renderer) const {
    // Visible ground marker so the player can spot and collect the
    // quest item while exploring (Zelda-style). Same injected-renderer
    // placeholder convention as NPC::Render — no direct raylib, no
    // DrawText/DrawTexture from an Item.
    renderer.DrawRect(hitBox_, nccu::gfx::Colors::Yellow);
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
