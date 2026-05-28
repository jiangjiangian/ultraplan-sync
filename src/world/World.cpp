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
        player_->HasFlag(kFlagScoldedSenior) &&
        !player_->HasFlag(kFlagHelpedSenior);

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
        if (loadSprites_) {
            // A spriteOverride (the 自動販賣機 machine art) renders as a
            // whole static image; otherwise pick a distinct Pipoya person.
            if (!vp.config.spriteOverride.empty()) {
                vendor->LoadSprite(vp.config.spriteOverride);
                vendor->SetStaticSprite(true);
            } else {
                vendor->LoadSprite(VendorSpriteFor(
                    vendorIdx, vp.config.stallKeeper, vp.config.name,
                    vp.pos));
            }
        }
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

    // Quest items: chapter-scoped QuestFlagPickups. Roster-tracked like
    // the coins, so an uncollected item is swept on the next state change
    // instead of leaking into the Interlude / next chapter. Ch1's 申請書
    // is NOT here — it stays ctor-spawned (a permanent Ch1 object);
    // ChapterQuestItems(Ch1) is empty.
    //
    // Ch2 = the 3 散落筆記, but those are DEFERRED: they must not appear
    // until the player wakes the 學霸 and he asks for them
    // (MaybeSpawnChapter2Notes, gated on Flag_Bookworm). So skip Ch2 here
    // — at chapter entry no note exists.
    //
    // A1: Ch1 = the 苦主's transparent umbrella, now ALSO DEFERRED: it must
    // not appear until the player has confronted the 西裝學長 and committed a
    // choice (Flag_SuitSeniorChoiceMade) — the 學長 reveals where he dropped
    // it (MaybeSpawnChapter1VictimUmbrella). So skip Ch1 here too — at chapter
    // entry the umbrella does NOT exist in the world, so the player cannot
    // grab it before the 學長 step (hard-gating the 苦主 → 學長 → 傘 → 苦主
    // spine). Every other state's items (none today) still spawn at entry
    // through this shared helper.
    if (state != SemesterState::Chapter2_Midterms &&
        state != SemesterState::Chapter1_AddDrop)
        SpawnChapterQuestItems(state);

    // Ch4 期末考終焉: the TrueUmbrella re-found behind the 體育館
    // (chapter4.md L6 傘再度失蹤). Claiming it does NOT clear Ch4 — it only
    // satisfies Ending A's 持-TrueUmbrella condition via Flag_HasTrueUmbrella
    // (no EventWiring Ch4 sibling-if). Coord (1640,375) is the walkable
    // 體育館後台 pocket INSIDE the gym footprint — INTENTIONALLY hidden
    // behind the gym building (an easter-egg alternate route to Ending A,
    // parallel to the gentle 助教 finale of T4). KEEP it hidden — T5 only
    // moves the Ch3 umbrella, never this one. Roster-tracked, swept on state
    // change. Spawned at chapter entry (ungated).
    if (state == SemesterState::Chapter4_Finals) {
        auto umb = GameObjectFactory::Create(
            ObjectType::TrueUmbrella, nccu::gfx::Vec2{1640.0f, 375.0f});
        chapterRoster_.push_back(umb.get());
        objects_.push_back(std::move(umb));

        // B2: the DLC easter-egg "?" sign at the 風雩走廊 (corridor rect
        // {1242,8,154,108}, centre ≈(1319,62)). Placed at its south edge so
        // a player exploring up from the 操場/plaza can walk to it and read
        // the teaser. Re-readable, no gameplay effect (no flag/karma/money).
        // Roster-tracked, so it is swept the moment Ch4 ends like every other
        // chapter object. Present only for the open-explore Chapter4_Finals.
        auto dlc = std::make_unique<DlcSign>(nccu::gfx::Vec2{1305.0f, 88.0f});
        chapterRoster_.push_back(dlc.get());
        objects_.push_back(std::move(dlc));
    }
    // Ch3 校慶運動會: the TrueUmbrella the 啦啦隊 took. T5 — it is now
    // DEFERRED (MaybeSpawnChapter3Umbrella), spawned only AFTER the C-系
    // 學姊 reveals its location (Flag_KnowsUmbrellaLoc), and at a coord
    // LEFT of the gym so it is no longer occluded by the 體育館 building
    // (the old (1640,375) sat inside the gym footprint — visible only once
    // you walked behind it; players reported "傘沒出現"). Claiming it is
    // the Ch3 clear (Ch1-isomorphic: beClaimed → UmbrellaClaimed → the
    // EventWiring Ch3 sibling-if → Interlude returnTo Ch4). So it is NOT
    // spawned here at entry — see MaybeSpawnChapter3Umbrella below.

    // 操場 校慶 crowd (player request): 5 students RUN the track at distinct
    // speeds + 10 IDLE/mill, each a DIFFERENT shipped sprite (no new art).
    // All decorative — isQuestGiver=false, non-blocking (NPC::BlocksMovement
    // is false for circular_/wander_) — so the crowd never gates or walls
    // the player. Sprites load only under loadSprites_ (headless tests skip
    // the GPU upload, as the archetype/ambient loops do). This Ch3 path runs
    // AFTER the ctor's terrainMask_ load (it is a state-change spawn, never
    // the initial ctor call), so the idlers get a valid wander mask.
    if (state == SemesterState::Chapter3_SportsDay) {
        static const char* const kCrowd[15] = {
            "resources/assets/sprites/school_uniform_3/male_01.png",
            "resources/assets/sprites/school_uniform_3/male_04.png",
            "resources/assets/sprites/school_uniform_3/male_05.png",
            "resources/assets/sprites/school_uniform_3/male_07.png",
            "resources/assets/sprites/school_uniform_3/male_08.png",
            "resources/assets/sprites/school_uniform_3/male_09.png",
            "resources/assets/sprites/school_uniform_3/male_11.png",
            "resources/assets/sprites/school_uniform_3/female_02.png",
            "resources/assets/sprites/school_uniform_3/female_04.png",
            "resources/assets/sprites/school_uniform_3/female_05.png",
            "resources/assets/sprites/school_uniform_3/female_06.png",
            "resources/assets/sprites/school_uniform_3/female_07.png",
            "resources/assets/sprites/school_uniform_3/female_10.png",
            "resources/assets/sprites/school_uniform_3/female_11.png",
            "resources/assets/sprites/school_uniform_3/female_12.png",
        };
        const nccu::gfx::Vec2 trackC{kSportsTrackCx, kSportsTrackCy};
        const float trackR = 150.0f;   // runners circle ~on the stadium track
        for (int i = 0; i < 5; ++i) {                 // runners
            const float a0 = static_cast<float>(i) * 1.25664f;   // 72° apart
            auto run = std::make_unique<NPC>(
                nccu::gfx::Vec2{trackC.x + trackR * std::cos(a0),
                                trackC.y + trackR * std::sin(a0)},
                std::vector<std::string>{}, false, std::string_view{});
            run->EnableCircularRun(trackC, trackR,
                                   0.30f + 0.08f * static_cast<float>(i), a0);
            if (loadSprites_) run->LoadSprite(kCrowd[i]);
            chapterRoster_.push_back(run.get());
            objects_.push_back(std::move(run));
        }
        static const nccu::gfx::Vec2 kIdle[10] = {
            {1500.0f, 640.0f}, {1620.0f, 600.0f}, {1760.0f, 640.0f},
            {1860.0f, 700.0f}, {1560.0f, 800.0f}, {1700.0f, 820.0f},
            {1820.0f, 800.0f}, {1480.0f, 720.0f}, {1640.0f, 700.0f},
            {1900.0f, 660.0f}};
        unsigned seed = 0x5A17C0DEu;
        for (int i = 0; i < 10; ++i) {                // idlers
            auto npc = std::make_unique<NPC>(
                kIdle[i], std::vector<std::string>{}, false,
                std::string_view{});
            npc->EnableWander(40.0f, seed);
            npc->SetWanderMask(terrainMask_);
            if (loadSprites_) npc->LoadSprite(kCrowd[5 + i]);
            chapterRoster_.push_back(npc.get());
            objects_.push_back(std::move(npc));
            seed = seed * 1664525u + 1013904223u;
        }
    }

    // G-1 + G-2: the Ch1 加退選搶課 crowd + stationary flavor NPCs (player
    // request — make the course-grab rush feel populated). Mirrors the Ch3
    // 操場 crowd block above: roster-tracked (swept on chapter exit), sprites
    // gated by loadSprites_ (headless tests skip the GPU upload), and this Ch1
    // path runs only on a state-change/ctor respawn so the wanderers get a
    // valid terrain mask. ALL are decorative-or-flavor — never quest-givers,
    // never spine-touching:
    //   • Crowd (Chapter1CrowdSpawns): wander=true → non-blocking ambient
    //     pedestrians (random-walk + animate), empty npcId → no dialog, no
    //     `!`. Distinct per-NPC PRNG seed so they don't march in lock-step.
    //   • Flavor (Chapter1FlavorSpawns): wander=false → solid standing
    //     students with a flavor npcId. Their chapter1.md (a) line pool is
    //     loaded into dialogLines_ here via NPC::LoadDialog; GameController
    //     routes a flavor npcId to NPC::Interact() (deterministic per-talk
    //     cycle), NEVER to a spine hook — so they set no quest flag and the
    //     hard-gated 苦主→學長→苦主 spine is untouched. isQuestGiver=false →
    //     no `!` (Ch1IndicatorVisible's fall-through returns the false bit).
    if (state == SemesterState::Chapter1_AddDrop) {
        unsigned seed = 0xC0FFEE11u;
        for (const auto& s : Chapter1CrowdSpawns()) {
            auto npc = std::make_unique<NPC>(s.pos, std::vector<std::string>{},
                                             s.isQuestGiver, s.npcId);
            npc->EnableWander(45.0f, seed);
            npc->SetWanderMask(terrainMask_);
            if (loadSprites_)
                npc->LoadSprite(PickNpcSprite(s.npcId, s.pos, s.spritePath));
            chapterRoster_.push_back(npc.get());
            objects_.push_back(std::move(npc));
            seed = seed * 1664525u + 1013904223u;
        }
        for (const auto& s : Chapter1FlavorSpawns()) {
            auto npc = std::make_unique<NPC>(s.pos, std::vector<std::string>{},
                                             s.isQuestGiver, s.npcId);
            // Load the flavor line pool from chapter1.md (the NPC's (a)
            // section). NPC::Interact() then cycles these one per talk. A
            // headless context with no readable content yields an empty pool
            // (LoadDialog is no-throw) — Interact() simply no-ops, no crash.
            npc->LoadDialog(s.npcId, SemesterState::Chapter1_AddDrop, 0);
            if (loadSprites_)
                npc->LoadSprite(PickNpcSprite(s.npcId, s.pos, s.spritePath));
            chapterRoster_.push_back(npc.get());
            objects_.push_back(std::move(npc));
        }
    }
}

void World::SpawnChapterQuestItems(nccu::SemesterState state) {
    for (const auto& qi : ChapterQuestItems(state)) {
        auto item = std::make_unique<QuestFlagPickup>(
            qi.pos, qi.flag, qi.message, qi.completionFlags,
            qi.completionKarma, qi.countMessages);
        chapterRoster_.push_back(item.get());
        objects_.push_back(std::move(item));
    }
}

bool World::MaybeSpawnChapter2Notes() {
    // Self-gating, sibling of UpdateSportsLap. The 3 散落筆記 appear ONLY
    // after the 學霸 is woken (Flag_Bookworm, set by TryRescueBookworm's
    // wake step). One-shot per Ch2 visit (ch2NotesSpawned_), so a re-talk
    // / re-frame never double-spawns.
    if (ch2NotesSpawned_) return false;
    if (semester_.Current() != SemesterState::Chapter2_Midterms) return false;
    if (!player_ || !player_->HasFlag(kFlagBookworm)) return false;
    SpawnChapterQuestItems(SemesterState::Chapter2_Midterms);
    ch2NotesSpawned_ = true;
    return true;
}

bool World::MaybeSpawnChapter1VictimUmbrella() {
    // A1: the Ch1 苦主's transparent umbrella is REVEAL-AFTER-CHOICE, the
    // sibling of MaybeSpawnChapter2Notes / MaybeSpawnChapter3Umbrella. It
    // appears ONLY after the player has committed a choice with the 西裝學長
    // (Flag_SuitSeniorChoiceMade, set by GameController on a confirmed
    // suit_senior choice) — so it does not exist before the 學長 step. The
    // placement (coord/flag/message) stays single-sourced in
    // ChapterQuestItems(Chapter1) via the shared SpawnChapterQuestItems
    // helper. One-shot per Ch1 visit (ch1VictimUmbrellaSpawned_). Roster-
    // tracked, so it is swept if the player leaves Ch1 uncleared. Carrying it
    // back to the 苦主 is the Ch1 clear (TryReturnVictimUmbrella's grant); the
    // pickup itself only sets Flag_HasVictimUmbrella.
    if (ch1VictimUmbrellaSpawned_) return false;
    if (semester_.Current() != SemesterState::Chapter1_AddDrop) return false;
    if (!player_ || !player_->HasFlag(kFlagSuitSeniorChoiceMade))
        return false;
    SpawnChapterQuestItems(SemesterState::Chapter1_AddDrop);
    ch1VictimUmbrellaSpawned_ = true;
    return true;
}

bool World::MaybeSpawnChapter3Umbrella() {
    // T5: the Ch3 TrueUmbrella is REVEAL-AFTER-CLUE, the sibling of
    // MaybeSpawnChapter2Notes. It appears ONLY after the C-系 學姊 reveals
    // its location (Flag_KnowsUmbrellaLoc, set by TryAdvanceCh3Trade's final
    // link) — so it does not exist before the clue is earned. One-shot per
    // Ch3 visit (ch3UmbrellaSpawned_). Spawned at kChapter3UmbrellaPos —
    // LEFT of the 體育館 (gym left edge x=1493), in the open gap between
    // 風雩樓 and the gym, so it is visible/reachable instead of occluded
    // inside the gym footprint as the old (1640,375) was. Roster-tracked,
    // so it is swept if the player leaves Ch3 uncleared. Claiming it is the
    // Ch3 clear (beClaimed → UmbrellaClaimed → EventWiring → Interlude).
    if (ch3UmbrellaSpawned_) return false;
    if (semester_.Current() != SemesterState::Chapter3_SportsDay) return false;
    if (!player_ || !player_->HasFlag(kFlagKnowsUmbrellaLoc)) return false;
    auto umb = GameObjectFactory::Create(
        ObjectType::TrueUmbrella, kChapter3UmbrellaPos);
    chapterRoster_.push_back(umb.get());
    objects_.push_back(std::move(umb));
    ch3UmbrellaSpawned_ = true;
    return true;
}

bool World::MaybeSpawnInterludeLibrarianReturn() {
    // G-3: the Ch2→Ch3 Interlude 管理員的傘 return-point, the sibling of
    // MaybeSpawnChapter2Notes / MaybeSpawnChapter3Umbrella. A small marker NPC
    // (kNpcLibrarianReturn) appears at the 中正圖書館 front (just south of the
    // library rect {698,254,382,255}) ONLY when ALL hold:
    //   • the FSM is in the Interlude, and
    //   • this market returns to Ch3 (InterludeReturnTo == Chapter3_SportsDay)
    //     — the only market where the Ch2 loaner can still be in hand, and
    //   • the player STILL holds the loaner (Flag_LibrarianUmbrella +
    //     HeldUmbrella::Loaner), and
    //   • the loaner has not already been returned (Flag_…Returned).
    // One-shot per Interlude visit (interludeReturnSpawned_). Roster-tracked,
    // so it is swept on the next state change. If the player never returns it,
    // the loaner still auto-clears on Ch3 entry (SceneRouter) with no karma —
    // returning it is the purely-positive optional 責任感 +10 path.
    if (interludeReturnSpawned_) return false;
    if (semester_.Current() != SemesterState::Interlude_Market) return false;
    if (semester_.InterludeReturnTo() != SemesterState::Chapter3_SportsDay)
        return false;
    if (!player_) return false;
    if (!player_->HasFlag(kFlagLibrarianUmbrella)) return false;
    if (player_->HeldUmbrellaKind() != HeldUmbrella::Loaner) return false;
    if (player_->HasFlag(kFlagLibrarianUmbrellaReturned)) return false;

    // (820,560): mask-verified STRICTLY walkable (100%, all 4 neighbours
    // 100%) and flood-reachable from the Interlude entry (500,1500), just
    // south of the 中正圖書館 rect bottom (y=509). It is the librarian's own
    // Ch1/Ch2 desk apron, so it reads as "return it to her counter".
    auto npc = std::make_unique<NPC>(
        nccu::gfx::Vec2{820.0f, 560.0f}, std::vector<std::string>{},
        /*isQuestGiver=*/false, std::string_view{kNpcLibrarianReturn});
    if (loadSprites_)
        npc->LoadSprite("resources/assets/sprites/school_uniform_3/female_01.png");
    chapterRoster_.push_back(npc.get());
    objects_.push_back(std::move(npc));
    interludeReturnSpawned_ = true;
    return true;
}

void World::UpdateSportsLap() noexcept {
    if (semester_.Current() != SemesterState::Chapter3_SportsDay) return;
    if (!player_ || player_->HasFlag(kFlagSportsLapDone)) return;
    const float dx = player_->GetPosition().x - kSportsTrackCx;
    const float dy = player_->GetPosition().y - kSportsTrackCy;
    const float dist = std::hypot(dx, dy);
    // Only sweep while on/near the stadium track band — loitering the
    // centre or wandering far off the field does not count toward the lap.
    if (dist < 90.0f || dist > 320.0f) return;
    const float ang = std::atan2(dy, dx);
    if (!lapStarted_) {                       // first on-band frame: anchor
        lapStarted_   = true;
        lapPrevAngle_ = ang;
        lapSwept_     = 0.0f;
        return;
    }
    constexpr float kPi = 3.14159265358979323846f;
    float d = ang - lapPrevAngle_;            // shortest signed step
    while (d >  kPi) d -= 2.0f * kPi;
    while (d < -kPi) d += 2.0f * kPi;
    lapSwept_    += d;
    lapPrevAngle_ = ang;
    if (std::fabs(lapSwept_) >= 2.0f * kPi * 0.92f)   // ~one lap (8% slack)
        player_->SetFlag(kFlagSportsLapDone);
}

float World::SportsLapProgress() const noexcept {
    if (player_ && player_->HasFlag(kFlagSportsLapDone)) return 1.0f;
    constexpr float kTwoPi = 6.28318530717958647692f;
    const float f = std::fabs(lapSwept_) / kTwoPi;
    return f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f);
}

bool World::SportsLapActive() const noexcept {
    return semester_.Current() == SemesterState::Chapter3_SportsDay
        && player_ != nullptr && !player_->HasFlag(kFlagSportsLapDone);
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
