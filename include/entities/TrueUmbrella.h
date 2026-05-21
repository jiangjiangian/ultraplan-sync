#ifndef TRUE_UMBRELLA_H_
#define TRUE_UMBRELLA_H_
#include "entities/TransparentUmbrella.h"

class TrueUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: bright sky-cyan + the full Domed canopy — the
    // unambiguous "this is the clean / correct umbrella" read.
    explicit TrueUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "TrueUmbrella",
                              nccu::gfx::Color{70, 190, 255, 255},
                              UmbrellaStyle::Domed) {}

    void beClaimed(Player* player) override;
};

#endif // TRUE_UMBRELLA_H_
