#ifndef PROFESSOR_TRAP_UMBRELLA_H_
#define PROFESSOR_TRAP_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

/**
 * @file ProfessorTrapUmbrella.h
 * @brief 教授陷阱傘葉類別：以 BeClaimed 覆寫播下負面漣漪因子並標記 trap 旗標。
 */

/**
 * @brief 偷了教授的傘所觸發的陷阱傘，雨傘 Template Method 樹的具體葉類別之一。
 *
 * 以警示琥珀橙色搭配尖角的 Spiked 傘面，給出武裝化、「這是陷阱」的剪影，與 True 截然有別。
 */
class ProfessorTrapUmbrella final : public TransparentUmbrella {
public:
    /** @brief 在指定世界座標生成教授陷阱傘（固定橙色 Spiked 外形）。@param[in] position 世界座標位置（像素）。 */
    explicit ProfessorTrapUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella",
                              nccu::engine::math::Color{255, 140, 30, 255},
                              UmbrellaStyle::Spiked),
          spawnedEnemiesCount_(0) {}

    /**
     * @brief 認領教授陷阱傘：設立持傘狀態與 trap 旗標，種下後續章節的負面漣漪。
     * @param[in] player 認領者。
     */
    void BeClaimed(Player* player) override;

    /** @brief 取得已生成的追兵數。@return 模擬生成的助教 NPC 數量。 */
    [[nodiscard]] int GetSpawnedEnemiesCount() const noexcept { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;   ///< 模擬生成的追兵（助教）數量
};

#endif // PROFESSOR_TRAP_UMBRELLA_H_
