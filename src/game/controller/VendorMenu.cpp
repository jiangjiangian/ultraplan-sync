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
        // Sold-out lines are still shown (TryBuy answers "賣完了" on
        // confirm) so the stock count stays visible; an unlimited or
        // in-stock line is buyable. Label mirrors NPC::Interact's
        // "<id> - <price> 元" preview.
        choices.push_back(DialogChoice{
            item.itemId + std::string(" - ") +
                std::to_string(item.price) + std::string(" 元"),
            0, std::string{}, false, {}});
    }
    // REQUIREMENT #4: the always-present decline option. Appended LAST
    // so a stock choice keeps its 0-based index == its stock slot (the
    // pinned TryBuy(stockIdx) contract is unchanged); the decline index
    // is exactly cfg.stock.size(). Present even when stock is empty so a
    // greeting-only stall is still a real, exitable choice (no forced
    // dead-end). karmaDelta 0 / setsFlag "" — it carries no side effect.
    choices.push_back(DialogChoice{
        std::string(kVendorDeclineLabel), 0, std::string{}, false, {}});
    dlg.Open(std::move(greeting), std::move(choices));
    dlg.SetNpcContext(std::string(kVendorContext));
}

} // namespace nccu
