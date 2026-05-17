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
    int         stockLeft = -1;  // -1 = unlimited; >0 decremented per buy;
                                 // 0 = sold out (TryBuy fails before charge)
};

// Aggregate config for one market stall. The Vendor class consumes this in
// its constructor to derive both the display name and the initial dialog
// lines, so authoring a new stall is one POD-literal away.
//
// The first three members are the original shape and MUST stay in this
// order/type: VendorConfig{name, greeting, stock} aggregate-init is the
// pinned contract (test_vendor, GameObjectFactory). Everything below is
// additive with defaults — S5b-3's runtime parser (LoadInterludeVendors)
// fills them; a hand-written 3-field literal still compiles unchanged and
// behaves exactly as before (greetingLines empty -> single greeting,
// karmaOnInteract 0 -> no karma, stockLeft -1 -> unlimited).
struct VendorConfig {
    std::string             name;      // attached NPC display name
    std::string             greeting;  // shown on first Interact (single-line fallback)
    std::vector<VendorItem> stock;     // items offered (price-ordered by author)

    // Every member below carries an explicit default member initializer
    // so a 3-field aggregate literal (VendorConfig{name,greeting,stock})
    // does NOT trip -Wmissing-field-initializers — clang suppresses that
    // warning only when the omitted trailing members are default-init'd
    // (same mechanism as VendorItem::stockLeft). Do not drop the `{}`/`=`.
    std::string              stallKeeper{};       // 攤主 display ("" = none)
    int                      tier = 0;            // 1..4 design grouping (0 = unset)
    std::string              mechanic{};          // buy|donate|sell|game|flavor
    int                      karmaOnInteract = 0; // applied on a successful buy
    std::vector<std::string> greetingLines{};     // multi-line greeting (overrides
                                                  // `greeting` in BuildDialogLines)
    std::vector<std::string> onPurchase{};        // post-transaction flavour
    std::vector<std::string> onLeave{};           // farewell flavour
};

#endif // VENDOR_CONFIG_H_
