#pragma once
#include "NPC.h"
#include "VendorConfig.h"
#include "gfx/Vec2.h"
#include <cstddef>

// Vendor: an NPC that doubles as a shop counter. The base NPC class still
// drives Interact() cycling so visitors browse via dialog; the dedicated
// TryBuy() entry point is invoked by the UI / shop screen with an explicit
// stock index after the player picks a line.
class Vendor : public NPC {
public:
    Vendor(nccu::gfx::Vec2 position, VendorConfig config);

    // Attempts to sell stock_[stockIndex] to player.
    //   - out-of-range stockIndex  -> false, no events
    //   - DeductMoney fails (poor) -> false, ShowMessage "你錢不夠"
    //   - success                  -> true, ShowMessage + PickupAcquired
    bool TryBuy(class Player* player, std::size_t stockIndex);

    const VendorConfig& Config() const { return config_; }

private:
    // Builds the NPC dialog lines from greeting + each stock entry. Called
    // once from the constructor; if stock ever mutates we'd re-run this and
    // pump the new lines through SetDialogLines.
    static std::vector<std::string> BuildDialogLines(const VendorConfig& config);

    VendorConfig config_;
};
