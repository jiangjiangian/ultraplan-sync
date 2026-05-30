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
        chapterRoster_.push_back(npc.get());   // 須在 move 之前記錄裸指標
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

    // CashPickups：循環經濟中靠探索賺取的部分。與 NPC／攤販一樣記入 chapterRoster_，
    // 故章節結束前未收集的硬幣會隨名冊一起清掃（每次章節造訪一次性；已入帳的金錢存放在
    // Player 上）。第一章今日已有具體的散布；其他狀態的表格日後填入。
    for (const auto& pp : ChapterPickups(state)) {
        auto coin = std::make_unique<CashPickup>(pp.pos, pp.value);
        chapterRoster_.push_back(coin.get());
        objects_.push_back(std::move(coin));
    }

    // 任務物品：章節限定的 QuestFlagPickup。與硬幣一樣記入名冊，故未拾取的物品會在下次
    // 狀態切換時被清掃，而非洩漏進插曲段／下一章。第一章的申請書「不」在此——它仍由建構式
    // 生成（一個永久的第一章物件）；ChapterQuestItems(Ch1) 為空。
    //
    // 第二章 = 3 份散落筆記，但它們是「延後」生成的：必須等到玩家喚醒學霸、他開口要這些
    // 筆記後才出現（MaybeSpawnChapter2Notes，以 Flag_Bookworm 為閘）。故此處略過第二章
    // ——章節進入時尚無任何筆記。
    //
    // 第一章 = 苦主的透明傘，現在「也」延後生成：必須等到玩家面對西裝學長並確認某個選項
    //（Flag_SuitSeniorChoiceMade）後才出現——學長會透露他把傘掉在哪
    //（MaybeSpawnChapter1VictimUmbrella）。故此處也略過第一章——章節進入時這把傘「不」
    // 存在於世界中，使玩家無法在學長那一步之前就拿走它（硬閘控 苦主 → 學長 → 傘 → 苦主
    // 主線）。其餘所有狀態的物品（今日無）仍會在進入時經此共用輔助函式生成。
    if (state != SemesterState::Chapter2_Midterms &&
        state != SemesterState::Chapter1_AddDrop)
        SpawnChapterQuestItems(state);

    // Ch4 期末考終焉：在體育館後方重新尋獲的 TrueUmbrella（劇情中傘再度失蹤）。拾取它
    // 「不」清第四章關——它只經 Flag_HasTrueUmbrella 滿足結局 A 的「持有 TrueUmbrella」
    // 條件（無事件接線的第四章兄弟 if）。座標 (1640,375) 是位於體育館足跡「內」、可行走的
    // 後台口袋——刻意藏在體育館建築後方（通往結局 A 的彩蛋替代路線，與助教溫柔終局並行）。
    // 維持其隱藏——只有第三章的傘會被移動，這把絕不移動。記入名冊，狀態切換時清掃。在章節
    // 進入時生成（無閘控）。
    if (state == SemesterState::Chapter4_Finals) {
        auto umb = GameObjectFactory::Create(
            ObjectType::TrueUmbrella, nccu::engine::math::Vec2{1640.0f, 375.0f});
        chapterRoster_.push_back(umb.get());
        objects_.push_back(std::move(umb));

        // 位於風雩走廊（走廊矩形 {1242,8,154,108}，中心約 (1319,62)）的 DLC 彩蛋「?」
        // 告示牌。放在其南緣，使從操場／廣場往上探索的玩家能走到它並讀到預告。可重複閱讀、
        // 無玩法效果（不設旗標／業力／金錢）。記入名冊，故第四章一結束便與其他章節物件一樣
        // 被清掃。僅在可自由探索的 Chapter4_Finals 出現。
        auto dlc = std::make_unique<DlcSign>(nccu::engine::math::Vec2{1305.0f, 88.0f});
        chapterRoster_.push_back(dlc.get());
        objects_.push_back(std::move(dlc));
    }
    // Ch3 校慶運動會：被啦啦隊拿走的 TrueUmbrella。它現在是「延後」生成
    //（MaybeSpawnChapter3Umbrella），只在 C 系學姊透露其位置（Flag_KnowsUmbrellaLoc）
    // 後才生成，且座標在體育館「左側」，使其不再被體育館建築遮住（舊的 (1640,375) 落在
    // 體育館足跡內——須走到後方才看得到；玩家曾回報「傘沒出現」）。拾取它即第三章通關
    //（與第一章同構：BeClaimed → UmbrellaClaimed → 事件接線的第三章兄弟 if → 插曲段
    // returnTo 第四章）。故此處在進入時「不」生成——見下方 MaybeSpawnChapter3Umbrella。

    // 操場校慶人群（玩家需求）：5 名學生在跑道上以不同速度「奔跑」 + 10 名閒置／走動，
    // 每個都用不同的隨附 sprite（不新增美術）。全部為裝飾——isQuestGiver=false、不阻擋
    //（circular_/wander_ 的 NPC::BlocksMovement 為 false）——故人群絕不會閘控或擋住玩家。
    // sprite 僅在 loadSprites_ 下載入（headless 測試略過 GPU 上傳，與原型／環境迴圈同理）。
    // 此第三章路徑在建構式載入 terrainMask_ 「之後」執行（它是狀態切換生成，絕非最初的
    // 建構式呼叫），故閒置者能取得有效的遊蕩遮罩。
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
        const float trackR = 150.0f;   // 跑者大致沿體育場跑道繞圈
        for (int i = 0; i < 5; ++i) {                 // 跑者
            const float a0 = static_cast<float>(i) * 1.25664f;   // 相隔 72°
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
        for (int i = 0; i < 10; ++i) {                // 閒置者
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

    // 第一章加退選搶課人群 + 站立的風味 NPC（玩家需求——讓搶課的混亂感覺有人氣）。比照
    // 上方第三章操場人群區塊：記入名冊（章節離開時清掃）、sprite 以 loadSprites_ 閘控
    //（headless 測試略過 GPU 上傳），且此第一章路徑只在狀態切換／建構式重生時執行，故遊蕩者
    // 能取得有效的地形遮罩。全部為裝飾或風味——絕非任務給予者、絕不碰主線：
    //   • 人群（Chapter1CrowdSpawns）：wander=true → 不阻擋的環境行人（隨機走動 + 動畫），
    //     npcId 為空 → 無對話、無「!」。每個 NPC 各有不同的 PRNG 種子，故不會步伐一致地齊走。
    //   • 風味（Chapter1FlavorSpawns）：wander=false → 實心站立的學生、帶風味 npcId。其章節
    //     內容的 (a) 台詞池在此經 NPC::LoadDialog 載入 dialogLines_；GameController 會把風味
    //     npcId 路由到 NPC::Interact()（每次對話依序循環），「絕不」路由到主線鉤子——故它們
    //     不設任何任務旗標，硬閘控的 苦主→學長→苦主 主線不受影響。isQuestGiver=false → 無
    //    「!」（Ch1IndicatorVisible 的落空回傳 false 位元）。
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
            // 從章節內容載入風味台詞池（該 NPC 的 (a) 段）。NPC::Interact() 之後每次對話
            // 依序循環取用。無可讀內容的 headless 環境會得到空池（LoadDialog 不拋例外）——
            // Interact() 單純無動作、不會當機。
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
    // 自我閘控，為 UpdateSportsLap 的兄弟。3 份散落筆記「只」在學霸被喚醒後出現
    //（Flag_Bookworm，由 TryRescueBookworm 的喚醒步驟設下）。每次第二章造訪一次性
    //（ch2NotesSpawned_），故重新對話／重新進場絕不會重複生成。
    if (ch2NotesSpawned_) return false;
    if (semester_.Current() != SemesterState::Chapter2_Midterms) return false;
    if (!player_ || !player_->HasFlag(kFlagBookworm)) return false;
    SpawnChapterQuestItems(SemesterState::Chapter2_Midterms);
    ch2NotesSpawned_ = true;
    return true;
}

bool World::MaybeSpawnChapter1VictimUmbrella() {
    // 第一章苦主的透明傘是「選擇後才揭示」，為 MaybeSpawnChapter2Notes /
    // MaybeSpawnChapter3Umbrella 的兄弟。它「只」在玩家對西裝學長確認某個選項後出現
    //（Flag_SuitSeniorChoiceMade，suit_senior 選項確認時由 GameController 設下）——故它
    // 在學長那一步之前不存在。其擺放（座標／旗標／訊息）仍單一來源於
    // ChapterQuestItems(Chapter1)，經共用的 SpawnChapterQuestItems 輔助函式。每次第一章
    // 造訪一次性（ch1VictimUmbrellaSpawned_）。記入名冊，故玩家未通關就離開第一章時會被
    // 清掃。把它帶回給苦主才是第一章通關（TryReturnVictimUmbrella 的授予）；拾取物本身只
    // 設下 Flag_HasVictimUmbrella。
    if (ch1VictimUmbrellaSpawned_) return false;
    if (semester_.Current() != SemesterState::Chapter1_AddDrop) return false;
    if (!player_ || !player_->HasFlag(kFlagSuitSeniorChoiceMade))
        return false;
    SpawnChapterQuestItems(SemesterState::Chapter1_AddDrop);
    ch1VictimUmbrellaSpawned_ = true;
    return true;
}

bool World::MaybeSpawnChapter3Umbrella() {
    // 第三章的 TrueUmbrella 是「線索後才揭示」，為 MaybeSpawnChapter2Notes 的兄弟。它
    //「只」在 C 系學姊透露其位置後出現（Flag_KnowsUmbrellaLoc，由 TryAdvanceCh3Trade 的
    // 最後一個環節設下）——故在取得線索之前不存在。每次第三章造訪一次性
    //（ch3UmbrellaSpawned_）。生成於 kChapter3UmbrellaPos——體育館「左側」（體育館左緣
    // x=1493），落在風雩樓與體育館之間的空隙，故可見且可達，而非像舊的 (1640,375) 那樣被
    // 遮在體育館足跡內。記入名冊，故玩家未通關就離開第三章時會被清掃。拾取它即第三章通關
    //（BeClaimed → UmbrellaClaimed → 事件接線 → 插曲段）。
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
    // 第二章→第三章插曲段「管理員的傘」歸還點，為 MaybeSpawnChapter2Notes /
    // MaybeSpawnChapter3Umbrella 的兄弟。一個小型標記 NPC（kNpcLibrarianReturn）會出現在
    // 中正圖書館前方（就在圖書館矩形 {698,254,382,255} 南側），「只」在以下全部成立時：
    //   • FSM 位於插曲段，且
    //   • 此市集會返回第三章（InterludeReturnTo == Chapter3_SportsDay）——這是唯一仍可能
    //     手持第二章借傘的市集，且
    //   • 玩家「仍」持有借傘（Flag_LibrarianUmbrella + HeldUmbrella::Loaner），且
    //   • 借傘尚未歸還（Flag_…Returned）。
    // 每次插曲段造訪一次性（interludeReturnSpawned_）。記入名冊，故下次狀態切換時清掃。
    // 若玩家始終不歸還，借傘仍會在進入第三章時自動清除（SceneRouter）且無業力——歸還它是
    // 純正向的選用「責任感 +10」路線。
    if (interludeReturnSpawned_) return false;
    if (semester_.Current() != SemesterState::Interlude_Market) return false;
    if (semester_.InterludeReturnTo() != SemesterState::Chapter3_SportsDay)
        return false;
    if (!player_) return false;
    if (!player_->HasFlag(kFlagLibrarianUmbrella)) return false;
    if (player_->HeldUmbrellaKind() != HeldUmbrella::Loaner) return false;
    if (player_->HasFlag(kFlagLibrarianUmbrellaReturned)) return false;

    // (820,560)：經遮罩驗證為「嚴格」可行走（100%，四個鄰格皆 100%），且可由插曲段入口
    //（500,1500）淹沒法到達，就在中正圖書館矩形底部（y=509）南側。它是管理員第一／二章
    // 櫃台前的區域，故讀來像「把傘還回她的櫃台」。
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
