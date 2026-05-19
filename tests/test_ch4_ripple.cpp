#include "doctest/doctest.h"
#include "Chapter4Quest.h"
#include "DialogOpener.h"
#include "Player.h"
#include "gfx/Vec2.h"

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

TEST_CASE("TryApplyCh4Ripple: 學長 +10 only on the (b) route, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // No HelpedSenior -> no +10 even at high karma.
    p.AddKarma(40);                          // ~90 (>70), no flag
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 40);

    p.SetFlag("Flag_HelpedSenior");
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 50);          // +10 landed
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 50);          // once only

    // karma not > 70 -> no +10.
    Player q = MakePlayer();
    q.SetFlag("Flag_HelpedSenior");          // default karma ~50 (<70)
    const int qk = q.GetKarma();
    nccu::TryApplyCh4Ripple(q, "suit_senior", kCh4);
    CHECK(q.GetKarma() == qk);
}

TEST_CASE("TryApplyCh4Ripple: 學霸 +5 on BookwormRecovered, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0);               // no flag
    p.SetFlag("Flag_BookwormRecovered");
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0 + 5);
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0 + 5);
}

TEST_CASE("TryApplyCh4Ripple: 助教 +10 / -15 are INDEPENDENT (L235)") {
    SUBCASE("HelpedTA only -> +10") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag("Flag_HelpedTA_Ch1");
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 + 10);
    }
    SUBCASE("ProfTrap only -> -15") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag("Flag_HasProfessorTrap");
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 15);
    }
    SUBCASE("both -> +10 AND -15 (net -5), each once") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag("Flag_HelpedTA_Ch1");
        p.SetFlag("Flag_HasProfessorTrap");
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 5);
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 5);       // both keys consumed
    }
}

// B3: the Ch1→Ch4 福利社阿姨 ripple. Flag_BoughtCoffeeForAuntie_Ch1
// (seeded by the Ch1 阿姨 (d) 請咖啡 choice) cashes in as the +3
// 直接情報 callback (chapter4.md 阿姨 (a)), once. Without the wiring
// the flag was set by nothing and read by nothing — dead content.
TEST_CASE("B3 TryApplyCh4Ripple: 福利社阿姨 +3 on coffee flag, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    // No Ch1 coffee -> indirect-info route, no +3.
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0);
    CHECK_FALSE(p.HasFlag("Flag_Ch4Rippled_Auntie"));

    p.SetFlag("Flag_BoughtCoffeeForAuntie_Ch1");
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0 + 3);                 // 情分 callback landed
    CHECK(p.HasFlag("Flag_Ch4Rippled_Auntie"));
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0 + 3);                 // once only

    // Wrong state -> no-op even with the flag.
    Player q = MakePlayer();
    q.SetFlag("Flag_BoughtCoffeeForAuntie_Ch1");
    const int qk = q.GetKarma();
    nccu::TryApplyCh4Ripple(q, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
    CHECK(q.GetKarma() == qk);
}

// B3: Ch4 阿姨 dialogue routes on the Ch1 coffee flag — (a) 直接情報
// (subState 0) when the player bought coffee in Ch1, else (d) 間接情報
// (subState 3). The dead annotation gated nothing before this.
TEST_CASE("B3 ResolveOpenerSubState: Ch4 shop_auntie coffee routing") {
    Player p = MakePlayer();
    // No Ch1 coffee -> (d) 間接情報.
    CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh4, p) == 3);
    p.SetFlag("Flag_BoughtCoffeeForAuntie_Ch1");
    // Coffee情分 -> (a) 直接情報.
    CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh4, p) == 0);
}

TEST_CASE("TryApplyCh4Ripple: wrong state / wrong npc -> no-op") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    p.SetFlag("Flag_HelpedSenior");
    p.AddKarma(40);
    nccu::TryApplyCh4Ripple(p, "suit_senior",
                            SemesterState::Chapter1_AddDrop);
    nccu::TryApplyCh4Ripple(p, "victim", kCh4);
    CHECK(p.GetKarma() == k0 + 40);          // unchanged
}
