#ifndef VENDOR_MESSAGES_H_
#define VENDOR_MESSAGES_H_
#include <string_view>

/**
 * @file VendorMessages.h
 * @brief Vendor 對外顯示的全部 zh-TW 文案常數，自 Vendor.cpp 抽出。
 *
 * 把文案與交易邏輯分離，兩者因不同理由而變動（SRP）。維持為 string_view 常數，也讓
 * 日後的 i18n 對照表可直接抽換。
 */
namespace nccu::vendor::msg {

/// @brief DeductMoney 失敗時（ShowMessage 事件）顯示；錢包未動，交易未發生。
inline constexpr std::string_view kInsufficientFunds = "你錢不夠";

/// @name 成交提示文案的組件
/// 成交提示會顯示「花費」而非僅 itemId，讓經濟回饋可見。由 Vendor::TryBuy 以
/// 物品目錄的中文顯示名＋價格＋扣款後餘額組成：
/// `買了<中文名>，花了 <price> 元（剩 <balance> 元）`，
/// 例如 `買了螢光綠醜傘，花了 100 元（剩 0 元）`。各片段抽成 string_view 常數
/// （同樣便於 i18n 對照表）。kPurchasedPrefix 為開頭「買了」且尾端不留空白（中文名直接
/// 接在後）。
///@{
inline constexpr std::string_view kPurchasedPrefix = "買了";
inline constexpr std::string_view kSpentMid        = "，花了 ";
inline constexpr std::string_view kSpentUnitOpen   = " 元（剩 ";
inline constexpr std::string_view kSpentUnitClose  = " 元）";
///@}

/// @brief 有限庫存售罄（stockLeft 歸 0）時的 ShowMessage；錢包未動，交易未發生。
inline constexpr std::string_view kSoldOut = "賣完了";

/// @name 庫存對話行的組件（由 BuildDialogLines 組成）
/// 格式為 `<itemId>` + kStockLineSep + `<price>` + kStockLineUnit。
///@{
inline constexpr std::string_view kStockLineSep  = " - ";
inline constexpr std::string_view kStockLineUnit = " 元";
///@}

} // namespace nccu::vendor::msg

#endif // VENDOR_MESSAGES_H_
