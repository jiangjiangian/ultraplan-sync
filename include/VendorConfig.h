#ifndef VENDOR_CONFIG_H_
#define VENDOR_CONFIG_H_
#include <string>
#include <vector>

// Plain data describing a single sellable line item attached to a Vendor NPC.
// Kept deliberately tiny so the content layer (main.cpp, future SCRIPT loader)
// can build vendor stock tables without depending on game-runtime headers.
struct VendorItem {
    std::string itemId;   // e.g. "HotPack", "TrueUmbrella"
    int         price;    // money cost; matched against Player::DeductMoney
};

// Aggregate config for one market stall. The Vendor class consumes this in
// its constructor to derive both the display name and the initial dialog
// lines, so authoring a new stall is one POD-literal away.
struct VendorConfig {
    std::string             name;      // attached NPC display name
    std::string             greeting;  // shown on first Interact
    std::vector<VendorItem> stock;     // items offered (price-ordered by author)
};

#endif // VENDOR_CONFIG_H_
