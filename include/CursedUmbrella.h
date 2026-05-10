#pragma once
#include "TransparentUmbrella.h"

class CursedUmbrella : public TransparentUmbrella {
public:
    explicit CursedUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella", nccu::gfx::Color{120, 100, 140, 255}),
          karmaPenalty_(50) {}

    void beClaimed(Player* player) override;

    int GetKarmaPenalty() const { return karmaPenalty_; }

private:
    int karmaPenalty_;
};
