#ifndef CHARACTER_H_
#define CHARACTER_H_
#include "engine/core/GameObject.h"
#include "engine/math/Vec2.h"

/**
 * @file Character.h
 * @brief 可移動角色的抽象基底：在 GameObject 之上加入速度、朝向與幀率無關的位移。
 */

/**
 * @brief 會在地圖上移動之角色（玩家、NPC）的共同基底。
 *
 * 集中提供「正規化方向 + 速度 × 時間」的位移與位置同步邏輯，讓 Player 與 NPC
 * 共用同一套移動語意，避免每個子類別各自重寫座標與碰撞盒的同步。
 */
class Character : public GameObject {
public:
    /**
     * @brief 以世界座標、碰撞盒與移動速度建構角色。
     * @param position 世界座標位置（像素）。
     * @param hitBox   軸對齊碰撞盒（AABB）。
     * @param speed    移動速度，單位為像素／秒（與幀率無關）。
     */
    Character(nccu::engine::math::Vec2 position, nccu::engine::math::Rect hitBox, float speed)
        : GameObject(position, hitBox), speed_(speed),
          direction_({0.0f, 0.0f}), currentFrame_(0) {}

    /**
     * @brief 依方向與經過時間移動角色，並同步碰撞盒與朝向。
     * @param dir       移動方向（無需預先正規化）。
     * @param deltaTime 本幀經過的秒數。
     *
     * 先將 dir 正規化，斜向才不會比正向快 sqrt(2) 倍；再以 speed_ × deltaTime
     * 推進位置。
     */
    void Move(nccu::engine::math::Vec2 dir, float deltaTime) {
        const nccu::engine::math::Vec2 n = dir.Normalized();
        position_.x += n.x * speed_ * deltaTime;
        position_.y += n.y * speed_ * deltaTime;
        hitBox_.x = position_.x;
        hitBox_.y = position_.y;
        direction_ = n;
    }

    /**
     * @brief 直接設定世界座標，並讓碰撞盒同步跟進。
     * @param p 新的世界座標位置（像素）。
     *
     * 供 Update 後套用世界邊界夾制（ClampToWorld）等外部修正使用。
     */
    void SetPosition(nccu::engine::math::Vec2 p) noexcept {
        position_ = p;
        hitBox_.x = p.x;
        hitBox_.y = p.y;
    }

protected:
    float speed_;                         ///< 移動速度（像素／秒）
    nccu::engine::math::Vec2 direction_;  ///< 最近一次的移動朝向（已正規化）
    int currentFrame_;                    ///< 動畫影格索引
};

#endif // CHARACTER_H_
