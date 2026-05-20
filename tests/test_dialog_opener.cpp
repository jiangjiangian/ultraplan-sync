#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"
#include "GameController.h"
#include "Player.h"

using nccu::DialogState;
using nccu::SemesterState;

TEST_CASE("OpenNpcDialogSub loads victim Ch1 (a) opener, line-only") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "victim", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "……我的傘不見了。");
    for (int i = 0; i < 4; ++i) CHECK(d.Advance() == nullptr);
    CHECK(d.CurrentLine() == "你也是被偷的嗎？");
    CHECK(d.Advance() == nullptr);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialogSub unknown npcId leaves dialog inactive") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "nobody", SemesterState::Chapter1_AddDrop, 0);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialogSub suit_senior Ch1 (a) opener first line") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "suit_senior",
                           SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
}

TEST_CASE("3-arg OpenNpcDialog victim Ch1: opener + 2 choices, (b) plays") {
    DialogState d;
    nccu::OpenNpcDialog(d, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘不見了。");      // (a) opener line 0
    // Step through the 5 opener lines into choice mode.
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    REQUIRE(d.Choices().size() == 2);
    // Table order: subState 1 (b) first, subState 2 (c) second.
    CHECK(d.Choices()[0].label == "我去幫你追");
    CHECK(d.Choices()[1].label == "玩家無視走過");
    // Pick the help branch (index 0) -> its (b) consequence plays.
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "我去幫你追");
    CHECK(c->setsFlag == "Flag_PromisedVictim");
    CHECK(c->flagValue == true);
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "真的？謝謝你……");          // (b) line 0
    while (d.Active()) d.Advance();                       // exhaust -> close
    CHECK_FALSE(d.Active());
}

TEST_CASE("3-arg OpenNpcDialog shop_auntie Ch1: opener + buy-umbrella choice") {
    DialogState d;
    nccu::OpenNpcDialog(d, "shop_auntie", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    // Step through the 4 (a) opener lines into choice mode.
    for (int i = 0; i < 4; ++i) d.Advance();
    CHECK(d.AtChoice());
    // B3: a third Ch1 choice — 請阿姨喝咖啡 — seeds the Ch1→Ch4
    // 福利社阿姨 ripple the GDD names but engine never read.
    REQUIRE(d.Choices().size() == 3);
    // Table order: subState 1, 2, 3 (a<b<c<d, opener is subState 0).
    CHECK(d.Choices()[0].label == "玩家詢問雨傘");
    CHECK(d.Choices()[1].label == "玩家購買醜綠傘後");
    CHECK(d.Choices()[2].label == "請阿姨喝一杯熱咖啡");
    // C.1 (pre-approved): the Ch1 阿姨 buy branch now seeds
    // Flag_KnowsUglyUmbrella, NOT Flag_BoughtUglyUmbrella. The real
    // purchase (→ Ending C) moved to the Ch4 集英樓 Vendor; the Ch1
    // buy is now a 伏筆 seed, not an instant ending trigger.
    d.MoveChoice(1);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "玩家購買醜綠傘後");
    CHECK(c->setsFlag == "Flag_KnowsUglyUmbrella");
    CHECK(c->flagValue == true);
    while (d.Active()) d.Advance();    // exhaust -> close
    CHECK_FALSE(d.Active());
}

// B3 regression: the Ch1 福利社阿姨 (d) 請咖啡 choice is the SEED of
// Flag_BoughtCoffeeForAuntie_Ch1 — the GDD-named Ch1→Ch4 ripple that
// was dead content (set by nothing, read by nothing). Picking it must
// set the flag (+5 karma); WITHOUT the chapter1.md (d) substate +
// the choice-opener path this choice does not exist and the REQUIRE
// on its presence fails.
TEST_CASE("B3: Ch1 shop_auntie coffee choice seeds BoughtCoffeeForAuntie") {
    DialogState d;
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    nccu::OpenNpcDialog(d, p, "shop_auntie", SemesterState::Chapter1_AddDrop);
    for (int i = 0; i < 4; ++i) d.Advance();          // (a) opener lines
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);
    const nccu::DialogChoice& coffee = d.Choices()[2];
    CHECK(coffee.label == "請阿姨喝一杯熱咖啡");
    CHECK(coffee.setsFlag == "Flag_BoughtCoffeeForAuntie_Ch1");
    CHECK(coffee.flagValue == true);
    CHECK(coffee.karmaDelta == 5);
    // Confirm the choice end-to-end through the GameController applier.
    d.MoveChoice(2);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag("Flag_BoughtCoffeeForAuntie_Ch1"));
    CHECK(p.GetKarma() == k0 + 5);
}

TEST_CASE("ResolveOpenerSubState: ta gated by fetch-quest flags") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 0);   // fresh
    p.SetFlag("Flag_FoundForm");
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // reward
    p.SetFlag("Flag_HelpedTA_Ch1");
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // stays 1
}

TEST_CASE("ResolveOpenerSubState: victim recap gated by promise flag") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 0);  // fresh
    p.SetFlag("Flag_PromisedVictim");
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 1);  // recap
}

TEST_CASE("ResolveOpenerSubState: non-quest NPC always subState 0") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
    p.SetFlag("Flag_FoundForm");
    p.SetFlag("Flag_PromisedVictim");
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
}

TEST_CASE("Player overload: ta reward applies karma/flag exactly once") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_FoundForm");
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");  // ta sub-1 line 0
    CHECK(p.HasFlag("Flag_HelpedTA_Ch1"));
    CHECK(p.GetKarma() == k0 + 5);
    // Re-open with the SAME player -> apply-once guard skips (no double).
    DialogState d2;
    nccu::OpenNpcDialog(d2, p, "ta", Ch1);
    CHECK(d2.Active());
    CHECK(d2.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");
    CHECK(p.GetKarma() == k0 + 5);
}

TEST_CASE("Player overload: ta with no flag is the line-only intro") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p2{nccu::gfx::Vec2{0, 0}};
    const int k0 = p2.GetKarma();
    DialogState d2;
    nccu::OpenNpcDialog(d2, p2, "ta", Ch1);
    CHECK(d2.Active());
    CHECK_FALSE(d2.AtChoice());
    CHECK(d2.CurrentLine() == "同學，加退選截止了，現在不受理。");  // ta sub-0 line 0
    CHECK(p2.GetKarma() == k0);
    CHECK_FALSE(p2.HasFlag("Flag_HelpedTA_Ch1"));
}

TEST_CASE("Player overload: victim no flag still opens the 1b-2 choice") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘不見了。");      // (a) opener line 0
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
}

TEST_CASE("Player overload: victim with promise flag is line-only recap") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_PromisedVictim");
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "真的？謝謝你……");        // victim sub-1 line 0
    CHECK(p.GetKarma() == k0);   // +5 was the choice's job, flag preset -> skip
}

// ---------------------------------------------------------------------------
// Cycle-8 audit Finding 2: restore the GDD §伍 Ch1「斥責學長」漣漪選擇.
// Pre-fix: chapter1.md suit_senior had (a)/(b)/(c)/(d) with (c) and (d)
// both annotating `Flag_ScoldedSenior = false`, and NO substate setting
// `Flag_ScoldedSenior = true`. Yet DialogOpener.cpp:101 (Ch2 suit_senior
// → (c) cold) + Chapter2Quest.cpp:66 (cold -3 ripple) + chapter4.md:82/
// 88/405 (學長 不出場) + voice_bible.md:51 (Interlude 側身走開) ALL
// branch on `Flag_ScoldedSenior = true` — so a whole GDD-named
// cross-chapter cold-senior arc was permanently unreachable dead
// narrative. Fix: chapter1.md (b) re-authored from inert "拒絕，no flag"
// into the GDD §伍 選項 B "憤怒斥責" path (-5 / Flag_ScoldedSenior=true);
// chose to re-author (b) not append (e) because DialogLoader.cpp:83
// hard-caps substate letters at 'a'..'d'. Revert-verified: with the
// chapter1.md (b) change reverted, every CHECK below fails (suit_senior
// choice index 0 carries no flag → ScoldedSenior never set → Ch2
// routing falls through to (a) opener, Ch2 ripple TryApplyCh2Ripple
// finds neither HelpedSenior nor ScoldedSenior so it grants nothing).
TEST_CASE("F2: Ch1 suit_senior choice 0 (b) seeds Flag_ScoldedSenior") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    DialogState d;
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();

    nccu::OpenNpcDialog(d, p, "suit_senior", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
    // Step through the 5 opener lines into choice mode.
    for (int i = 0; i < 5; ++i) d.Advance();
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);

    // Choice index 0 is the new (b) 憤怒斥責, dressed as GDD §伍 選項 B.
    // (DialogOpener.cpp:62-68 packs substates ≥ 1 in ascending order, so
    // b→0, c→1, d→2 — the ending_a.txt `choose 2` for suit_senior still
    // resolves to (d) HelpedSenior. State.jsonl byte-parity preserved.)
    const nccu::DialogChoice& scolded = d.Choices()[0];
    CHECK(scolded.label == "玩家憤怒斥責，奪回雨傘");
    CHECK(scolded.setsFlag == "Flag_ScoldedSenior");
    CHECK(scolded.flagValue == true);
    CHECK(scolded.karmaDelta == -5);

    // End-to-end via the GameController applier — the live confirm path.
    d.MoveChoice(0);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag("Flag_ScoldedSenior"));
    CHECK_FALSE(p.HasFlag("Flag_HelpedSenior"));   // mirror, not set
    CHECK(p.GetKarma() == k0 - 5);
}

TEST_CASE("F2: Ch2 suit_senior routes to (c) cold when Flag_ScoldedSenior") {
    // DialogOpener.cpp:101 was a read with no setter. After F2 the (b)
    // 斥責 path lands the flag and this branch fires.
    const auto Ch2 = SemesterState::Chapter2_Midterms;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_ScoldedSenior");
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, p) == 2);   // (c)

    Player q{nccu::gfx::Vec2{0, 0}};
    // No flag → (a) opener — the default. (Mirror sanity check; if this
    // ever changes, the audit-F2 routing is no longer the only path.)
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, q) == 0);
}
