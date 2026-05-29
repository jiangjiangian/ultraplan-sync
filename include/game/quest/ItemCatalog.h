#ifndef ITEM_CATALOG_H_
#define ITEM_CATALOG_H_
#include "game/entities/Player.h"   // class Player + HeldUmbrella（背包列來源）
#include <string>
#include <string_view>
#include <vector>

class EventBus;                 // ApplyConsumableEffect 由外部注入

namespace nccu {

/**
 * @file ItemCatalog.h
 * @brief 玩家可持有之每件物品的「單一」程式目錄與相關查詢。
 *
 * 以引擎其餘部分共用的同一個 itemId 為鍵（消耗品計數表的鍵、Vendor 庫存 itemId，
 * 以及背包 DTO 合成的雨傘／任務物品哨兵值）。純資料——查詢本身不碰 raylib、不存取
 * World/Player——使 Tab 背包 DTO（BuildInventoryRows）與 Vendor 購買提示都從「同一
 * 處」讀取中文顯示名與描述而不致漂移。
 *
 * 禁字規範：此處每個字串皆為符合劇情的繁中，不出現任何外部工具／模型／品牌識別
 * 字。
 */
struct ItemInfo {
    std::string_view displayName;   ///< 背包／提示顯示的中文名稱
    std::string_view description;   ///< 一行簡短說明：它的作用
};

// 非消耗品背包分類的哨兵 itemId（計數表只會持有消耗品／攤販的 itemId，故這些絕不
// 會與真實的鍵相撞）。背包 DTO 以它們查找金錢、各種攜帶雨傘與任務紙張的中文名與
// 描述——把「所有」文案集中在這一份目錄。
inline constexpr const char* kItemMoney         = "__money__";
inline constexpr const char* kItemTrueUmbrella  = "__umbrella_true__";
inline constexpr const char* kItemCursedUmbrella = "__umbrella_cursed__";
inline constexpr const char* kItemUglyUmbrella  = "__umbrella_ugly__";
inline constexpr const char* kItemVictimUmbrella = "__umbrella_victim__";
// 其餘 HeldUmbrella 種類的手持傘哨兵，使背包能顯示玩家實際能撐在頭上的「每一把」
// 傘（破傘／陷阱傘地上拾取物與 Ch2 管理員的傘借傘），以 HeldUmbrellaKind() 為鍵而
// 非持久的結局旗標。
inline constexpr const char* kItemFragileUmbrella  = "__umbrella_fragile__";
inline constexpr const char* kItemProfTrapUmbrella = "__umbrella_proftrap__";
inline constexpr const char* kItemLoanerUmbrella   = "__umbrella_loaner__";
inline constexpr const char* kItemForm          = "__quest_form__";
inline constexpr const char* kItemNotes         = "__quest_notes__";
// Ch3 物物交換鏈的攜帶物品（香腸／大聲公）。與申請書／筆記一樣是以旗標建模的一次
// 性任務物品；在背包中呈現使交換鏈可見（且在某次交換消費掉前一個旗標的當下即消失
// ——Chapter3Quest 會清除它）。情報（KnowsUmbrellaLoc）是知識而非攜帶物，故無背包
// 列。
inline constexpr const char* kItemSausage       = "__quest_sausage__";
inline constexpr const char* kItemLoudspeaker   = "__quest_loudspeaker__";

/**
 * @brief 取得某手持傘種類的目錄哨兵 itemId（背包列與 InventoryView 字形據以為鍵）。
 * @param kind 手持傘種類。
 * @return 對應哨兵；None／Victim 回傳 nullptr（苦主攜帶的傘以其任務旗標顯示，而非
 *         手持種類，因它不提供遮蔽）。
 *
 * 純對應——不存取 Player。
 */
[[nodiscard]] const char* HeldUmbrellaCatalogId(HeldUmbrella kind);

/**
 * @brief 把攤販庫存的雨傘 itemId（如 "UglyUmbrella"）對應到購買後手持的傘種類。
 * @param itemId 攤販庫存 itemId。
 * @return 對應的 HeldUmbrella 種類；非雨傘 id 回傳 HeldUmbrella::None（常見情況——
 *         食物／飲料／裝備仍為計數消耗品）。
 *
 * 使買來的傘成為「手持傘」（一列背包傘列＋自動遮蔽），而非幽靈計數項目。純對應。
 */
[[nodiscard]] HeldUmbrella HeldUmbrellaForItemId(std::string_view itemId);

/**
 * @brief 取得 itemId 的目錄條目。
 * @param itemId 物品 id。
 * @return 對應的 ItemInfo；未知 id 回傳合理退路（{itemId 作為名稱, ""}），絕不丟
 *         例外。
 *
 * 使未來無目錄列的庫存物品仍渲染其原始 id，而非崩潰或印出空白。
 */
[[nodiscard]] ItemInfo ItemInfoFor(std::string_view itemId);

/**
 * @brief 列出目錄中每個 displayName 與 description 字串，供字形覆蓋掃描使用。
 * @return 攤平內部目錄表後的字串向量；順序未指定。
 *
 * 這些名稱／描述會渲染在背包列與攤販購買提示，故每個字形都必須烘進字型。純資料。
 */
[[nodiscard]] std::vector<std::string> CatalogStrings();

/**
 * @brief itemId 是否為玩家可從背包「使用」的消耗品。
 * @param itemId 物品 id。
 * @return 可使用回傳 true。
 *
 * 驅動 DTO 的 usable 旗標並把守「從背包使用」效果：只有這些 id 有可套用的效果；金
 * 錢／雨傘／任務紙張皆為唯讀條目。
 */
[[nodiscard]] bool IsUsableConsumable(std::string_view itemId);

/**
 * @brief itemId 是否指稱一把雨傘（任一目錄雨傘哨兵或攤販雨傘庫存 id）。
 * @param itemId 物品 id。
 * @return 含子字串 "Umbrella"/"umbrella" 即回傳 true。
 *
 * 撐在頭上的傘由手持種類列呈現，而「非」計數迴圈，故 BuildInventoryRows 把這些自
 * 消耗品計數中「排除」，而 InventoryView 也據此把某列分類為雨傘。單一來源，使兩個
 * 呼叫端（目錄的列建構器與 InventoryView 的列分類器）永不漂移。
 */
[[nodiscard]] bool IsUmbrellaItemId(std::string_view itemId);

/**
 * @brief 玩家從開啟的背包使用某手持消耗品時套用其效果。
 * @param bus    事件匯流排（由此發布風味與各效果 ShowMessage，與舊拾取路徑一致）。
 * @param player 玩家（施加業力／雨量變化）。
 * @param itemId 物品 id。
 *
 * 這與每個 ConsumableItem::Consume 內套用的「同一」效果（相同業力差、相同
 * ShowMessage 文字，共用實體的業力加成常數），如今改由玩家觸發而非拾取時觸發。對
 * 不可使用／未知 id 為 no-op。它「不」碰計數表——由呼叫端（控制器）透過
 * Player::ConsumeOne 遞減，使「花掉一個」的記帳集中在一處。
 */
void ApplyConsumableEffect(EventBus& bus, Player& player,
                           std::string_view itemId);

/**
 * @brief Tab 背包的一列「唯讀」資料（View 繪製用的 DTO）。
 *
 * 與 EndingSummary 同形：一個由基本型別組成的扁平結構，不持有 World/Player 控柄、
 * 不含玩法邏輯——InventoryView 只負責渲染它。itemId 隨列攜帶，使控制器能把選取列
 * 對應回消耗品 id 以供 ApplyConsumableEffect ＋ ConsumeOne，而不必從顯示名重新推
 * 導。
 */
struct InventoryRow {
    std::string name;          ///< 中文顯示名稱
    int         count = 0;     ///< >0 顯示 "xN"；0 = 單一實例，無後綴
    std::string description;   ///< 一行功能說明
    bool        usable = false; ///< E/Enter 是否套用效果？
    std::string itemId;        ///< 引擎 id（消耗品鍵），供從背包使用
};

/**
 * @brief 從 Player 組出整個背包。
 * @param player 玩家（唯讀讀取旗標與計數表）。
 * @return 背包列向量；空背包回傳空向量（View 改畫其「（空）」佔位）。
 *
 * 內容：金幣（餘額，標註為跨章節）、每個手持消耗品（數量＋作用）、攜帶的雨傘（由
 * 玩家持有的 Flag_* 推導）、以及當前進度找到的任務紙張（申請書／Ch2 三頁筆記）。
 * 空分類直接省略。建構於任務層——它讀取 Player 旗標與計數表（唯讀）及目錄；View
 * 絕不為之。順序確定：金幣，再依 itemId 排序的消耗品，再雨傘，再任務紙張——使面板
 * 與回歸測試皆穩定。
 */
[[nodiscard]] std::vector<InventoryRow> BuildInventoryRows(const Player& player);

}  // namespace nccu

#endif  // ITEM_CATALOG_H_
