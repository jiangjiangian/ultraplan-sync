#include "World.h"
#include "CashPickup.h"
#include "ChapterPickups.h"
#include "ChapterQuestItems.h"
#include "ChapterSpawns.h"
#include "ChapterVendors.h"
#include "GameObjectFactory.h"
#include "NPC.h"
#include "NpcSpawns.h"
#include "QuestFlagPickup.h"
#include "Vendor.h"
#include "gfx/Vec2.h"
#include <algorithm>
#include <cassert>
#include <unordered_set>

namespace nccu {

World::World(const std::string& playerSpritePath, bool loadSprites)
    : loadSprites_(loadSprites) {
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
        if (loadSprites_) npc->LoadSprite(s.spritePath);
        npc->EnableWander(50.0f, seed);
        npc->SetWanderMask(terrainMask_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}

void World::SpawnChapterNpcs(nccu::SemesterState state) {
    for (const auto& spawn : ChapterNpcSpawns(state)) {
        auto npc = std::make_unique<NPC>(spawn.pos, std::vector<std::string>{},
                                         spawn.isQuestGiver, spawn.npcId);
        if (loadSprites_) npc->LoadSprite(spawn.spritePath);
        chapterRoster_.push_back(npc.get());   // record before the move
        objects_.push_back(std::move(npc));
    }

    // Vendors are the price-table sibling of the NPC roster (their
    // placement carries a VendorConfig, not a sprite path + npcId, so
    // they get their own table). Tracked in chapterRoster_ so the next
    // state change sweeps them exactly like an NPC. ChapterVendors() is
    // empty for every state until S5b-3 transcribes the Interlude
    // lineup, so today this loop is a no-op — it only proves the spawn
    // MECHANISM, with zero behaviour change.
    for (const auto& vp : ChapterVendors(state)) {
        auto vendor = std::make_unique<Vendor>(vp.pos, vp.config);
        chapterRoster_.push_back(vendor.get());
        objects_.push_back(std::move(vendor));
    }

    // CashPickups: the exploration earner of the loop economy. Tracked
    // in chapterRoster_ like the NPCs/Vendors, so a coin not collected
    // before the chapter ends is swept with the roster (one shot per
    // chapter visit; money already banked lives on the Player). Ch1 has
    // a concrete spread today; other states' tables fill in S5c/d/e.
    for (const auto& pp : ChapterPickups(state)) {
        auto coin = std::make_unique<CashPickup>(pp.pos, pp.value);
        chapterRoster_.push_back(coin.get());
        objects_.push_back(std::move(coin));
    }

    // Quest items: chapter-scoped QuestFlagPickups (Ch2 = the 3 散落
    // 筆記, S5c-2). Roster-tracked like the coins, so an uncollected
    // note is swept on the next state change instead of leaking into
    // the Interlude / next chapter. Ch1's 申請書 is NOT here — it stays
    // ctor-spawned (a permanent Ch1 object); ChapterQuestItems(Ch1) is
    // empty so this loop is a no-op for every state but Ch2.
    for (const auto& qi : ChapterQuestItems(state)) {
        auto item = std::make_unique<QuestFlagPickup>(
            qi.pos, qi.flag, qi.message, qi.completionFlags,
            qi.completionKarma);
        chapterRoster_.push_back(item.get());
        objects_.push_back(std::move(item));
    }

    // Ch3 校慶運動會 (S5d-2): the TrueUmbrella the 啦啦隊 took, sitting
    // in the 體育館後台道具箱. Claiming it is the chapter clear, exactly
    // Ch1-isomorphic — beClaimed fires UmbrellaClaimed and the
    // EventWiring Ch3 sibling-if routes to the Interlude (returnTo Ch4).
    // The 物物交換鏈 (TryAdvanceCh3Trade) is the karma / narrative path,
    // not a hard gate (mirrors Ch1's optional umbrella quest). One
    // single-chapter object, so it is spawned inline rather than via a
    // 5th per-state table (no speculative no-caller infra). Roster-
    // tracked, so it is swept if the player leaves Ch3 uncleared.
    // Ch3 道具箱 (claim = Ch1-isomorphic clear via EventWiring) AND Ch4
    // (chapter4.md L6 傘再度失蹤 — the player re-finds the TrueUmbrella;
    // claiming it does NOT clear Ch4, it only satisfies Ending A's
    // 持-TrueUmbrella condition via Flag_HasTrueUmbrella, since no
    // EventWiring Ch4 sibling-if exists). Same proven coord; different
    // state so no overlap. Roster-tracked, swept on state change.
    if (state == SemesterState::Chapter3_SportsDay ||
        state == SemesterState::Chapter4_Finals) {
        auto umb = GameObjectFactory::Create(
            ObjectType::TrueUmbrella, nccu::gfx::Vec2{1500.0f, 1430.0f});
        chapterRoster_.push_back(umb.get());
        objects_.push_back(std::move(umb));
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

} // namespace nccu
