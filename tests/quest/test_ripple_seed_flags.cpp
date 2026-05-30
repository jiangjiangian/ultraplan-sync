/**
 * @file test_ripple_seed_flags.cpp
 * @brief 驗證 Ch1 各「壞傘」領取時種下對應漣漪旗標（ProfTrap/Cursed），含冪等與污染值；好傘/脆傘不種旗標且 BeClaimed 冪等。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/entities/CursedUmbrella.h"
#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/FragileUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"

// Ch1 的「壞傘」領取，是 Ch2/Ch3/Ch4 兌現負面漣漪的源頭（助教 -10/-15、
// 學霸冷淡、Ending B）。在此之前這些旗標從未被設下，整條鏈都是死內容。
// 以下案例釘住「在領取點種下旗標」以及其專一性。

// 領取 ProfessorTrapUmbrella 會種下 Flag_HasProfessorTrap（只一次，第二次領取為無操作）。
TEST_CASE("領取 ProfessorTrapUmbrella 種下 Flag_HasProfessorTrap 一次") {
    EventBus::Instance().Clear();
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasProfessorTrap));

    ProfessorTrapUmbrella trap{nccu::engine::math::Vec2{0, 0}};
    trap.BeClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK(p.HasUmbrella());

    // 冪等守門：第二次領取為無操作，旗標維持已設。
    trap.BeClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagHasProfessorTrap));
}

// 領取 CursedUmbrella 會種下 Flag_TookCursedUmbrella 並提升污染值；karma 在撿取當下不變、污染值不重複提升。
TEST_CASE("領取 CursedUmbrella 種下 Flag_TookCursedUmbrella 並提升污染值") {
    // 此前的做法是在撿取時一次性扣 -30 karma；現在把 karma 代價從撿取移到
    // 各章的污染衰減（在 SceneRouter 進入 Ch2/3/4 時）。旗標與 Ending B 路徑維持不變。
    EventBus::Instance().Clear();
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p.GetCursedTaint() == 0);

    CursedUmbrella cursed{nccu::engine::math::Vec2{0, 0}};
    cursed.BeClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p.GetCursedTaint() == 1);          // 提升的是污染值，不是 karma
    CHECK(p.GetKarma() == k0);               // 撿取當下 karma 不變

    cursed.BeClaimed(&p);                    // 冪等（isActive_ 守門）
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p.GetCursedTaint() == 1);          // 不重複提升
}

// 紅線契約：雨傘的 BeClaimed／撿取必須保留 isActive_ 冪等守門。TrueUmbrella 與
// FragileUmbrella::BeClaimed 過去沒有守門，僅依賴呼叫端的活動過濾；但 OnPickup 是
// 第二個進入點，契約要求方法本身也要守門（縱深防禦）。無守門的 TrueUmbrella 在
// 第二次呼叫時會重發 UmbrellaClaimed——由於 Ch1/Ch3 的事件接線會因該事件推進學期，
// 這是潛在的重複轉場風險。
TEST_CASE("TrueUmbrella::BeClaimed 具冪等性（不會重複發 UmbrellaClaimed）") {
    EventBus::Instance().Clear();
    int claimed = 0;
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [&claimed](const Event& e) { if (e.text == "TrueUmbrella") ++claimed; });
    Player p{nccu::engine::math::Vec2{0, 0}};

    TrueUmbrella good{nccu::engine::math::Vec2{0, 0}};
    good.BeClaimed(&p);
    CHECK(p.HasUmbrella());
    CHECK_FALSE(good.IsActive());            // 已標記待清除
    CHECK(claimed == 1);

    good.BeClaimed(&p);                       // 第二次呼叫：必須是無操作
    CHECK(claimed == 1);                      // 不重發（修正前會變成 2）
    EventBus::Instance().Clear();
}

// FragileUmbrella::BeClaimed 同樣具冪等性，第二次呼叫不得重發 UmbrellaClaimed。
TEST_CASE("FragileUmbrella::BeClaimed 具冪等性（不會重複發 UmbrellaClaimed）") {
    EventBus::Instance().Clear();
    int claimed = 0;
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [&claimed](const Event& e) { if (e.text == "FragileUmbrella") ++claimed; });
    Player p{nccu::engine::math::Vec2{0, 0}};

    FragileUmbrella fragile{nccu::engine::math::Vec2{0, 0}};
    fragile.BeClaimed(&p);
    CHECK(p.HasUmbrella());
    CHECK_FALSE(fragile.IsActive());
    CHECK(claimed == 1);

    fragile.BeClaimed(&p);                    // 第二次呼叫：必須是無操作
    CHECK(claimed == 1);                      // 不重發（修正前會變成 2）
    EventBus::Instance().Clear();
}

// 好傘／脆傘不會種下任何漣漪旗標（保證壞傘旗標的專一性）。
TEST_CASE("好傘／脆傘不會種下任何漣漪旗標") {
    EventBus::Instance().Clear();
    Player p{nccu::engine::math::Vec2{0, 0}};

    TrueUmbrella good{nccu::engine::math::Vec2{0, 0}};
    good.BeClaimed(&p);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    Player q{nccu::engine::math::Vec2{0, 0}};
    FragileUmbrella fragile{nccu::engine::math::Vec2{0, 0}};
    fragile.BeClaimed(&q);
    CHECK_FALSE(q.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK_FALSE(q.HasFlag(nccu::kFlagTookCursedUmbrella));
}
