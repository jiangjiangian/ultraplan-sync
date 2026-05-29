#ifndef ENTITY_ROLES_H_
#define ENTITY_ROLES_H_
#include <concepts>

/**
 * @file Roles.h
 * @brief 角色介面拆分（ISP）與 CRTP 靜態分派 mixin：讓實體只宣告自己實際扮演的能力。
 *
 * 在此之前 GameObject 是「肥大介面」：每個實體都得實作 Update / Render /
 * Interact，即使函式體只是空 no-op（如 ConsumableItem 的 Update/Render、
 * Player 的 Interact）。將這三項職責拆成三個獨立角色介面，讓類別精確宣告它
 * 真正扮演的角色——即介面隔離原則（ISP）。異質場景容器仍持有 GameObject* 並
 * 透過 GameObject 的能力查詢（AsUpdatable / AsDrawable / AsInteractable）分
 * 派，使執行期多型保留在該在的位置（一個 vector、多種具體型別）。
 *
 * 能力查詢本身則以「靜態多型」綁定：WithRoles 是 CRTP mixin，於編譯期以
 * std::derived_from + if constexpr 偵測最末端型別繼承了哪些角色介面，並回傳
 * static_cast 後的指標——不用 dynamic_cast、不做每次呼叫的型別檢查。
 */

namespace nccu::engine::render { class IRenderer; }
class Player;  // 角色鉤子收 Player* 發起者；此處不需要完整型別

/** @brief 可更新角色：每幀依時間步進演進自身狀態。 */
struct IUpdatable {
    virtual ~IUpdatable() = default;
    /**
     * @brief 依時間差更新狀態。
     * @param[in] deltaTime 距上一幀的秒數。
     */
    virtual void Update(float deltaTime) = 0;
};

/** @brief 可繪製角色：透過 IRenderer 抽象介面把自身畫到畫面上。 */
struct IDrawable {
    virtual ~IDrawable() = default;
    /**
     * @brief 將自身繪製至給定的渲染服務。
     * @param[in] renderer 抽象渲染介面（每個 raylib 繪圖呼叫都藏在其後）。
     */
    virtual void Render(nccu::engine::render::IRenderer& renderer) const = 0;
};

/** @brief 可互動角色：被玩家以 E 鍵觸發時執行對話／購買／拾取等行為。 */
struct IInteractable {
    virtual ~IInteractable() = default;
    /**
     * @brief 由發起者觸發互動。
     * @param[in] initiator 觸發互動的玩家。
     */
    virtual void Interact(Player* initiator) = 0;
};

/**
 * @brief 可受傷角色：擁有生命值、可被傷害／擊殺。
 *
 * 與上三個介面同形（無資料成員、僅行為合約）的第四個獨立角色。生存玩法需要有
 * 生命值的玩家與敵人；把「可受傷」獨立成角色，能讓它不落入 GameObject 基底
 * （道具、裝飾並不「可受傷」），正是 ISP 所要的。hp 由具體類別自行持有，葉類別
 * 以繼承並覆寫的方式選擇性加入。
 */
struct IMortal {
    virtual ~IMortal() = default;
    /**
     * @brief 施加傷害（下限夾為 0）。
     * @param[in] amount 傷害量；負值忽略（回血請走獨立 API，傷害只會降低 hp）。
     *
     * noexcept——位於戰鬥熱迴圈，絕不丟例外。
     */
    virtual void TakeDamage(int amount) noexcept = 0;
    /** @brief 是否已耗盡生命值。@return 死亡回傳 true。 */
    [[nodiscard]] virtual bool IsDead() const noexcept = 0;
    /** @brief 取得目前生命值（>= 0），供 HUD／測試／戰鬥數值使用。 */
    [[nodiscard]] virtual int  Hp() const noexcept = 0;
};

/**
 * @brief CRTP 分派 mixin：在編譯期靜態實作 GameObject 的角色能力查詢。
 *
 * WithRoles<Derived, Base> 插入於既有 GameObject 衍生狀態基底（Item /
 * Character / ConsumableItem / TransparentUmbrella …）與最末端 Derived 之間，
 * 於編譯期偵測 Derived 繼承了哪些角色介面，據此實作能力查詢。套用在「角色集合
 * 已固定」的層級：當某中介層下所有葉類別共享相同角色時套在中介層
 * （ConsumableItem、TransparentUmbrella、NPC），葉類別彼此不同時則套在葉類別。
 */
template <class Derived, class Base>
class WithRoles : public Base {
public:
    using Base::Base;   // 繼承 Base 的建構子

    IUpdatable* AsUpdatable() noexcept override {
        if constexpr (std::derived_from<Derived, IUpdatable>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
    const IDrawable* AsDrawable() const noexcept override {
        if constexpr (std::derived_from<Derived, IDrawable>)
            return static_cast<const Derived*>(this);
        else
            return nullptr;
    }
    IInteractable* AsInteractable() noexcept override {
        if constexpr (std::derived_from<Derived, IInteractable>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
    IMortal* AsMortal() noexcept override {
        if constexpr (std::derived_from<Derived, IMortal>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
};

/**
 * @brief 泛型角色分派輔助：對容器中扮演指定 Role 的每個存活物件呼叫 fn。
 * @param[in,out] objects std::unique_ptr<GameObject> 的容器（或元素可解參為
 *                        GameObject& 的任意範圍）。
 * @param[in]     fn      對每個 As<Role>() 非 null 的物件呼叫的可呼叫物。
 *
 * Role 與能力查詢的對映以 if constexpr 在編譯期解析，因此日後新增角色只是這裡
 * 多一個分支，而非每個呼叫點都要改。
 *
 * 注意：僅可變角色（IUpdatable / IInteractable / IMortal）走此輔助。const 的
 * Render 路徑（IDrawable）由 View 直接分派——其畫家演算法（painter's order）本
 * 來就需要 GameObject 取得深度排序鍵，直接分派比在此加 const 重載更清楚。
 */
template <class Role, class Container, class F>
void ForEachRole(Container& objects, F&& fn) {
    for (auto& obj : objects) {
        if (!obj || !obj->IsActive()) continue;
        if constexpr (std::same_as<Role, IUpdatable>) {
            if (IUpdatable* r = obj->AsUpdatable()) fn(*r);
        } else if constexpr (std::same_as<Role, IInteractable>) {
            if (IInteractable* r = obj->AsInteractable()) fn(*r);
        } else if constexpr (std::same_as<Role, IMortal>) {
            // 走訪每個可受傷實體（玩家與敵人），讓傷害／死亡掃描收斂為單次呼叫
            if (IMortal* r = obj->AsMortal()) fn(*r);
        } else {
            static_assert(std::same_as<Role, IUpdatable>,
                          "ForEachRole supports the mutable roles "
                          "(IUpdatable / IInteractable / IMortal); render "
                          "through GameObject::AsDrawable() directly.");
        }
    }
}

#endif // ENTITY_ROLES_H_
