#include "Vendor.h"
#include "Player.h"

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

bool Vendor::TryBuy(Player* /*player*/, std::size_t /*stockIndex*/) {
    // Implemented in a later commit alongside the EventBus extension.
    return false;
}
