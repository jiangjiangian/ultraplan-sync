#ifndef FRAGILE_UMBRELLA_H_
#define FRAGILE_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

class FragileUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: washed bone-grey + the small Broken canopy (a torn
    // rib) — reads as the cheap / about-to-fail borrowed umbrella.
    explicit FragileUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella",
                              nccu::engine::math::Color{210, 205, 190, 255},
                              UmbrellaStyle::Broken),
          leakRate_(0.5f) {}

    void BeClaimed(Player* player) override;

    [[nodiscard]] float GetLeakRate() const noexcept { return leakRate_; }

private:
    float leakRate_;
};

#endif // FRAGILE_UMBRELLA_H_
