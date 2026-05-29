#ifndef DLC_SIGN_H_
#define DLC_SIGN_H_
#include "engine/core/GameObject.h"
#include "engine/math/Vec2.h"
#include <string>

/**
 * @file DlcSign.h
 * @brief 可重複閱讀的彩蛋告示牌：互動時發出 DLC 預告，永不被消耗、無玩法效果。
 */

/**
 * @brief 立於風雩走廊、可重複閱讀的彩蛋告示牌（一個粗大的「？」）。
 *
 * E 互動時發出 ShowMessage 預告（「DLC開發中\n敬請期待」，渲染為兩行置中提示），且「永不」
 * 被消耗，故玩家可隨意重讀（不同於拾取即失效的 QuestFlagPickup／CashPickup）。它無任何玩法
 * 效果：無旗標、無業力、無金錢、無任務鉤子，是純粹的裝飾性點綴，為開放探索的 Chapter4_Finals
 * 而生。
 *
 * 它刻意「不」作為 Item 子類別：Item 是帶 OnPickup 語意與 isPickable_ 的可拾取物，E 互動掃描
 * 會預期一次性收取。告示牌是可對話的固定佈景，故直接落在 GameObject 之下，只扮演它實際需要的
 * 兩個角色：IDrawable（經注入的 IRenderer 自繪「？」，與 UmbrellaGlyph／QuestFlagPickup 同理
 * ——不改 View、不呼叫 DrawText、僅用矩形基本圖元）與 IInteractable（其 Interact 發出預告）。
 * 它的 NpcId() 維持為空、IsVendor() 維持 false，故控制器的 E 互動掃描會將它導向通用的
 * AsInteractable()->Interact() 分支，絕不走 NPC 對話／攤販購買路徑。
 *
 * ISP 角色：IDrawable + IInteractable（無 IUpdatable——告示牌不需逐幀更新）。葉類別，故
 * WithRoles 以 DlcSign 自身為鍵。
 */
class DlcSign final : public WithRoles<DlcSign, GameObject>,
                      public IDrawable, public IInteractable {
public:
    /** @brief 在指定世界座標生成 DLC 告示牌。@param[in] position 世界座標位置（像素）。 */
    explicit DlcSign(nccu::engine::math::Vec2 position);

    /**
     * @brief 繪出粗大的「？」字符（僅用矩形圖元）。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;
    /**
     * @brief 發出預告訊息；可重複閱讀，不使自身失效。
     * @param[in] initiator 互動發起者（不觸碰其狀態，故未使用）。
     *
     * 發布預告 ShowMessage 但「不」設 isActive_ = false，故告示牌在互動後仍存於世界、玩家可
     * 再次閱讀。除了 early return 外無須額外的 null 發起者守衛（它不觸碰任何 Player 狀態）。
     */
    void Interact(Player* initiator) override;

private:
    std::string message_;   ///< 預告訊息文字
};

#endif // DLC_SIGN_H_
