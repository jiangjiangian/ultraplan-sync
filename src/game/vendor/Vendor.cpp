#include "game/vendor/Vendor.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/entities/Player.h"
#include "game/vendor/VendorMessages.h"
#include "game/quest/ItemCatalog.h"
#include "engine/math/Color.h"

#include <string>
#include <utility>
#include <vector>

namespace {

// 小輔助函式，免得只為拼接一行價格就動用 <sstream>。格式："<itemId> - <price> 元"
std::string FormatStockLine(const VendorItem& item) {
    namespace msg = nccu::vendor::msg;
    return item.itemId + std::string(msg::kStockLineSep)
         + std::to_string(item.price) + std::string(msg::kStockLineUnit);
}

} // namespace

std::vector<std::string> Vendor::BuildDialogLines(const VendorConfig& config) {
    // 先放招呼語，再每筆庫存品項一行。刻意保持簡單：目的只是給 NPC::Interact 一串可
    // 循環翻看的內容，讓玩家在叫用 TryBuy 前先預覽價格。
    std::vector<std::string> lines;
    // 解析而來的攤位帶有多行招呼語（greetingLines）；手寫的 3 欄位字面值則只帶單一
    // `greeting`。存在時優先採多行形式，否則退回單行（這維持了固定的 test_vendor 兩行
    // 預期：該設定沒有 greetingLines，故為 招呼語 + 1 筆庫存品項）。
    if (!config.greetingLines.empty()) {
        lines.reserve(config.greetingLines.size() + config.stock.size());
        for (const auto& g : config.greetingLines) lines.push_back(g);
    } else {
        lines.reserve(1 + config.stock.size());
        lines.push_back(config.greeting);
    }
    for (const auto& item : config.stock) {
        lines.push_back(FormatStockLine(item));
    }
    return lines;
}

Vendor::Vendor(nccu::engine::math::Vec2 position, VendorConfig config)
    : NPC(position, BuildDialogLines(config), /*isQuestGiver=*/false,
          config.npcId),   // 透傳身分（多半為空；Ch2 自販機用它參與「!」判定）
      config_(std::move(config)) {}

bool Vendor::TryBuy(Player* player, std::size_t stockIndex) {
    // 先做防禦性邊界檢查——null 玩家或超出範圍的索引，是呼叫端（UI 傳錯插槽）的程式
    // 錯誤，故我們靜默退出、不發任何事件，以免對使用者謊報一筆根本未開始的交易。
    if (!player) return false;
    if (stockIndex >= config_.stock.size()) return false;

    VendorItem& item = config_.stock[stockIndex];

    // 已售完（stockLeft 歸 0；-1 = 無限）。在扣款「之前」檢查，使我們無法交付的品項
    // 絕不動到錢包。
    if (item.stockLeft == 0) {
        nccu::events::Sink().Publish(Event{ EventType::ShowMessage, std::string(nccu::vendor::msg::kSoldOut) });
        return false;
    }

    // DeductMoney 是把關者：餘額不足時回傳 false 且「不」產生任何副作用，故此處玩家的
    // 錢包安全無虞。
    if (!player->DeductMoney(item.price)) {
        nccu::events::Sink().Publish(Event{ EventType::ShowMessage, std::string(nccu::vendor::msg::kInsufficientFunds) });
        return false;
    }

    // 成功：公告這筆交易（UI）與獲得的品項（物品欄）。發兩個事件，因為訂閱者不同——
    // 一個畫提示橫幅，另一個附加到物品欄模型。
    //
    // 提示橫幅現在顯示「花費」——「買了<中文名>，花了 N 元（剩 M 元）」——採用物品圖鑑
    // 的顯示名稱（使集英樓的醜傘與每個市集攤位讀來一致），並用扣款後的餘額（DeductMoney
    // 已於上方執行，故 GetMoney() 即剩餘的錢包）。沒有圖鑑列的 itemId 會退回其原始 id
    //（ItemInfoFor 的退路），故未來的庫存品項絕不會印出空白。
    namespace msg = nccu::vendor::msg;
    const std::string itemName{nccu::ItemInfoFor(item.itemId).displayName};
    const std::string toast =
        std::string(msg::kPurchasedPrefix) + itemName +
        std::string(msg::kSpentMid)        + std::to_string(item.price) +
        std::string(msg::kSpentUnitOpen)   + std::to_string(player->GetMoney()) +
        std::string(msg::kSpentUnitClose);
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, toast });
    nccu::events::Sink().Publish(Event{ EventType::PickupAcquired, item.itemId });

    // 購買確實會進入計數型物品欄，攤位的業力鉤子觸發（例如募款箱 +1；預設 0 = 空操作，
    // 故固定的 test_vendor 攤位不受影響），且有限庫存會遞減（-1 = 無限，故固定攤位
    // 永不遞減）。
    //
    // 買傘（集英樓的醜傘）是「持有」型雨傘，而非計數型消耗品——設定持有種類（同時也設下
    // HasUmbrella，使其自動遮雨並顯示為背包中唯一的雨傘列），且「不」加進計數表（否則會
    // 畫出幻影般的第二列雨傘並誤判為可用品）。下方仍會經 item.setsFlag 設下結局 C 的
    // Flag_BoughtUglyUmbrella，未受更動。
    if (const HeldUmbrella k = nccu::HeldUmbrellaForItemId(item.itemId);
        k != HeldUmbrella::None) {
        player->SetHeldUmbrella(k);
    } else {
        player->AddConsumable(item.itemId);
    }
    if (config_.karmaOnInteract != 0)
        player->AddKarma(config_.karmaOnInteract);
    if (item.stockLeft > 0) --item.stockLeft;
    // 一個可選用的逐品項旗標（預設 "" = 空操作，故每個既有攤位皆不受影響）。集英樓的
    // 醜傘會設下 Flag_BoughtUglyUmbrella → CheckEndingGates 路由至結局 C。
    if (!item.setsFlag.empty()) player->SetFlag(item.setsFlag);
    return true;
}
