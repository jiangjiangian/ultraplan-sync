#ifndef GAME_OBJECT_H_
#define GAME_OBJECT_H_
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "engine/core/Roles.h"   // IUpdatable / IDrawable / IInteractable + WithRoles
#include <string>
#include <string_view>
#include <vector>

/**
 * @file GameObject.h
 * @brief 所有遊戲物件的抽象基底：持有座標、碰撞盒與存活旗標，並以「角色介面」
 *        取代肥大純虛擬，提供能力查詢。
 */

namespace nccu::engine::render { class IRenderer; }
class Player; // 前向宣告——避免角色鉤子造成的循環 include

/**
 * @brief 地圖上每個「東西」（玩家、NPC、道具、裝飾）的共同基底。
 *
 * 只持有最小共同狀態（位置、碰撞盒、是否存活、碰撞層），不含任何渲染或輸入
 * 邏輯。能力依介面隔離原則（ISP）拆成獨立角色介面，由子類別選擇性扮演，避免
 * 每個葉類別被迫實作用不到的空殼函式。
 */
class GameObject {
public:
    /**
     * @brief 以世界座標與碰撞盒建構物件，預設存活、碰撞層為 0。
     * @param position 物件在世界座標系的位置（像素）。
     * @param hitBox   軸對齊碰撞盒（AABB）。
     */
    GameObject(nccu::engine::math::Vec2 position, nccu::engine::math::Rect hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    /// @name 角色能力查詢（ISP；以靜態多型取代 dynamic_cast）
    /// 更新／繪製／互動不再是每個實體都得實作的肥大純虛擬；具體實體只繼承它
    /// 實際扮演的角色介面，由 CRTP 的 WithRoles mixin 在「編譯期」靜態判斷並
    /// 回傳 typed pointer（或 nullptr）。場景容器據此分派，例如
    /// `if (auto* u = o.AsUpdatable()) u->Update(dt);`。回傳 nullptr 表示
    /// 「不扮演此角色」，迴圈會略過。詳見 Roles.h。
    ///@{
    /** @brief 取得可更新能力。@return 扮演 IUpdatable 時回傳指標，否則 nullptr。 */
    virtual IUpdatable*      AsUpdatable()      noexcept { return nullptr; }
    /** @brief 取得可繪製能力。@return 扮演 IDrawable 時回傳指標，否則 nullptr。 */
    virtual const IDrawable* AsDrawable() const noexcept { return nullptr; }
    /** @brief 取得可互動能力。@return 扮演 IInteractable 時回傳指標，否則 nullptr。 */
    virtual IInteractable*   AsInteractable()   noexcept { return nullptr; }
    /**
     * @brief 取得「有生命值／可受傷」能力（IMortal：hp／TakeDamage／IsDead）。
     * @return 扮演 IMortal 時回傳指標，否則 nullptr。
     *
     * 與上三者同形，作為生存／戰鬥機制的擴充點；目前僅 Player 扮演。場景容器可
     * 藉此對所有 mortal 實體統一跑傷害／死亡判定。
     */
    virtual IMortal*         AsMortal()          noexcept { return nullptr; }
    ///@}

    /**
     * @brief 與另一矩形做 AABB 碰撞測試。
     * @param other 要測試的矩形。
     * @return 兩者相交回傳 true。
     */
    [[nodiscard]] bool CheckCollision(nccu::engine::math::Rect other) const noexcept {
        return hitBox_.Intersects(other);
    }

    /** @brief 物件是否仍存活（false 者由幀末清除階段移除）。 */
    [[nodiscard]] bool IsActive() const noexcept { return isActive_; }
    /** @brief 標記為死亡；實際移除延後到幀末，避免迭代途中刪除造成失效。 */
    void Deactivate() noexcept { isActive_ = false; }
    /** @brief 取得世界座標位置。 */
    [[nodiscard]] nccu::engine::math::Vec2 GetPosition() const noexcept { return position_; }

    /**
     * @brief 取得碰撞層標籤（0 為預設層，含玩家、NPC、道具）。
     * @return 目前碰撞層編號。
     *
     * 供生存玩法分層（玩家／敵人／投射物／拾取物）使用，讓碰撞系統決定哪些配對
     * 才需互動；搭配 SetCollisionLayer 可讓生成器不必新增建構參數即標記實體。
     */
    [[nodiscard]] int  GetCollisionLayer() const noexcept {
        return collisionLayer_;
    }
    /**
     * @brief 設定碰撞層。
     * @param layer 新的碰撞層編號。
     */
    void SetCollisionLayer(int layer) noexcept { collisionLayer_ = layer; }

    /**
     * @brief 此物件是否阻擋移動。
     * @return 預設 false（道具、玩家、裝飾）；NPC、牆等覆寫為 true。
     *
     * 以虛擬函式回傳 bool 取代碰撞迴圈中的 dynamic_cast——虛擬分派在繼承下封閉，
     * dynamic_cast 則否。下列幾個查詢函式同此設計理由。
     */
    [[nodiscard]] virtual bool BlocksMovement() const noexcept { return false; }

    /**
     * @brief 查詢可對話台詞。
     * @return 可對話者回傳其台詞向量指標；預設 nullptr（道具、玩家、裝飾）。
     *
     * 回傳指標（而非參考），讓「無對話」不需哨兵向量即可表達。
     */
    [[nodiscard]] virtual const std::vector<std::string>*
        DialogLines() const noexcept { return nullptr; }

    /**
     * @brief 對話查找用的 NPC 身分識別字串。
     * @return 原型 NPC 回傳其 npcId；預設空字串（道具、玩家、攤販、裝飾、路人，
     *         這些改走 Interact() 退路）。
     *
     * GameController 以 (npcId, SemesterState) 組出對應的對話開場。
     */
    [[nodiscard]] virtual std::string_view NpcId() const noexcept { return {}; }

    /**
     * @brief 此物件是否為商店攤販。
     * @return 預設 false；Vendor 覆寫為 true。
     *
     * Vendor 的 NpcId() 為空（它非對話內容 NPC），若缺此旗標，E 互動會誤把它導向
     * NPC 的台詞循環而永遠不觸發購買；GameController 以此驅動購買選單。
     */
    [[nodiscard]] virtual bool IsVendor() const noexcept { return false; }

    /**
     * @brief 此物件是否為任務給予者。
     * @return 預設 false；生成時被標記為任務給予者的原型 NPC 覆寫為 true。
     *
     * View 據此在其 sprite 上方畫「!」提示，讓玩家一眼看出對話鉤子。
     */
    [[nodiscard]] virtual bool IsQuestGiver() const noexcept { return false; }

protected:
    nccu::engine::math::Vec2 position_;   ///< 世界座標位置（像素）
    nccu::engine::math::Rect hitBox_;     ///< 軸對齊碰撞盒（AABB）
    bool isActive_;                       ///< 存活旗標；false 由幀末清除
    int collisionLayer_;                  ///< 碰撞層標籤（0 = 預設層）
};

#endif // GAME_OBJECT_H_
