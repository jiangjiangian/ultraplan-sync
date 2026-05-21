#include "entities/QuestFlagPickup.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include "gfx/Color.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
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
                                 int completionKarma)
    : Item(position,
           nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f},
           "QuestItem"),
      flagName_(std::move(flagName)),
      message_(std::move(message)),
      completionFlags_(std::move(completionFlags)),
      completionKarma_(completionKarma) {}

void QuestFlagPickup::OnPickup(Player* player) {
    if (!player) return;
    player->SetFlag(flagName_);
    isActive_ = false;
    EventBus::Instance().Publish(
        Event{ EventType::ShowMessage, message_ });

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
