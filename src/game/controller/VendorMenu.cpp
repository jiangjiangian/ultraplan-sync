#include "game/controller/VendorMenu.h"
#include "game/dialog/DialogState.h"
#include "game/vendor/Vendor.h"
#include <string>
#include <vector>

namespace nccu {

void OpenVendorMenu(DialogState& dlg, const Vendor& vendor) {
    const VendorConfig& cfg = vendor.Config();
    std::vector<std::string> greeting;
    if (!cfg.greetingLines.empty()) greeting = cfg.greetingLines;
    else                            greeting.push_back(cfg.greeting);

    std::vector<DialogChoice> choices;
    for (const auto& item : cfg.stock) {
        // 售罄項目仍會顯示（TryBuy 於確認時回覆「賣完了」），使庫存數量維持可見；
        // 無限量或仍有庫存的項目則可購買。標籤沿用 NPC::Interact 的「<id> - <價> 元」
        // 預覽格式。
        choices.push_back(DialogChoice{
            item.itemId + std::string(" - ") +
                std::to_string(item.price) + std::string(" 元"),
            0, std::string{}, false, {}});
    }
    // 恆存在的放棄選項。附加在「最後」，使庫存選項保有其 0 起算索引 == 庫存槽位
    // （TryBuy(stockIdx) 契約不變）；放棄選項的索引恰為 cfg.stock.size()。即使庫存
    // 為空也存在，使僅有問候語的攤位仍是一個真正、可離開的選項（不強迫陷入死路）。
    // karmaDelta 0／setsFlag 空——它不帶任何副作用。
    choices.push_back(DialogChoice{
        std::string(kVendorDeclineLabel), 0, std::string{}, false, {}});
    dlg.Open(std::move(greeting), std::move(choices));
    dlg.SetNpcContext(std::string(kVendorContext));
}

} // namespace nccu
