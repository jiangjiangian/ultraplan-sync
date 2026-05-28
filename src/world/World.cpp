#include "world/World.h"
#include "entities/CashPickup.h"
#include "entities/DlcSign.h"
#include "quest/ChapterPickups.h"
#include "quest/ChapterQuestItems.h"
#include "quest/ChapterSpawns.h"
#include "quest/ChapterVendors.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "controller/GameObjectFactory.h"
#include "entities/NPC.h"
#include "quest/NpcSpawns.h"
#include "quest/PipoyaRoster.h"
#include "entities/QuestFlagPickup.h"
#include "vendor/Vendor.h"
#include "vendor/VendorSprite.h"
#include "engine/math/Vec2.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <unordered_set>

namespace nccu {

World::World(const std::string& playerSpritePath, bool loadSprites)
    : loadSprites_(loadSprites) {
    using nccu::gfx::Vec2;

    // Cycle 9.E (audit D8 / SC 2.3.3): pick up the reduced-motion
    // accessibility preference from the environment so the engine side
    // works without a pause-menu UI yet. Accepts "1" (anything else,
    // including unset or "0", leaves the default false). A future UI
    // PR can flip this through SetReducedMotion(); the env-var path
    // remains for headless / scripted contexts and the harness.
    if (const char* env = std::getenv("UMBRELLA_REDUCED_MOTION");
        env != nullptr && std::strcmp(env, "1") == 0) {
        reducedMotion_ = true;
    }

    // Cycle 9.E (audit M2 / D7 / SC 2.5.8): pick up the "larger targets"
    // accessibility profile from the environment, exactly the
    // UMBRELLA_REDUCED_MOTION shape above. Accepts "1" only; anything
    // else (unset/"0") leaves the default false. A future pause-menu UI
    // can flip this via SetLargeTargets(); the env-var path serves
    // headless / scripted contexts (the harness, an end-user's
    // accessibility profile script). Read at construction so the choice
    // is consistent for the whole session.
    if (const char* env = std::getenv("UMBRELLA_LARGE_TARGETS");
        env != nullptr && std::strcmp(env, "1") == 0) {
        largeTargets_ = true;
    }

    // Player on Zhinan Rd east of 正門, clear of every wall/NPC hitbox so
    // the AABB resolver never has to rescue them at frame 0. The 3 morality
    // umbrellas sit on the central strip between plaza and Si Wei Blvd.
    //
    // 善有善報 redesign: the world TrueUmbrella is GONE. The player's真傘 is
    // no longer claimable off the ground — the 苦主 grants it once the
    // player carries HIS umbrella back (TryReturnVictimUmbrella). The
    // Cursed / Fragile / ProfessorTrap umbrellas REMAIN as the morality /
    // Ending-B-etc. paths and still clear Ch1 via their own beClaimed (the
    // three-ending architecture is untouched — CLAUDE.md §5).
    objects_.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{500, 1860}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{ 750, 1280}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1200, 1256}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    // Ch1 跑腿道具：被風吹走的加退選申請書，落在四維堂南側空地。撿起 ->
    // Flag_FoundForm -> 助教給獎勵對白（集英樓 2 樓線索）。
    objects_.push_back(std::make_unique<QuestFlagPickup>(
        nccu::gfx::Vec2{560.0f, 1725.0f}, kFlagFoundForm));

    // Cache the Player BEFORE spawning chapter NPCs so the front-is-
    // Player invariant is established up front and never disturbed:
    // SpawnChapterNpcs only appends at the back.
    player_ = dynamic_cast<Player*>(objects_.front().get());
    if (player_) player_->LoadSprite(playerSpritePath);

    // Ch1 spawns through the same state-aware path the FSM later drives,
    // so the initial roster is the identical 5 archetypes.
    RespawnChapterRoster(semester_.Current());

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
        if (loadSprites_)
            npc->LoadSprite(PickNpcSprite(s.npcId, s.pos, s.spritePath));
        npc->EnableWander(50.0f, seed);
        npc->SetWanderMask(terrainMask_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}


void World::RespawnChapterRoster(nccu::SemesterState state) {
    // Build the drop-set from the tracked pointers, then clear the
    // tracker FIRST so the raw pointers are never dereferenced after
    // their unique_ptr is freed — the erase predicate compares pointer
    // identity only (no deref). The single remove-erase pass runs AFTER
    // we are done iterating chapterRoster_, never mid-iteration, and
    // never reorders/removes element 0 (Player) since the Player was
    // never in chapterRoster_.
    if (!chapterRoster_.empty()) {
        const std::unordered_set<GameObject*> drop(chapterRoster_.begin(),
                                                   chapterRoster_.end());
        chapterRoster_.clear();
        objects_.erase(
            std::remove_if(objects_.begin(), objects_.end(),
                [&drop](const std::unique_ptr<GameObject>& o) {
                    return drop.find(o.get()) != drop.end();
                }),
            objects_.end());
    }

    // Re-arm the Ch2 deferred-note one-shot: the old notes (if any) were
    // just swept with the roster, so a fresh visit to any chapter starts
    // with the deferred spawn re-armed. MaybeSpawnChapter2Notes then only
    // fires once the player re-wakes the 學霸 in a new Ch2 visit.
    ch2NotesSpawned_ = false;
    // T5: same re-arm for the Ch3 reveal-after-clue umbrella one-shot.
    ch3UmbrellaSpawned_ = false;
    // A1: same re-arm for the Ch1 reveal-after-choice victim's-umbrella
    // one-shot, so a fresh Ch1 visit re-gates it on Flag_SuitSeniorChoiceMade.
    ch1VictimUmbrellaSpawned_ = false;
    // G-3: same re-arm for the Interlude 管理員的傘 return-point one-shot, so a
    // fresh Interlude visit re-gates it on (ReturnTo==Ch3 && holds loaner).
    interludeReturnSpawned_ = false;

    SpawnChapterNpcs(state);

    // Invariant guard: front must still be the live Player and the
    // cached pointer must still address it. Both hold by construction
    // (we only ever appended at the back and dropped tracked NPCs that
    // were never element 0), this asserts it rather than trusting it.
    if (player_) {
        assert(!objects_.empty() &&
               objects_.front().get() == static_cast<GameObject*>(player_));
    }
}

void World::Sweep() {
    // Snapshot the player's death BEFORE erase — the cached player_
    // pointer dangles the instant its owning unique_ptr is destroyed
    // (heap-use-after-free per [basic.stc.dynamic.deallocation]/4). The
    // bool snapshot sidesteps that: we decide whether to clear the cache
    // while the object still exists, then erase, then act on the snapshot.
    const bool playerWillDie = player_ && !player_->IsActive();
    objects_.erase(
        std::remove_if(objects_.begin(), objects_.end(),
            [](const std::unique_ptr<GameObject>& o) {
                return !o || !o->IsActive();
            }),
        objects_.end());
    if (playerWillDie) ClearPlayer();
}

} // namespace nccu
