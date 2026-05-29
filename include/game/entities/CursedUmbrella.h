#ifndef CURSED_UMBRELLA_H_
#define CURSED_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

class CursedUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: deep ominous violet + the sagging Drooping canopy
    // with a pure-black handle — an oppressive "this is wrong" read.
    explicit CursedUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella",
                              nccu::gfx::Color{95, 45, 115, 255},
                              UmbrellaStyle::Drooping) {}

    // P2: pickup no longer applies an immediate karma penalty. It increments
    // Player::cursedTaint_ instead; the per-chapter ApplyCursedTaintDecay
    // (SceneRouter Ch2/3/4 entry) bleeds -5 * taint each transition so the
    // moral cost is felt cumulatively across the run rather than as a single
    // sticker shock. Flag_TookCursedUmbrella is still set (Ending B path).
    void beClaimed(Player* player) override;
};

#endif // CURSED_UMBRELLA_H_
