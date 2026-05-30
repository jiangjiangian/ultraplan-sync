/**
 * @file test_chapter_questitems.cpp
 * @brief 驗證各章任務物品表（Ch2 三張筆記、Ch1 苦主之傘）與 QuestFlagPickup 的計數式提示、完成獎勵與延後生成。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/quest/Chapter2Quest.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <set>
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::World;

// Ch2 任務物品為 3 張筆記（完成集合 +3）；Ch1 為苦主之傘（單旗標、無完成集合）；其餘狀態皆為空。
TEST_CASE("ChapterQuestItems：Ch2 是 3 張筆記；其餘狀態皆為空") {
    const auto& ch2 = nccu::ChapterQuestItems(SemesterState::Chapter2_Midterms);
    REQUIRE(ch2.size() == 3);

    std::set<std::string> flags;
    for (const auto& q : ch2) {
        flags.insert(q.flag);
        CHECK(q.completionKarma == 3);              // 學霸 (b) +3
        CHECK(q.completionFlags.size() == 3);       // 集滿 3 張才完成
        CHECK_FALSE(q.message.empty());
        CHECK(q.countMessages.size() == 3);         // 依計數給第 1/2/3 則
    }
    CHECK(flags == std::set<std::string>{nccu::kFlagFoundNote1,
                                         nccu::kFlagFoundNote2,
                                         nccu::kFlagFoundNote3});

    // Ch1 恰好帶有苦主之傘這一個撿取物（單旗標、無完成集合、karma 0——
    // +5 在承諾的選擇上，授予本身就是回報）。其餘非 Ch2 的狀態都維持為空。
    const auto& ch1 = nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 1);
    CHECK(ch1[0].flag == nccu::kFlagHasVictimUmbrella);
    CHECK(ch1[0].completionFlags.empty());
    CHECK(ch1[0].completionKarma == 0);
    CHECK_FALSE(ch1[0].message.empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Interlude_Market).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Chapter3_SportsDay).empty());
    CHECK(nccu::ChapterQuestItems(SemesterState::Ending_A).empty());
}

// 計數式提示：第 N 次撿到就顯示第 N 則訊息，與撿取順序無關（修正「先撿到第 3 張卻印出最後一頁」的問題）。
TEST_CASE("QuestFlagPickup：計數式訊息 — 第 N 次撿到 -> 第 N 則，"
          "與撿取順序無關") {
    EventBus::Instance().Clear();
    std::string lastMsg;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&lastMsg](const Event& e) { lastMsg = e.text; });

    const std::vector<std::string> set = {nccu::kFlagFoundNote1,
                                          nccu::kFlagFoundNote2,
                                          nccu::kFlagFoundNote3};
    const std::vector<std::string> msgs = {"first", "second", "third"};
    auto makeNote = [&](const char* flag) {
        return QuestFlagPickup(nccu::engine::math::Vec2{0, 0}, flag, "fallback",
                               set, 3, msgs);
    };

    // 以反向的身分順序撿取（note3、note2、note1）。訊息必須跟著「持有計數」走
    //（第 1 次 -> first、第 2 次 -> second、第 3 次 -> third），而非筆記的身分。
    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    auto n3 = makeNote(nccu::kFlagFoundNote3);
    auto n2 = makeNote(nccu::kFlagFoundNote2);
    auto n1 = makeNote(nccu::kFlagFoundNote1);
    n3.OnPickup(&p);
    CHECK(lastMsg == "first");     // 第 1 次撿到 -> first
    n2.OnPickup(&p);
    CHECK(lastMsg == "second");    // 第 2 次撿到 -> second
    n1.OnPickup(&p);
    CHECK(lastMsg == "third");     // 第 3 次撿到 -> third

    // 換一種順序（先撿 note2）也會得到同樣依計數對應的訊息。
    Player q{nccu::engine::math::Vec2{0.0f, 0.0f}};
    auto m2 = makeNote(nccu::kFlagFoundNote2);
    auto m1 = makeNote(nccu::kFlagFoundNote1);
    m2.OnPickup(&q);
    CHECK(lastMsg == "first");
    m1.OnPickup(&q);
    CHECK(lastMsg == "second");

    EventBus::Instance().Clear();
}

// countMessages 為空時，沿用單一訊息的舊行為不變。
TEST_CASE("QuestFlagPickup：countMessages 為空時沿用單一訊息") {
    EventBus::Instance().Clear();
    std::string lastMsg;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&lastMsg](const Event& e) { lastMsg = e.text; });

    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    QuestFlagPickup form(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundForm, "撿到申請書");
    form.OnPickup(&p);
    CHECK(lastMsg == "撿到申請書");   // 單一訊息路徑不變
    EventBus::Instance().Clear();
}

// 完成集合時，完成獎勵 karma 只觸發一次（集滿最後一張才 +3）；2 參數建構子不給獎。
TEST_CASE("QuestFlagPickup：集合完成時完成獎勵 karma 只觸發一次") {
    Player p{nccu::engine::math::Vec2{0.0f, 0.0f}};
    const std::vector<std::string> set = {nccu::kFlagFoundNote1,
                                          nccu::kFlagFoundNote2,
                                          nccu::kFlagFoundNote3};
    QuestFlagPickup n1(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote1,
                       "a", set, 3);
    QuestFlagPickup n2(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote2,
                       "b", set, 3);
    QuestFlagPickup n3(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundNote3,
                       "c", set, 3);

    const int k0 = p.GetKarma();
    n1.OnPickup(&p);
    CHECK(p.GetKarma() == k0);          // 集合尚未完成
    n2.OnPickup(&p);
    CHECK(p.GetKarma() == k0);
    n3.OnPickup(&p);
    CHECK(p.GetKarma() == k0 + 3);      // 最後一張完成集合 -> +3，僅一次

    // 預設的 2 參數建構子仍可運作，且不給予獎勵。
    Player q{nccu::engine::math::Vec2{0.0f, 0.0f}};
    const int qk = q.GetKarma();
    QuestFlagPickup form(nccu::engine::math::Vec2{0, 0}, nccu::kFlagFoundForm);
    form.OnPickup(&q);
    CHECK(q.HasFlag(nccu::kFlagFoundForm));
    CHECK(q.GetKarma() == qk);
}

// World 把 3 張 Ch2 筆記延後到學霸被叫醒後才生成（且只一次），離開章節時隨名冊清除；申請書始終保留。
TEST_CASE("World 把 3 張 Ch2 筆記延後到學霸被叫醒後才生成，離開章節隨名冊清除") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    auto countNotes = [&]() -> std::size_t {
        std::size_t n = 0;
        for (const auto& o : w.Objects())
            if (dynamic_cast<const QuestFlagPickup*>(o.get())) ++n;
        return n;
    };

    // Ch1 進場現在只生成建構子的申請書（1 個，不受名冊追蹤）。苦主之傘的撿取物
    // 是延後生成的（只有在 Flag_SuitSeniorChoiceMade 之後、透過
    // MaybeSpawnChapter1VictimUmbrella 才出現）——因此章節進場時它不存在於世界中
    //（硬性守住主線）。
    CHECK(countNotes() == 1);

    // 進入 Ch2 不得生成筆記（要修的問題：筆記在學霸索取之前不該出現在任何地方）。
    // 依正式流程驅動狀態機（Transition + RespawnChapterRoster），讓延後生成的
    // 自我守門（semester==Ch2）看到正確狀態。建構子的申請書不受名冊追蹤，故持續存在。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK(countNotes() == 1);                  // 只有申請書；尚無筆記

    // 沒有叫醒旗標時，延後生成每幀都是無操作。
    CHECK_FALSE(w.MaybeSpawnChapter2Notes());
    CHECK(countNotes() == 1);

    // 叫醒學霸 -> 延後生成觸發一次，放下 3 張筆記。
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagBookworm);
    CHECK(w.MaybeSpawnChapter2Notes());        // 這一幀生成
    CHECK(countNotes() == 1 + 3);              // 申請書 + 3 張筆記
    CHECK_FALSE(w.MaybeSpawnChapter2Notes());  // 單次：絕不重生
    CHECK(countNotes() == 1 + 3);

    // 離開 Ch2 會把（延後生成的）筆記隨名冊清除；申請書保留。
    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(countNotes() == 1);
    EventBus::Instance().Clear();
}
