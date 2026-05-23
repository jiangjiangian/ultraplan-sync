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

// Item 5b — the successful-buy toast now shows the SPEND, not just the
// raw itemId, so the economy is visible (the owner "didn't see the ugly
// umbrella deduct money"). Format, composed in Vendor::TryBuy from the
// item-catalog 中文 display name + the price + the post-deduction balance:
//   "買了<中文名>，花了 <price> 元（剩 <balance> 元）"
// e.g. "買了螢光綠醜傘，花了 100 元（剩 0 元）". The pieces are pulled
// out as string_view constants (i18n-table-ready, same rationale as the
// other copy here). kPurchasedPrefix is kept as the leading "買了" with
// NO trailing space (the 中文 name follows directly); the old
// "買了 <itemId>" shape is replaced by this richer line.
inline constexpr std::string_view kPurchasedPrefix = "買了";
inline constexpr std::string_view kSpentMid        = "，花了 ";
inline constexpr std::string_view kSpentUnitOpen   = " 元（剩 ";
inline constexpr std::string_view kSpentUnitClose  = " 元）";

// ShowMessage when a finite-stock line is exhausted (stockLeft hit 0):
// the purse is untouched, the purchase did not happen.
inline constexpr std::string_view kSoldOut = "賣完了";

// Stock dialog line composed by BuildDialogLines:
//   <itemId> + kStockLineSep + <price> + kStockLineUnit
inline constexpr std::string_view kStockLineSep  = " - ";
inline constexpr std::string_view kStockLineUnit = " 元";

} // namespace nccu::vendor::msg

#endif // VENDOR_MESSAGES_H_
