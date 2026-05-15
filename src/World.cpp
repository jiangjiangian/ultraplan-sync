#include "World.h"
#include "GameObjectFactory.h"
#include "NPC.h"
#include "NpcSpawns.h"
#include "Buildings.h"
#include "Obstacles.h"
#include "gfx/Vec2.h"
#include <algorithm>
#include <iterator>

namespace nccu {
namespace {

// The 3/4-oblique building art has its solid ground floor in the LOWER
// band of the sprite; the roof and eaves overhang the path above it. So
// the collider is only the bottom footprint — kFootprintFrac of the
// trigger-rect height, inset on x for the side walls. The overhang and
// the whole area behind the building stay walkable (and the full
// trigger-rect still fires the entry event via BuildingTracker).
// Open-ground "buildings" (track, plaza) are skipped — Obstacles.h
// supplies their strip colliders instead.
constexpr float kBuildingInset = 24.0f;
constexpr float kFootprintFrac = 0.40f;

bool IsCollisionBuilding(const buildings::Building& b) noexcept {
    const auto& skips = obstacles::kBuildingCollisionSkip;
    return std::find(skips.begin(), skips.end(), b.name) == skips.end();
}

nccu::gfx::Rect ToCollider(const buildings::Building& b) noexcept {
    const float fh = b.triggerRect.height * kFootprintFrac;
    return nccu::gfx::Rect{
        b.triggerRect.x + kBuildingInset,
        b.triggerRect.y + b.triggerRect.height - fh,
        b.triggerRect.width - 2.0f * kBuildingInset,
        fh};
}

} // namespace

World::World(const std::string& playerSpritePath) {
    using nccu::gfx::Vec2;

    // Player on Zhinan Rd east of 正門, clear of every wall/NPC hitbox so
    // the AABB resolver never has to rescue them at frame 0. The 4
    // umbrellas sit on the central strip between plaza and Si Wei Blvd.
    objects_.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{500, 1860}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{ 320, 1280}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{ 750, 1280}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1180, 1280}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    for (const auto& spawn : DefaultNpcSpawns()) {
        auto npc = std::make_unique<NPC>(spawn.pos, std::vector<std::string>{},
                                         spawn.isQuestGiver, spawn.npcId);
        npc->LoadSprite(spawn.spritePath);
        objects_.push_back(std::move(npc));
    }

    player_ = dynamic_cast<Player*>(objects_.front().get());
    if (player_) player_->LoadSprite(playerSpritePath);

    staticColliders_.reserve(buildings::kAll.size() + obstacles::kAll.size());
    for (const auto& b : buildings::kAll) {
        if (IsCollisionBuilding(b)) staticColliders_.push_back(ToCollider(b));
    }
    std::copy(obstacles::kAll.begin(), obstacles::kAll.end(),
              std::back_inserter(staticColliders_));

    // Ambient pedestrians — wired AFTER staticColliders_ is filled so the
    // self-resolving wander stays out of buildings and the river. Each
    // gets a distinct PRNG seed so the crowd doesn't move in lock-step.
    unsigned seed = 0x1234567u;
    for (const auto& s : AmbientStudentSpawns()) {
        auto npc = std::make_unique<NPC>(s.pos, std::vector<std::string>{},
                                         s.isQuestGiver, s.npcId);
        npc->LoadSprite(s.spritePath);
        npc->EnableWander(50.0f, seed);
        npc->SetWanderColliders(staticColliders_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}

} // namespace nccu
