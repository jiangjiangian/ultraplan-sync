#pragma once
#include "TransparentUmbrella.h"

class FragileUmbrella : public TransparentUmbrella {
public:
    explicit FragileUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella", nccu::gfx::Color{200, 220, 235, 255}),
          leakRate_(0.5f) {}

    void beClaimed(Player* player) override;

    float GetLeakRate() const { return leakRate_; }

private:
    float leakRate_;
};
