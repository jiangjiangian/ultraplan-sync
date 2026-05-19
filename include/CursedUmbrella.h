#ifndef CURSED_UMBRELLA_H_
#define CURSED_UMBRELLA_H_
#include "TransparentUmbrella.h"

class CursedUmbrella final : public TransparentUmbrella {
public:
    explicit CursedUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella", nccu::gfx::Color{120, 100, 140, 255}),
          karmaPenalty_(30) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] int GetKarmaPenalty() const noexcept { return karmaPenalty_; }

private:
    int karmaPenalty_;
};

#endif // CURSED_UMBRELLA_H_
