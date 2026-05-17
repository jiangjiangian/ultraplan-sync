#include "Vendor.h"
#include "EventBus.h"
#include "Player.h"
#include "VendorMessages.h"
#include "gfx/Color.h"

#include <string>
#include <utility>
#include <vector>

namespace {

// Tiny helper so we don't reach for <sstream> just to glue together a price
// line. Format: "<itemId> - <price> 元"
std::string FormatStockLine(const VendorItem& item) {
    namespace msg = nccu::vendor::msg;
    return item.itemId + std::string(msg::kStockLineSep)
         + std::to_string(item.price) + std::string(msg::kStockLineUnit);
}

} // namespace

std::vector<std::string> Vendor::BuildDialogLines(const VendorConfig& config) {
    // Greeting first, then one line per stock entry. We keep this trivial:
    // the goal is just to give NPC::Interact something to cycle through so
    // the player can preview prices before invoking TryBuy.
    std::vector<std::string> lines;
    // Parsed stalls carry a multi-line greeting (greetingLines); a
    // hand-written 3-field literal carries the single `greeting`. Prefer
    // the multi-line form when present, else fall back to the single
    // line (this keeps the pinned test_vendor 2-line expectation: that
    // config has no greetingLines, so it is greeting + 1 stock entry).
    if (!config.greetingLines.empty()) {
        lines.reserve(config.greetingLines.size() + config.stock.size());
        for (const auto& g : config.greetingLines) lines.push_back(g);
    } else {
        lines.reserve(1 + config.stock.size());
        lines.push_back(config.greeting);
    }
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

    VendorItem& item = config_.stock[stockIndex];

    // Sold out (stockLeft hit 0; -1 = unlimited). Checked before the
    // charge so the purse is never touched for an item we can't deliver.
    if (item.stockLeft == 0) {
        EventBus::Instance().Publish(Event{ EventType::ShowMessage, std::string(nccu::vendor::msg::kSoldOut) });
        return false;
    }

    // DeductMoney is the gatekeeper: it returns false on insufficient funds
    // and performs NO side effect, so the player's purse is safe here.
    if (!player->DeductMoney(item.price)) {
        EventBus::Instance().Publish(Event{ EventType::ShowMessage, std::string(nccu::vendor::msg::kInsufficientFunds) });
        return false;
    }

    // Success: announce the transaction (UI) and the item gain (inventory).
    // Two events because subscribers are different — one paints a toast,
    // the other appends to the inventory model.
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, std::string(nccu::vendor::msg::kPurchasedPrefix) + item.itemId });
    EventBus::Instance().Publish(Event{ EventType::PickupAcquired, item.itemId });

    // S5b-3: the buy actually lands in the count inventory, the stall's
    // karma hook fires (e.g. 募款箱 +1; default 0 = no-op, so the pinned
    // test_vendor stall is unaffected), and a finite stock ticks down
    // (-1 = unlimited, so the pinned stall never decrements).
    player->AddConsumable(item.itemId);
    if (config_.karmaOnInteract != 0)
        player->AddKarma(config_.karmaOnInteract);
    if (item.stockLeft > 0) --item.stockLeft;
    return true;
}
