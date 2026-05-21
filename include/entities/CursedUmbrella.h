#ifndef CURSED_UMBRELLA_H_
#define CURSED_UMBRELLA_H_
#include "entities/TransparentUmbrella.h"

class CursedUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: deep ominous violet + the sagging Drooping canopy
    // with a pure-black handle — an oppressive "this is wrong" read.
    explicit CursedUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella",
                              nccu::gfx::Color{95, 45, 115, 255},
                              UmbrellaStyle::Drooping),
          karmaPenalty_(30) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] int GetKarmaPenalty() const noexcept { return karmaPenalty_; }

private:
    int karmaPenalty_;
};

#endif // CURSED_UMBRELLA_H_
