#ifndef CONTROLLER_VENDOR_MENU_H_
#define CONTROLLER_VENDOR_MENU_H_
#include <string_view>

class Vendor;

namespace nccu {

class DialogState;

// I5: a Vendor has an empty NpcId(), so its open conversation is tagged
// with this sentinel context (NOT a real npcId — DialogOpener never
// produces it, the C.3 suit_senior / ta one-shot guards compare against
// real ids only, so this is inert for every existing routing). The
// dialog confirm branch (HandleDialog) reads dlg.NpcId() == kVendorContext
// to know the confirmed choice is a stock line and route it to
// Vendor::TryBuy.
inline constexpr std::string_view kVendorContext = "__vendor__";

// The trailing "不買" (decline) label appended to every vendor menu.
// REQUIREMENT #4: a purchase must never be forced — the player can
// always walk away without spending money or setting a flag. The
// confirm branch recognises a chosen index == cfg.stock.size() (this
// entry, always the LAST choice) as "decline": it closes the dialog
// and does NOT call Vendor::TryBuy, so no DeductMoney / AddConsumable /
// item.setsFlag / EventBus purchase event fires.
inline constexpr std::string_view kVendorDeclineLabel = "先不買，謝謝";

// Build the shop conversation: the greeting line(s) then one choice per
// in-stock item, then a final "不買" decline choice. The stock choice
// label is the vendor's own formatted stock line (NPC::Interact already
// cycles those, so this reuses the exact same browsing text). No
// karma/flag on the DialogChoice itself — the economy side-effects
// (DeductMoney, AddConsumable, the EventBus purchase events, the
// soft-cap, item.setsFlag) ALL stay inside Vendor::TryBuy, untouched;
// the stock choice only carries the index (its position in Choices()).
// The decline choice carries nothing and is detected positionally
// (index == stock size) so it can never mutate state — REQUIREMENT #4.
void OpenVendorMenu(DialogState& dlg, const Vendor& vendor);

} // namespace nccu

#endif // CONTROLLER_VENDOR_MENU_H_
