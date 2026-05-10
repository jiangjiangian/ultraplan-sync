#pragma once
#include "TransparentUmbrella.h"

class ProfessorTrapUmbrella : public TransparentUmbrella {
public:
    explicit ProfessorTrapUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella", nccu::gfx::Color{210, 200, 230, 255}),
          spawnedEnemiesCount_(0) {}

    void beClaimed(Player* player) override;

    int GetSpawnedEnemiesCount() const { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;
};
