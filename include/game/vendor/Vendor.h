#ifndef VENDOR_H_
#define VENDOR_H_
#include "game/entities/NPC.h"
#include "game/vendor/VendorConfig.h"
#include "engine/math/Vec2.h"
#include <cstddef>

/**
 * @file Vendor.h
 * @brief 兼任商店櫃檯的 NPC：以 NPC 對話循環瀏覽商品，另開 TryBuy() 作為實際成交入口。
 */

/**
 * @brief 同時是 NPC 又是商店櫃檯的攤販。
 *
 * 仍由基底 NPC 的 Interact() 驅動台詞循環，讓訪客以對話瀏覽商品；專用的 TryBuy()
 * 入口則由 UI／商店畫面在玩家選定某一行後，帶著明確的庫存索引呼叫。瀏覽與成交因此
 * 走兩條路徑，互不干擾。
 */
class Vendor final : public NPC {
public:
    Vendor(nccu::engine::math::Vec2 position, VendorConfig config);

    /**
     * @brief 嘗試把 stock_[stockIndex] 賣給玩家。
     * @param player     買方玩家。
     * @param stockIndex 欲購買的庫存索引。
     * @return 成交回傳 true，否則 false。
     *
     * 三種結果：索引越界→false 且不發任何事件；DeductMoney 失敗（錢不夠）→false 並
     * ShowMessage「你錢不夠」；成交→true 並送出 ShowMessage 與 PickupAcquired。
     */
    [[nodiscard]] bool TryBuy(class Player* player, std::size_t stockIndex);

    /**
     * @brief 標記此物件為商店攤販。
     * @return 恆為 true。
     *
     * Vendor 的 NpcId() 為空（非對話內容 NPC），GameController 需要一個可分派的虛擬
     * 函式，才能把商店互動導向購買選單 UI，而非 NPC::Interact 的台詞循環（後者永遠到
     * 不了 TryBuy）。採虛擬分派而非 dynamic_cast，理由同 NPC::BlocksMovement()。
     */
    [[nodiscard]] bool IsVendor() const noexcept override { return true; }

    /** @brief 取得此攤販的設定（greeting、庫存、機制等）。 */
    [[nodiscard]] const VendorConfig& Config() const noexcept { return config_; }

private:
    /**
     * @brief 由 greeting 與各庫存項組出 NPC 對話台詞。
     * @param config 攤販設定。
     * @return 組好的台詞向量。
     *
     * 由建構式呼叫一次；若日後庫存會變動，需重跑此函式並把新台詞透過 SetDialogLines
     * 灌回去。
     */
    static std::vector<std::string> BuildDialogLines(const VendorConfig& config);

    VendorConfig config_;
};

#endif // VENDOR_H_
