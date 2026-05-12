#include "Vendor.h"
#include "EventBus.h"
#include "Player.h"
#include "gfx/Color.h"

#include <string>
#include <utility>
#include <vector>

namespace {

// Tiny helper so we don't reach for <sstream> just to glue together a price
// line. Format: "<itemId> - <price> 元"
std::string FormatStockLine(const VendorItem& item) {
    return item.itemId + " - " + std::to_string(item.price) + " 元";
}

} // namespace

std::vector<std::string> Vendor::BuildDialogLines(const VendorConfig& config) {
    // Greeting first, then one line per stock entry. We keep this trivial:
    // the goal is just to give NPC::Interact something to cycle through so
    // the player can preview prices before invoking TryBuy.
    std::vector<std::string> lines;
    lines.reserve(1 + config.stock.size());
    lines.push_back(config.greeting);
    for (const auto& item : config.stock) {
        lines.push_back(FormatStockLine(item));
    }
    return lines;
}

Vendor::Vendor(nccu::gfx::Vec2 position, VendorConfig config)
    : NPC(position, BuildDialogLines(config), /*isQuestGiver=*/false),
      config_(std::move(config)) {}

bool Vendor::TryBuy(Player* player, std::size_t stockIndex) {
    // Defensive bounds check first — a null player or an out-of-range index
    // is a programmer error from the caller (UI passing the wrong slot), so
    // we silently bail without emitting any event to avoid lying to the
    // user about a transaction that never started.
    if (!player) return false;
    if (stockIndex >= config_.stock.size()) return false;

    const VendorItem& item = config_.stock[stockIndex];

    // DeductMoney is the gatekeeper: it returns false on insufficient funds
    // and performs NO side effect, so the player's purse is safe here.
    if (!player->DeductMoney(item.price)) {
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            position_,
            nccu::gfx::Colors::Red,
            "你錢不夠"
        });
        return false;
    }

    // Success: announce the transaction (UI) and the item gain (inventory).
    // Two events because subscribers are different — one paints a toast,
    // the other appends to the inventory model.
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        std::string("買了 ") + item.itemId
    });
    EventBus::Instance().Publish(Event{
        EventType::PickupAcquired,
        position_,
        nccu::gfx::Colors::White,
        item.itemId
    });
    return true;
}
