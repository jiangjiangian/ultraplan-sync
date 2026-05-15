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

// Each building's trigger-rect shrunk uniformly so the outer frame stays
// walkable (and still fires the entry trigger) while the interior is a
// wall. Open-ground "buildings" (track, plaza) are skipped — Obstacles.h
// supplies their strip colliders instead.
constexpr float kBuildingInset = 32.0f;

bool IsCollisionBuilding(const buildings::Building& b) noexcept {
    const auto& skips = obstacles::kBuildingCollisionSkip;
    return std::find(skips.begin(), skips.end(), b.name) == skips.end();
}

nccu::gfx::Rect ToCollider(const buildings::Building& b) noexcept {
    return nccu::gfx::Rect{
        b.triggerRect.x + kBuildingInset,
        b.triggerRect.y + kBuildingInset,
        b.triggerRect.width  - 2.0f * kBuildingInset,
        b.triggerRect.height - 2.0f * kBuildingInset};
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
        auto npc = std::make_unique<NPC>(spawn.pos, spawn.dialog, spawn.isQuestGiver);
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
}

} // namespace nccu
