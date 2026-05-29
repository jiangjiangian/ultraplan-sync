#ifndef PROFESSOR_TRAP_UMBRELLA_H_
#define PROFESSOR_TRAP_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

class ProfessorTrapUmbrella final : public TransparentUmbrella {
public:
    // REQUIREMENT #9: alarm amber-orange + the angular Spiked canopy —
    // a weaponised, "this is a trap" silhouette, unmistakable vs True.
    explicit ProfessorTrapUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella",
                              nccu::engine::math::Color{255, 140, 30, 255},
                              UmbrellaStyle::Spiked),
          spawnedEnemiesCount_(0) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] int GetSpawnedEnemiesCount() const noexcept { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;
};

#endif // PROFESSOR_TRAP_UMBRELLA_H_
