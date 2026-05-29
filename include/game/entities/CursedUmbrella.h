#ifndef CURSED_UMBRELLA_H_
#define CURSED_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

/**
 * @file CursedUmbrella.h
 * @brief 詛咒傘葉類別：以 BeClaimed 覆寫累加道德污點並標記通往 Ending B 的旗標。
 */

/**
 * @brief 「順手牽羊」的詛咒傘，雨傘 Template Method 樹的具體葉類別之一。
 *
 * 以深沉不祥的紫色搭配下垂的 Drooping 傘面與純黑手柄，給出壓迫「這是錯的」的視覺印象。
 */
class CursedUmbrella final : public TransparentUmbrella {
public:
    /** @brief 在指定世界座標生成詛咒傘（固定紫色 Drooping 外形）。@param[in] position 世界座標位置（像素）。 */
    explicit CursedUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella",
                              nccu::engine::math::Color{95, 45, 115, 255},
                              UmbrellaStyle::Drooping) {}

    /**
     * @brief 認領詛咒傘：累加道德污點計數並設立 Ending B 路徑旗標。
     * @param[in] player 認領者。
     *
     * 拾取不再當場扣業力，而是遞增 Player::cursedTaint_；改由每章的
     * ApplyCursedTaintDecay（SceneRouter Ch2/3/4 進場）每次過場扣 -5 × 污點，使道德
     * 代價隨整場遊戲累積反映，而非一次性的當頭重擊。Flag_TookCursedUmbrella 仍會設立。
     */
    void BeClaimed(Player* player) override;
};

#endif // CURSED_UMBRELLA_H_
