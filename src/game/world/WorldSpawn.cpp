#include "game/world/World.h"
#include "game/entities/CashPickup.h"
#include "game/entities/DlcSign.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/quest/ChapterPickups.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/quest/ChapterSpawns.h"
#include "game/quest/ChapterVendors.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Flags.h"
#include "game/quest/NpcSpawns.h"
#include "game/quest/PipoyaRoster.h"
#include "game/controller/GameObjectFactory.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorSprite.h"
#include "engine/math/Vec2.h"
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// SpawnChapterNpcs + 4 個自我閘控的 MaybeSpawn 輔助函式 + SpawnChapterQuestItems
// 從 World.cpp 抽出（847->220 行）。它們是 World 的成員，此處「只」放實作。.h 的
// 宣告與每個成員存取皆未變動——C++ 允許將類別實作依檔案分割，CMake 的 GLOB 也會自動
// 納入此 TU。行為完全不變；所有成員狀態變動（objects_、chapterRoster_、ch*Spawned_）
// 皆與它們所取代的內聯區塊逐位元相同。

namespace nccu {

void World::SpawnChapterNpcs(nccu::SemesterState state) {
    // 第四章「斥責學長後不出場」漣漪。劇情承諾：曾在第一章斥責 suit_senior 的玩家
    //（Flag_ScoldedSenior）在期末不會再見到他——除非日後修補了關係（Flag_HelpedSenior，
    // 第二章的回呼紙條）。在生成時就過濾，使 suit_senior 單純不出現在 objects_ 中，這與
    // 其他每個「此章節沒有的 NPC」情形完全一致（例如圖書館員只在第二章）：對話開啟器無法
    // 以他為目標，Chapter4Quest 的路由也找不到可進一步斥責／幫助的對象，而若他在下次
    // Transition 仍缺席，chapterRoster_ 的清掃已能處理其拆除。以 player_ 為條件，使
    // 無 Player 的 headless World 單元測試保持防禦性，即使建構式在首次重生前即已快取
    // player_。
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

    // 攤販是 NPC 名冊在價目表上的兄弟（其擺放帶的是 VendorConfig，而非 sprite 路徑
    // + npcId，故自成一張表）。同樣記入 chapterRoster_，使下次狀態切換時與 NPC 一樣
    // 被清掃。在尚未填入市集陣容前，ChapterVendors() 對每個狀態皆為空，故今日此迴圈是
    // 空操作——它只證明生成「機制」，行為完全不變。
    // 每個市集攤位都必須是「不同的人」。舊版對「所有」攤位都傳入字面值 "vendor" 加上
    // 單一的 shop_auntie.png 退路，故在乾淨 clone（PIPOYA 資源包缺席 → 走退路）上，
    // 十個攤主會渲染成同一張 sprite 的十個分身。如今每攤位的選圖規則集中在一個純函式
    //（VendorSprite.h 的 VendorSpriteFor）中，並與其回歸測試共用，使「十個不同的人」
    // 之保證透過真正的產線程式碼路徑被驗證。
    std::size_t vendorIdx = 0;
    for (const auto& vp : ChapterVendors(state)) {
        auto vendor = std::make_unique<Vendor>(vp.pos, vp.config);
        // 與上方的章節 NPC 完全一樣，以 loadSprites_ 閘控，使無頭的 World 單元測試
        //（loadSprites=false）略過 GPU 上傳。VendorSpriteFor 以攤位自己唯一的攤主／
        // 名稱為鍵，並依生成索引挑選彼此分明的精選退路，故乾淨 clone 仍會顯示十個不同
        // 的人（而非十個 shop_auntie 分身），PIPOYA 路徑也不再相撞。
        if (loadSprites_) {
            // spriteOverride（自動販賣機的機台美術）會整張當作靜態圖渲染；否則挑選一個
            // 彼此分明的 Pipoya 人物。
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
            ObjectType::TrueUmbrella, nccu::engine::math::Vec2{1640.0f, 375.0f});
        chapterRoster_.push_back(umb.get());
        objects_.push_back(std::move(umb));

        // B2: the DLC easter-egg "?" sign at the 風雩走廊 (corridor rect
        // {1242,8,154,108}, centre ≈(1319,62)). Placed at its south edge so
        // a player exploring up from the 操場/plaza can walk to it and read
        // the teaser. Re-readable, no gameplay effect (no flag/karma/money).
        // Roster-tracked, so it is swept the moment Ch4 ends like every other
        // chapter object. Present only for the open-explore Chapter4_Finals.
        auto dlc = std::make_unique<DlcSign>(nccu::engine::math::Vec2{1305.0f, 88.0f});
        chapterRoster_.push_back(dlc.get());
        objects_.push_back(std::move(dlc));
    }
    // Ch3 校慶運動會: the TrueUmbrella the 啦啦隊 took. T5 — it is now
    // DEFERRED (MaybeSpawnChapter3Umbrella), spawned only AFTER the C-系
    // 學姊 reveals its location (Flag_KnowsUmbrellaLoc), and at a coord
    // LEFT of the gym so it is no longer occluded by the 體育館 building
    // (the old (1640,375) sat inside the gym footprint — visible only once
    // you walked behind it; players reported "傘沒出現"). Claiming it is
    // the Ch3 clear (Ch1-isomorphic: BeClaimed → UmbrellaClaimed → the
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
        const nccu::engine::math::Vec2 trackC{kSportsTrackCx, kSportsTrackCy};
        const float trackR = 150.0f;   // runners circle ~on the stadium track
        for (int i = 0; i < 5; ++i) {                 // runners
            const float a0 = static_cast<float>(i) * 1.25664f;   // 72° apart
            auto run = std::make_unique<NPC>(
                nccu::engine::math::Vec2{trackC.x + trackR * std::cos(a0),
                                trackC.y + trackR * std::sin(a0)},
                std::vector<std::string>{}, false, std::string_view{});
            run->EnableCircularRun(trackC, trackR,
                                   0.30f + 0.08f * static_cast<float>(i), a0);
            if (loadSprites_) run->LoadSprite(kCrowd[i]);
            chapterRoster_.push_back(run.get());
            objects_.push_back(std::move(run));
        }
        static const nccu::engine::math::Vec2 kIdle[10] = {
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
    // Ch3 clear (BeClaimed → UmbrellaClaimed → EventWiring → Interlude).
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
        nccu::engine::math::Vec2{820.0f, 560.0f}, std::vector<std::string>{},
        /*isQuestGiver=*/false, std::string_view{kNpcLibrarianReturn});
    if (loadSprites_)
        npc->LoadSprite("resources/assets/sprites/school_uniform_3/female_01.png");
    chapterRoster_.push_back(npc.get());
    objects_.push_back(std::move(npc));
    interludeReturnSpawned_ = true;
    return true;
}

} // namespace nccu
