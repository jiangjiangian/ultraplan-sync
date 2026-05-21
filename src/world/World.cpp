#include "world/World.h"
#include "entities/CashPickup.h"
#include "quest/ChapterPickups.h"
#include "quest/ChapterQuestItems.h"
#include "quest/ChapterSpawns.h"
#include "quest/ChapterVendors.h"
#include "controller/GameObjectFactory.h"
#include "entities/NPC.h"
#include "quest/NpcSpawns.h"
#include "quest/PipoyaRoster.h"
#include "entities/QuestFlagPickup.h"
#include "vendor/Vendor.h"
#include "vendor/VendorSprite.h"
#include "gfx/Vec2.h"
#include <algorithm>
#include <cassert>
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
        if (loadSprites_)
            npc->LoadSprite(PickNpcSprite(s.npcId, s.pos, s.spritePath));
        npc->EnableWander(50.0f, seed);
        npc->SetWanderMask(terrainMask_);
        objects_.push_back(std::move(npc));
        seed = seed * 1664525u + 1013904223u;
    }
}

void World::SpawnChapterNpcs(nccu::SemesterState state) {
    // M7 (cycle9c): Ch4 「斥責學長後不出場」 ripple. chapter4.md L6 promises
    // a player who scolded the suit_senior in Ch1 (Flag_ScoldedSenior) won't
    // see him again at the finals — unless they later mended the
    // relationship (Flag_HelpedSenior, the Ch2 callback note). Filter at
    // spawn-time so the suit_senior is simply absent from objects_, which
    // is identically how every other "NPC not in this chapter" case looks
    // (e.g. librarian only in Ch2): the dialog opener cannot target him,
    // the routing in Chapter4Quest sees nothing to scold/help further, and
    // the chapterRoster_ sweep already handles teardown if he stays absent
    // through the next Transition. Conditioning on player_ keeps the
    // headless World unit tests (no Player) defensive even though the
    // ctor caches player_ before the first respawn.
    const bool skipScoldedSenior =
        state == SemesterState::Chapter4_Finals &&
        player_ != nullptr &&
        player_->HasFlag("Flag_ScoldedSenior") &&
        !player_->HasFlag("Flag_HelpedSenior");

    for (const auto& spawn : ChapterNpcSpawns(state)) {
        if (skipScoldedSenior &&
            std::string_view(spawn.npcId) == "suit_senior") {
            continue;
        }
        auto npc = std::make_unique<NPC>(spawn.pos, std::vector<std::string>{},
                                         spawn.isQuestGiver, spawn.npcId);
        if (loadSprites_)
            npc->LoadSprite(
                PickNpcSprite(spawn.npcId, spawn.pos, spawn.spritePath));
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
    // REQUIREMENT #6: every market stall must be a DIFFERENT person.
    // The old code passed the literal "vendor" + a single shop_auntie.png
    // fallback for ALL stalls, so on a clean clone (PIPOYA pack absent →
    // fallback path) the ten 攤主 rendered as ten clones of the same
    // sprite. The per-stall selection rule now lives in one pure
    // function (VendorSprite.h VendorSpriteFor) shared with its
    // regression test, so the "ten distinct people" guarantee is
    // exercised through the real production code path.
    std::size_t vendorIdx = 0;
    for (const auto& vp : ChapterVendors(state)) {
        auto vendor = std::make_unique<Vendor>(vp.pos, vp.config);
        // Gated by loadSprites_ exactly like the chapter NPCs above, so
        // the headless World unit tests (loadSprites=false) skip the GPU
        // upload. VendorSpriteFor keys off the stall's own unique 攤主/
        // name and picks a distinct curated fallback per spawn index, so
        // a clean clone still shows ten different people (not ten
        // shop_auntie clones) and the PIPOYA path no longer collides.
        if (loadSprites_)
            vendor->LoadSprite(VendorSpriteFor(
                vendorIdx, vp.config.stallKeeper, vp.config.name,
                vp.pos));
        ++vendorIdx;
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
