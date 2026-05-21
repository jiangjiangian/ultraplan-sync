#ifndef FRAGILE_UMBRELLA_H_
#define FRAGILE_UMBRELLA_H_
#include "entities/TransparentUmbrella.h"

class FragileUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: washed bone-grey + the small Broken canopy (a torn
    // rib) — reads as the cheap / about-to-fail borrowed umbrella.
    explicit FragileUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella",
                              nccu::gfx::Color{210, 205, 190, 255},
                              UmbrellaStyle::Broken),
          leakRate_(0.5f) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] float GetLeakRate() const noexcept { return leakRate_; }

private:
    float leakRate_;
};

#endif // FRAGILE_UMBRELLA_H_
