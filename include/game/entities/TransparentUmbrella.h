#ifndef TRANSPARENT_UMBRELLA_H_
#define TRANSPARENT_UMBRELLA_H_
#include "game/entities/Item.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"

/**
 * @file TransparentUmbrella.h
 * @brief 雨傘繼承樹的抽象中介（Template Method）：定稿拾取／繪製流程，BeClaimed
 *        為各葉類別覆寫的多型認領鉤子。
 */

/**
 * @brief 各子類別雨傘的外形輪廓，供 Render 在每個葉類別間切換剪影。
 *
 * Ch1 玩家要在四把傘之間做抉擇，它們必須一眼可辨，而非四個幾近相同的淡藍色字符。
 * Render 依此繪出每個剪影，即使在色調細微的螢幕上、未及辨色之前也能分辨。此為物
 * 件上的純資料，MVC 保持乾淨（View 於 Render 讀取；World／Item 不含 raylib）。
 */
enum class UmbrellaStyle {
    Domed,    ///< True——寬圓頂傘面：「乾淨／正確」的視覺印象
    Broken,   ///< Fragile——僅剩手柄／裸露傘骨（破損）
    Spiked,   ///< ProfessorTrap——尖角階梯狀傘面（危險／陷阱）
    Drooping  ///< Cursed——下垂暗色傘面＋黑色手柄（錯誤）
};

/**
 * @brief 將子類別的 UmbrellaStyle 映射到共用的 nccu::game::gfx::UmbrellaLook。
 * @param[in] style 子類別的傘面外形列舉。
 * @return 對應的共用外觀列舉（剪影＋識別色的唯一真實來源）。
 *
 * 同一份映射同時供此處的地圖內 Render 與任何想要相同外觀的其他表面使用；刻意與列舉
 * 內聯放在一起，使兩者永不漂移。
 */
[[nodiscard]] constexpr nccu::game::gfx::UmbrellaLook
LookForStyle(UmbrellaStyle style) noexcept {
    switch (style) {
        case UmbrellaStyle::Domed:    return nccu::game::gfx::UmbrellaLook::TrueBlue;
        case UmbrellaStyle::Broken:   return nccu::game::gfx::UmbrellaLook::FragileBroken;
        case UmbrellaStyle::Spiked:   return nccu::game::gfx::UmbrellaLook::ProfessorTrap;
        case UmbrellaStyle::Drooping: return nccu::game::gfx::UmbrellaLook::CursedPurple;
    }
    return nccu::game::gfx::UmbrellaLook::TrueBlue;
}

/**
 * @brief Item 與各具體雨傘之間的抽象中介，承載雨傘繼承樹的 Template Method 骨架。
 *
 * 拾取與繪製流程在本層定稿，可變的多型動詞 BeClaimed()（認領效果）延後到各葉類別。
 * 四個葉類別（True／Fragile／ProfessorTrap／Cursed）僅覆寫 BeClaimed()。
 *
 * ISP 角色：IDrawable + IInteractable。原本的 Update 為空殼（雨傘不需逐幀更新），
 * 故捨棄該角色；Render（依外形繪字符）與 Interact（受任務閘控的認領）皆為實作而保留。
 * 四個葉類別共用此角色集，故 WithRoles 以本中介層為鍵（Derived = TransparentUmbrella），
 * static_cast 至此對每個葉類別皆合法。
 */
class TransparentUmbrella : public WithRoles<TransparentUmbrella, Item>,
                            public IDrawable, public IInteractable {
public:
    /**
     * @brief 以世界座標、名稱、色調與外形建構雨傘（碰撞盒固定 20×20）。
     * @param[in] position 世界座標位置（像素）。
     * @param[in] name     雨傘名稱（兼作背包 itemId）。
     * @param[in] tint     雨傘色調。
     * @param[in] style    外形輪廓，預設為 Domed。
     */
    TransparentUmbrella(nccu::engine::math::Vec2 position, std::string name,
                        nccu::engine::math::Color tint,
                        UmbrellaStyle style = UmbrellaStyle::Domed)
        : WithRoles(position, nccu::engine::math::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint), style_(style) {}

    /**
     * @brief 經由 IRenderer 依外形繪出雨傘字符（Template Method 中的固定步驟）。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;

    /** @brief 取得外形輪廓。@return 此傘的 UmbrellaStyle。 */
    [[nodiscard]] UmbrellaStyle Style() const noexcept { return style_; }
    /**
     * @brief 互動即認領（與 OnPickup 共用同一道任務閘）。
     * @param[in] initiator 互動發起者（玩家）。
     *
     * 兩條拾取路徑都經過同一道任務閘（定義於 .cpp 以便看見 Player／EventBus）：
     * 唯有玩家已接下苦主的請求（Flag_PromisedVictim）後才能認領，否則發出引導提示而
     * 非無聲略過。到 Ch3／Ch4 該旗標早已設立，故該處 TrueUmbrella 的再認領不受影響。
     */
    void Interact(Player* initiator) override;
    /**
     * @brief 拾取即認領（與 Interact 共用同一道任務閘）。
     * @param[in] player 拾取者。
     */
    void OnPickup(Player* player) override;

    /**
     * @brief 多型認領鉤子：玩家取得此傘時施放各自的結果效果。
     * @param[in] player 認領者。
     */
    virtual void BeClaimed(Player* player) = 0;

protected:
    nccu::engine::math::Color umbrellaTint_;        ///< 雨傘色調
    UmbrellaStyle    style_{UmbrellaStyle::Domed};  ///< 外形輪廓
};

#endif // TRANSPARENT_UMBRELLA_H_
