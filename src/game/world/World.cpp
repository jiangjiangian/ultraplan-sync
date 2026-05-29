#include "game/world/World.h"
#include "game/entities/CashPickup.h"
#include "game/entities/DlcSign.h"
#include "game/quest/ChapterPickups.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/quest/ChapterSpawns.h"
#include "game/quest/ChapterVendors.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/NPC.h"
#include "game/quest/NpcSpawns.h"
#include "game/quest/PipoyaRoster.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorSprite.h"
#include "engine/math/Vec2.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <string_view>
#include <unordered_set>

/**
 * @file World.cpp
 * @brief World 的建構、章節名冊重生與幀末清除（Sweep）實作：維持 front 即玩家、
 *        延遲刪除、避免迭代器失效等不變式。
 */

namespace nccu {

World::World(const std::string& playerSpritePath, bool loadSprites,
             WorldOptions opts)
    : loadSprites_(loadSprites),
      reducedMotion_(opts.reducedMotion),
      largeTargets_(opts.largeTargets) {
    using nccu::engine::math::Vec2;

    // 無障礙旗標由 opts 注入（環境讀取集中在 ReadWorldOptionsFromEnv，由組裝根呼叫一
    // 次），World 因此對其引數保持純粹；採預設 WorldOptions{} 的測試兩旗標皆 false。

    // 玩家置於正門以東的指南路上，避開所有牆與 NPC 碰撞盒，使 AABB 解算器不必在第 0
    // 幀就要救援。三把象徵道德抉擇的雨傘排在廣場與四維道之間的中央帶。
    //
    // 善有善報設計：世界中不再放置可撿的真傘——玩家的真傘改由苦主在玩家把他的傘送回
    // 後授予（TryReturnVictimUmbrella）。詛咒／易碎／教授陷阱三把傘保留為道德／結局 B
    // 等路線，仍透過各自的 BeClaimed 清關第一章（三結局架構不變）。
    objects_.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{500, 1860}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{ 750, 1280}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{1200, 1256}));
    objects_.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{1560, 1280}));

    // 第一章跑腿道具：被風吹走的加退選申請書，落在四維堂南側空地。撿起後設立
    // Flag_FoundForm，助教給出獎勵對白（集英樓二樓線索）。
    objects_.push_back(std::make_unique<QuestFlagPickup>(
        nccu::engine::math::Vec2{560.0f, 1725.0f}, kFlagFoundForm));

    // 在生成章節 NPC 之前先快取玩家，使「front 即玩家」的不變式一開始就建立且不被打
    // 亂：SpawnChapterNpcs 只在尾端附加。此處的 static_cast 建立在已記錄的不變式上
    // （front() 即上方最先 push 的玩家，RespawnChapterRoster 會以 assert 確認它仍為元
    // 素 0），符合慣例對 static_cast 的允許。
    player_ = static_cast<Player*>(objects_.front().get());
    if (player_) player_->LoadSprite(playerSpritePath);

    // 第一章透過狀態機之後也會驅動的同一條狀態感知路徑生成，使初始名冊與後續一致。
    RespawnChapterRoster(semester_.Current());

    // 靜態碰撞現為像素精確的可走遮罩：工具把建築牆基與河流烘焙進 collision_mask_base
    // .png，美術再把樹木／花圃／外牆畫到 collision_mask.png 之上。buildings::kAll 的
    // sprite 矩形僅作為觸發區——BuildingTracker 以它為章節事件的鍵。
    terrainMask_ = LoadTerrainMask();

    // 環境路人——在遮罩載入「之後」才接上，使自我解算的漫遊不會走進建築與河流。各給一
    // 個不同的亂數種子，避免整群人同步移動。
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
