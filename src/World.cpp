#include "World.h"
#include "GameObjectFactory.h"
#include "NPC.h"
#include "NpcSpawns.h"
#include "Obstacles.h"
#include "gfx/Vec2.h"

namespace nccu {

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

    // Static collision is fully authored now: tools/tiled_to_world.py
    // emits colliders::kAll (Tiled tile-collision shapes rasterised to
    // AABB rects, footprint fallback for un-traced buildings, the river
    // appended). The sprite rect in buildings::kAll is a trigger zone
    // only — BuildingTracker keys chapter events off it.
    staticColliders_.assign(colliders::kAll.begin(), colliders::kAll.end());

    // Ambient pedestrians — wired AFTER staticColliders_ is filled so the
    // self-resolving wander stays out of buildings and the river. Each
    // gets a distinct PRNG seed so the crowd doesn't move in lock-step.
    unsigned seed = 0x1234567u;
    for (const auto& s : AmbientStudentSpawns()) {
        auto npc = std::make_unique<NPC>(s.pos, s.dialog, s.isQuestGiver);
        npc->LoadSprite(s.spritePath);
        npc->EnableWander(50.0f, seed);
        npc->SetWanderColliders(staticColliders_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}

} // namespace nccu
