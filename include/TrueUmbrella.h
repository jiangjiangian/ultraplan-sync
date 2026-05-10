#pragma once
#include "TransparentUmbrella.h"

class TrueUmbrella : public TransparentUmbrella {
public:
    explicit TrueUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "TrueUmbrella", nccu::gfx::Color{180, 230, 255, 255}) {}

    void beClaimed(Player* player) override;
};
