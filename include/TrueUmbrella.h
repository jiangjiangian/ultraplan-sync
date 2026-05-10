#pragma once
#include "TransparentUmbrella.h"

class TrueUmbrella : public TransparentUmbrella {
public:
    explicit TrueUmbrella(Vector2 position)
        : TransparentUmbrella(position, "TrueUmbrella", Color{180, 230, 255, 255}) {}

    void beClaimed(Player* player) override;
};
