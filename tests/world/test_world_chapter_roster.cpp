#include "doctest/doctest.h"
#include "game/quest/ChapterSpawns.h"
#include "engine/core/GameObject.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include <cmath>
#include <set>
#include <string>
#include <string_view>

/**
 * @file test_world_chapter_roster.cpp
 * @brief 驗證章節 roster 換班的不變量：World ctor 會把 Ch1 NPC 登記進受追蹤的 roster，
 *        因此任何「離開某章節」的轉場都會把前一章節的任務給予者從物件清單剝除，而 Player、
 *        雨傘與環境學生則完整保留。並涵蓋走完整條章節主線時無 NPC 外洩，以及 Ch3 的特定擺放。
 */

using nccu::SemesterState;
using nccu::World;

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

std::size_t QuestGiverCount(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (const auto* npc = dynamic_cast<const NPC*>(o.get()))
            if (npc->IsQuestGiver()) ++n;
    return n;
}

std::set<std::string> ChapterNpcIdSet(SemesterState s) {
    std::set<std::string> ids;
    for (const auto& sp : nccu::ChapterNpcSpawns(s)) ids.insert(sp.npcId);
    return ids;
}

} // namespace

// World ctor 把 Ch1 NPC 登記進章節 roster，因此它們會在換班時被掃除。
TEST_CASE("World ctor 把 Ch1 NPC 登記進 chapterRoster_ 以便換班時掃除") {
    World w("", /*loadSprites=*/false);

    // 前置條件：ctor 後 5 個 Ch1 原型都在。
    REQUIRE(HasNpcId(w, "victim"));
    REQUIRE(HasNpcId(w, "suit_senior"));
    REQUIRE(HasNpcId(w, "bookworm"));
    REQUIRE(HasNpcId(w, "ta"));
    REQUIRE(HasNpcId(w, "shop_auntie"));

    // 走到 Interlude。該處的生成為空；5 個 Ch1 NPC「必須」消失，否則它們未被 roster 追蹤，
    // 下一章節便會繼承到它們。
    w.RespawnChapterRoster(SemesterState::Interlude_Market);

    CHECK_FALSE(HasNpcId(w, "victim"));
    CHECK_FALSE(HasNpcId(w, "suit_senior"));
    CHECK_FALSE(HasNpcId(w, "bookworm"));
    CHECK_FALSE(HasNpcId(w, "ta"));
    CHECK_FALSE(HasNpcId(w, "shop_auntie"));

    // 更強的要求：換班後物件清單的 NPC id 應與 ChapterNpcSpawns(state) 完全相符（環境路人
    // 不帶 npcId，故不計）。Interlude 的章節生成為空，故換班後預期 0 個任務給予者、0 個章節
    // NPC。
    CHECK(QuestGiverCount(w) == 0);
}

// World ctor：每個 Ch1 NPC 都是章節 roster 的成員。
TEST_CASE("World ctor：每個 Ch1 NPC 都是 chapterRoster_ 的成員") {
    World w("", /*loadSprites=*/false);

    // 以行為計數，而非直接存取 private 的 chapterRoster_。若有任一 Ch1 NPC 在 ctor 被推入
    // 卻未登記，此斷言便會失敗。
    const std::size_t questGiversCh1 = QuestGiverCount(w);
    REQUIRE(questGiversCh1 >= 1);   // 至少有苦主

    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(QuestGiverCount(w) == 0);
}

// 走完整條章節主線時 roster 換班不會讓任何 NPC 外洩。
TEST_CASE("走完整條章節主線時 roster 換班不會讓任何 NPC 外洩") {
    World w("", /*loadSprites=*/false);

    // 走完 Ch1 -> Interlude -> Ch2 -> Interlude -> Ch3 -> Interlude -> Ch4 -> Ending。
    // 每一步後，物件清單中唯一的章節 NPC 只能是目的狀態 roster 所宣告的那些。
    const SemesterState path[] = {
        SemesterState::Interlude_Market,
        SemesterState::Chapter2_Midterms,
        SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay,
        SemesterState::Interlude_Market,
        SemesterState::Chapter4_Finals,
        SemesterState::Ending_A,
    };

    for (SemesterState s : path) {
        w.RespawnChapterRoster(s);

        const auto expected = ChapterNpcIdSet(s);
        for (const auto& o : w.Objects()) {
            if (o->IsVendor()) continue;       // 攤販來自 ChapterVendors（非 NPC 名冊）
            const std::string id{o->NpcId()};
            if (id.empty()) continue;          // 環境學生 / Player
            CHECK(expected.count(id) == 1);    // 只屬於當前章節
        }
        for (const auto& id : expected)
            CHECK(HasNpcId(w, id.c_str()));
    }
}

// 走完整條主線後 Player 不變量仍成立（同一物件、仍在清單最前端）。
TEST_CASE("走完整條主線後 Player 不變量仍成立") {
    World w("", /*loadSprites=*/false);
    Player*     p0 = w.GetPlayer();
    GameObject* f0 = w.Objects().front().get();
    REQUIRE(p0 != nullptr);
    REQUIRE(static_cast<GameObject*>(p0) == f0);

    for (SemesterState s : {SemesterState::Interlude_Market,
                            SemesterState::Chapter2_Midterms,
                            SemesterState::Chapter3_SportsDay,
                            SemesterState::Chapter4_Finals,
                            SemesterState::Ending_B,
                            SemesterState::Ending_D,
                            SemesterState::Ending_C,
                            SemesterState::Chapter1_AddDrop}) {
        w.RespawnChapterRoster(s);
        CHECK(w.GetPlayer() == p0);
        CHECK(w.Objects().front().get() == f0);
    }
}

// Ch3 物物交換鏈的三個 NPC 散落在羅馬廣場。
TEST_CASE("Ch3 物物交換鏈的三個 NPC 散落在羅馬廣場") {
    // 3 個物物交換鏈任務給予者（vendor_sausage_a / loudspeaker_b / senior_c）由舊的南側走廊
    // 移到羅馬廣場（玩家跑完操場一圈後前往之處）。把它們釘在可行走的廣場圓盤內（中心約
    // 1088,960，半徑約 200），使未來的調整不會把它們又散落到南側校園各處。
    World w("", /*loadSprites=*/false);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);

    for (const char* id : {"vendor_sausage_a", "loudspeaker_b", "senior_c"}) {
        const GameObject* hit = nullptr;
        for (const auto& o : w.Objects())
            if (o->NpcId() == std::string_view(id)) { hit = o.get(); break; }
        REQUIRE_MESSAGE(hit != nullptr,
                        "Ch3 trade-chain NPC missing: " << id);
        const auto p = hit->GetPosition();
        const float d = std::hypot(p.x - 1088.0f, p.y - 960.0f);
        CHECK_MESSAGE(d < 220.0f,
                      id << " at (" << p.x << "," << p.y
                         << ") is " << d << " px from 羅馬廣場 centre");
    }
}

// Ch3：操場的校慶人群會生成（5 名跑者 + 10 名閒置者）。
TEST_CASE("Ch3：操場的校慶人群會生成（5 名跑者 + 10 名閒置者）") {
    // 裝飾性人群填滿操場（x1384-2005, y541-940），讓運動會真的有人。在生成時計數：雨傘
    //（y375）/ 原型 / ABC（廣場）/ 環境學生（南側）都落在操場框外，故此處計數的是人群。
    World w("", /*loadSprites=*/false);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);

    int crowd = 0;
    for (const auto& o : w.Objects()) {
        const auto p = o->GetPosition();
        if (p.x >= 1384.0f && p.x <= 2005.0f && p.y >= 541.0f && p.y <= 940.0f)
            ++crowd;
    }
    CHECK(crowd >= 15);
}
