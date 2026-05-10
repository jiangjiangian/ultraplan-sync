#pragma once
#include "TransparentUmbrella.h"

class ProfessorTrapUmbrella : public TransparentUmbrella {
public:
    explicit ProfessorTrapUmbrella(Vector2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella", Color{210, 200, 230, 255}),
          spawnedEnemiesCount_(0) {}

    void beClaimed(Player* player) override;

    int GetSpawnedEnemiesCount() const { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;
};
