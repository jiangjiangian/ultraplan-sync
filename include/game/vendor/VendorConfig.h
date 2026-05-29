#ifndef VENDOR_CONFIG_H_
#define VENDOR_CONFIG_H_
#include <string>
#include <vector>

/**
 * @file VendorConfig.h
 * @brief 攤販（Vendor）的純資料設定：單一商品列與整攤設定，刻意與遊戲執行期
 *        標頭解耦，讓內容層能直接以 POD 字面值建構攤位。
 */

/**
 * @brief 描述掛在攤販 NPC 上的一筆可販售商品。
 *
 * 刻意保持精簡，使內容層（main.cpp、之後的 SCRIPT 載入器）能建出攤位庫存表而
 * 不必相依任何遊戲執行期標頭。
 */
struct VendorItem {
    std::string itemId;   ///< 商品代號，例如 "HotPack"、"TrueUmbrella"
    int         price;    ///< 金錢花費；交由 Player::DeductMoney 比對
    int         stockLeft = -1;  ///< -1 = 無限；>0 每次購買遞減；0 = 售罄（TryBuy 在扣款前即失敗）
    /**
     * @brief 購買成功時要 SetFlag 的 Player 旗標；"" 表示不設。
     *
     * 預設空字串，使 2／3 欄位的 VendorItem 字面值行為與舊版一致（與 stockLeft
     * 同樣靠尾端成員預設來抑制 -Wmissing-field-initializers）。集英樓的醜傘以此
     * 設定 Flag_BoughtUglyUmbrella（Ending C 的觸發條件）。
     */
    std::string setsFlag{};
};

/**
 * @brief 一個市集攤位的整合設定。
 *
 * Vendor 類別在建構式中消費它，藉以推導顯示名稱與初始對話列，所以新增一個攤位
 * 只差一個 POD 字面值。
 *
 * 前三個成員是原始形狀且「必須」維持此順序／型別：VendorConfig{name, greeting,
 * stock} 的聚合初始化是釘住的契約（test_vendor、GameObjectFactory 依賴它）。以
 * 下成員皆為帶預設值的附加欄位——由執行期解析器（LoadInterludeVendors）填入；手
 * 寫的 3 欄位字面值仍可原封不動編譯且行為不變（greetingLines 為空 -> 單行
 * greeting；karmaOnInteract 為 0 -> 無業力；stockLeft -1 -> 無限）。
 */
struct VendorConfig {
    std::string             name;      ///< 掛載 NPC 的顯示名稱
    std::string             greeting;  ///< 首次 Interact 顯示（單行退路）
    std::vector<VendorItem> stock;     ///< 提供的商品（由作者依價格排序）

    /// @name 帶預設值的附加欄位
    /// 以下每個成員都帶有明確的預設成員初始化，使 3 欄位聚合字面值
    /// （VendorConfig{name,greeting,stock}）不會觸發 -Wmissing-field-initializers
    /// ——clang 僅在省略的尾端成員都被預設初始化時才抑制該警告（與
    /// VendorItem::stockLeft 同機制）。切勿移除 `{}`／`=`。
    ///@{
    std::string              stallKeeper{};       ///< 攤主顯示名（"" = 無）
    int                      tier = 0;            ///< 1..4 設計分組（0 = 未設）
    std::string              mechanic{};          ///< buy|donate|sell|game|flavor
    int                      karmaOnInteract = 0; ///< 購買成功時施加的業力
    std::vector<std::string> greetingLines{};     ///< 多行問候（在 BuildDialogLines 中覆蓋 `greeting`）
    std::vector<std::string> onPurchase{};        ///< 交易後的風味台詞
    std::vector<std::string> onLeave{};           ///< 道別風味台詞
    ///@}

    /**
     * @brief 選用的整張圖 sprite 路徑（例如自動販賣機機台美術）。
     *
     * "" 時攤販改用每索引的 Pipoya 人物退路（VendorSpriteFor）。設定後 World 會
     * 載入它並標記 Vendor 的 staticSprite_，使 NPC::Render 繪製整張貼圖而非 32×32
     * 的 Pipoya 單格——機台美術是單張圖，不是角色表。
     */
    std::string              spriteOverride{};
};

#endif // VENDOR_CONFIG_H_
