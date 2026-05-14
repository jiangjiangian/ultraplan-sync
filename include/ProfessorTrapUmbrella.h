#ifndef PROFESSOR_TRAP_UMBRELLA_H_
#define PROFESSOR_TRAP_UMBRELLA_H_
#include "TransparentUmbrella.h"

class ProfessorTrapUmbrella final : public TransparentUmbrella {
public:
    explicit ProfessorTrapUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella", nccu::gfx::Color{210, 200, 230, 255}),
          spawnedEnemiesCount_(0) {}

    void beClaimed(Player* player) override;

    [[nodiscard]] int GetSpawnedEnemiesCount() const noexcept { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;
};

#endif // PROFESSOR_TRAP_UMBRELLA_H_
