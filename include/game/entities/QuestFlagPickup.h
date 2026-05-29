#ifndef QUEST_FLAG_PICKUP_H_
#define QUEST_FLAG_PICKUP_H_
#include "game/entities/Item.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

/**
 * @file QuestFlagPickup.h
 * @brief 任務旗標拾取物：拾取時設立具名玩家旗標的一次性地面道具，支援整組完成獎勵。
 */

/**
 * @brief 拾取時設立具名玩家旗標、隨即失效的一次性地面道具，是任務鏈的串接管線。
 *
 * 例如解鎖助教獎勵的四維道申請書，或 Ch2 的散落筆記。會繪出小型地面標記讓玩家探索時可
 * 發現並拾取；經由 E 互動掃描拾取（NpcId() 為空即走 Interact()）。
 *
 * 選用的整組完成獎勵：一個拾取物可攜帶一串姊妹旗標與一份業力加成。當「此」拾取物設立旗標
 * 後使整串都滿足時即發放加成；用於三張筆記集齊後給學霸的業力加成。只有「最後」被撿起者會
 * 看到所有旗標皆已設立（較早者尚有缺口而略過，且早已失效），故加成恰好觸發一次，無須守衛旗標。
 *
 * 選用的「依數量」訊息：當 countMessages_ 非空時，畫面訊息依玩家此刻已持有 completionFlags_
 * 中的「幾個」決定（第 1 個撿到 -> [0]、第 2 個 -> [1]、第 3 個 -> [2]），而非依這是哪個
 * 特定道具——故以任意順序撿筆記都能正確印出「找到第一張／第二張／最後一張」。當 countMessages_
 * 為空則使用單一的 message_（申請書／任何非整組拾取物維持原行為）。
 *
 * ISP 角色：IDrawable + IInteractable。原本的 Update 為空殼（申請書不需逐幀更新），故捨棄
 * 該角色；地面標記的 Render 與設立旗標（經 Interact）的 OnPickup 皆為實作而保留。葉類別，
 * 故 WithRoles 以 QuestFlagPickup 自身為鍵。
 */
class QuestFlagPickup final : public WithRoles<QuestFlagPickup, Item>,
                              public IDrawable, public IInteractable {
public:
    /**
     * @brief 建構任務旗標拾取物。
     * @param[in] position        世界座標位置（像素）。
     * @param[in] flagName        拾取時設立的玩家旗標名稱。
     * @param[in] message         單則畫面訊息（countMessages 為空時採用）。
     * @param[in] completionFlags 整組完成所需的姊妹旗標清單。
     * @param[in] completionKarma 整組集齊時發放的業力加成。
     * @param[in] countMessages   依已持有旗標數量挑選的訊息清單。
     */
    QuestFlagPickup(nccu::engine::math::Vec2 position, std::string flagName,
                    std::string message = "撿到了被風吹走的申請書",
                    std::vector<std::string> completionFlags = {},
                    int completionKarma = 0,
                    std::vector<std::string> countMessages = {});

    /**
     * @brief 依道具種類繪出地面標記（傘旗標畫真傘字符，紙張畫白色紙頁）。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;
    /** @brief 互動即拾取（轉呼叫 OnPickup）。@param[in] initiator 互動發起者（玩家）。 */
    void Interact(Player* initiator) override { OnPickup(initiator); }
    /**
     * @brief 拾取效果：設立旗標、依數量挑選訊息，並在整組集齊時發放業力加成。
     * @param[in] player 拾取者。
     */
    void OnPickup(Player* player) override;

private:
    std::string              flagName_;          ///< 拾取時設立的旗標名稱
    std::string              message_;           ///< 單則畫面訊息（countMessages 為空時採用）
    std::vector<std::string> completionFlags_;   ///< 整組完成所需的姊妹旗標
    int                      completionKarma_;    ///< 整組集齊時的業力加成
    std::vector<std::string> countMessages_;     ///< 依已持有旗標數量挑選的訊息
};
#endif // QUEST_FLAG_PICKUP_H_
