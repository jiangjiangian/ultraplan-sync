#ifndef CONTROLLER_VENDOR_MENU_H_
#define CONTROLLER_VENDOR_MENU_H_
#include <string_view>

class Vendor;

namespace nccu {

class DialogState;

/**
 * @file VendorMenu.h
 * @brief 把 Vendor 的庫存組成一段「購買選單」對話的建構器，以及辨識其對話／
 *        放棄選項所需的兩個哨兵常數。屬於 Controller 與對話資料的接縫。
 */

/**
 * @brief 商店對話的哨兵 NPC 情境字串（非真實 npcId）。
 *
 * Vendor 的 NpcId() 為空，故其開啟的對話以此哨兵情境標記。DialogOpener 不會
 * 產生它，suit_senior／ta 的一次性防護只比對真實 id，因此它對既有路由皆無作用。
 * HandleDialog 的確認分支以 dlg.NpcId() == kVendorContext 判定「確認的選項是
 * 一條庫存項目」，進而導向 Vendor::TryBuy。
 */
inline constexpr std::string_view kVendorContext = "__vendor__";

/**
 * @brief 附加在每個商店選單末尾的「放棄購買」標籤。
 *
 * 購買永不可被強迫：玩家永遠能在不花錢、不設旗標的情況下離開。確認分支以
 * 「選中索引 == cfg.stock.size()」（即此項，恆為最後一個選項）辨識為放棄：關閉
 * 對話且不呼叫 Vendor::TryBuy，故不會觸發 DeductMoney／AddConsumable／
 * item.setsFlag／EventBus 購買事件。
 */
inline constexpr std::string_view kVendorDeclineLabel = "先不買，謝謝";

/**
 * @brief 組出商店對話：問候語、每件在庫商品一個選項、最後一個放棄選項。
 * @param[in,out] dlg    要開啟此選單的對話狀態。
 * @param[in]     vendor 提供問候語與庫存清單的攤主。
 *
 * 庫存選項的標籤沿用攤主自己格式化的庫存字串（與 NPC::Interact 瀏覽文字一致）。
 * DialogChoice 本身不帶業力／旗標——所有經濟副作用（DeductMoney、AddConsumable、
 * EventBus 購買事件、金錢軟上限、item.setsFlag）一律留在 Vendor::TryBuy 內，
 * 庫存選項只攜帶其在 Choices() 中的索引。放棄選項不帶任何資料，靠位置（索引 ==
 * 庫存數）辨識，故絕不可能變動狀態。
 */
void OpenVendorMenu(DialogState& dlg, const Vendor& vendor);

} // namespace nccu

#endif // CONTROLLER_VENDOR_MENU_H_
