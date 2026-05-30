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
    // 先以追蹤指標組出待刪集合，再「先」清空追蹤容器，使裸指標絕不會在其 unique_ptr
    // 釋放後被解參考——erase 述詞「只」比對指標身分（不解參考）。這趟 remove-erase 在
    // 我們迭代完 chapterRoster_「之後」才跑，絕非迭代途中，也絕不重排或移除元素 0
    //（Player），因為 Player 從未被放進 chapterRoster_。
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

    // 重新武裝第二章延後筆記的一次性旗標：舊的筆記（若有）剛隨名冊一併清掃，故任何章節
    // 的新一次造訪都從「延後生成已重新武裝」起步。MaybeSpawnChapter2Notes 之後只在玩家
    // 於新的第二章造訪中重新喚醒學霸時才觸發。
    ch2NotesSpawned_ = false;
    // 第三章「線索後才揭示」傘的一次性旗標，同樣重新武裝。
    ch3UmbrellaSpawned_ = false;
    // 第一章「選擇後才揭示」苦主傘的一次性旗標，同樣重新武裝，使新一次第一章造訪重新以
    // Flag_SuitSeniorChoiceMade 為閘。
    ch1VictimUmbrellaSpawned_ = false;
    // 插曲段「管理員的傘」歸還點的一次性旗標，同樣重新武裝，使新一次插曲段造訪重新以
    //（ReturnTo==Ch3 且手持借傘）為閘。
    interludeReturnSpawned_ = false;

    SpawnChapterNpcs(state);

    // 不變式守衛：front 必須仍是存活的 Player，且快取指標必須仍指向它。兩者皆由建構過程
    // 保證（我們只在尾端附加，並只刪除從不是元素 0 的被追蹤 NPC）；此處以 assert 確認，
    // 而非盲信。
    if (player_) {
        assert(!objects_.empty() &&
               objects_.front().get() == static_cast<GameObject*>(player_));
    }
}

void World::Sweep() {
    // 在 erase「之前」先把玩家是否死亡快照下來——快取的 player_ 指標會在其所屬
    // unique_ptr 被銷毀的瞬間懸空（heap-use-after-free，依 [basic.stc.dynamic
    // .deallocation]/4）。bool 快照繞過這點：趁物件仍存在時就決定是否要清快取，再
    // erase，最後依快照行動。
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
