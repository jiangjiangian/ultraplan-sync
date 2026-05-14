#ifndef FRAGILE_UMBRELLA_H_
#define FRAGILE_UMBRELLA_H_
#include "TransparentUmbrella.h"

class FragileUmbrella final : public TransparentUmbrella {
public:
    explicit FragileUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella", nccu::gfx::Color{200, 220, 235, 255}),
          leakRate_(0.5f) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] float GetLeakRate() const noexcept { return leakRate_; }

private:
    float leakRate_;
};

#endif // FRAGILE_UMBRELLA_H_
