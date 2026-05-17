#ifndef VENDOR_MESSAGES_H_
#define VENDOR_MESSAGES_H_
#include <string_view>

// All player-facing zh-TW strings the Vendor emits, pulled out of
// Vendor.cpp so transaction logic and UI copy change for different
// reasons (SRP S4 in STRICT_REVIEW_R3). Keeping them as string_view
// constants also makes a future i18n table a drop-in swap.
namespace nccu::vendor::msg {

// Shown (ShowMessage event) when DeductMoney fails — the purse is
// untouched, the purchase did not happen.
inline constexpr std::string_view kInsufficientFunds = "你錢不夠";

// ShowMessage toast prefix on a successful buy: kPurchasedPrefix + itemId.
inline constexpr std::string_view kPurchasedPrefix = "買了 ";

// ShowMessage when a finite-stock line is exhausted (stockLeft hit 0):
// the purse is untouched, the purchase did not happen.
inline constexpr std::string_view kSoldOut = "賣完了";

// Stock dialog line composed by BuildDialogLines:
//   <itemId> + kStockLineSep + <price> + kStockLineUnit
inline constexpr std::string_view kStockLineSep  = " - ";
inline constexpr std::string_view kStockLineUnit = " 元";

} // namespace nccu::vendor::msg

#endif // VENDOR_MESSAGES_H_
