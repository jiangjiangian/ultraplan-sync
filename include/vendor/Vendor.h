#ifndef VENDOR_H_
#define VENDOR_H_
#include "entities/NPC.h"
#include "vendor/VendorConfig.h"
#include "gfx/Vec2.h"
#include <cstddef>

// Vendor: an NPC that doubles as a shop counter. The base NPC class still
// drives Interact() cycling so visitors browse via dialog; the dedicated
// TryBuy() entry point is invoked by the UI / shop screen with an explicit
// stock index after the player picks a line.
class Vendor final : public NPC {
public:
    Vendor(nccu::gfx::Vec2 position, VendorConfig config);

    // Attempts to sell stock_[stockIndex] to player.
    //   - out-of-range stockIndex  -> false, no events
    //   - DeductMoney fails (poor) -> false, ShowMessage "你錢不夠"
    //   - success                  -> true, ShowMessage + PickupAcquired
    [[nodiscard]] bool TryBuy(class Player* player, std::size_t stockIndex);

    // I5: a Vendor has an empty NpcId() (not a dialog-content NPC), so
    // GameController needs a virtual it can dispatch on to route a shop
    // interaction to the buy choice UI instead of NPC::Interact's line
    // cycling (which never reaches TryBuy). Virtual-not-dynamic_cast,
    // same rationale as NPC::BlocksMovement().
    [[nodiscard]] bool IsVendor() const noexcept override { return true; }

    [[nodiscard]] const VendorConfig& Config() const noexcept { return config_; }

private:
    // Builds the NPC dialog lines from greeting + each stock entry. Called
    // once from the constructor; if stock ever mutates we'd re-run this and
    // pump the new lines through SetDialogLines.
    static std::vector<std::string> BuildDialogLines(const VendorConfig& config);

    VendorConfig config_;
};

#endif // VENDOR_H_
