#include "World.h"
#include "GameObjectFactory.h"
#include "NPC.h"
#include "NpcSpawns.h"
#include "QuestFlagPickup.h"
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
    objects_.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1200, 1256}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    // Ch1 跑腿道具：被風吹走的加退選申請書，落在四維堂南側空地。撿起 ->
    // Flag_FoundForm -> 助教給獎勵對白（集英樓 2 樓線索）。
    objects_.push_back(std::make_unique<QuestFlagPickup>(
        nccu::gfx::Vec2{560.0f, 1725.0f}, "Flag_FoundForm"));

    for (const auto& spawn : DefaultNpcSpawns()) {
        auto npc = std::make_unique<NPC>(spawn.pos, std::vector<std::string>{},
                                         spawn.isQuestGiver, spawn.npcId);
        npc->LoadSprite(spawn.spritePath);
        objects_.push_back(std::move(npc));
    }

    player_ = dynamic_cast<Player*>(objects_.front().get());
    if (player_) player_->LoadSprite(playerSpritePath);

    // Static collision is a pixel-accurate walkability mask now:
    // tools/tiled_to_world.py bakes building wall bases + the river into
    // collision_mask_base.png, the artist paints trees / planters / the
    // perimeter wall on top into collision_mask.png. The sprite rect in
    // buildings::kAll is a trigger zone only — BuildingTracker keys
    // chapter events off it.
    terrainMask_ = LoadTerrainMask();

    // Ambient pedestrians — wired AFTER the mask is loaded so the
    // self-resolving wander stays out of buildings and the river. Each
    // gets a distinct PRNG seed so the crowd doesn't move in lock-step.
    unsigned seed = 0x1234567u;
    for (const auto& s : AmbientStudentSpawns()) {
        auto npc = std::make_unique<NPC>(s.pos, std::vector<std::string>{},
                                         s.isQuestGiver, s.npcId);
        npc->LoadSprite(s.spritePath);
        npc->EnableWander(50.0f, seed);
        npc->SetWanderMask(terrainMask_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}

} // namespace nccu
