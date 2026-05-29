#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogState.h"
#include "game/controller/DialogChoiceApply.h"
#include "game/entities/Player.h"

using nccu::DialogState;
using nccu::SemesterState;

TEST_CASE("OpenNpcDialogSub loads victim Ch1 (a) opener, line-only") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "victim", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "……我的傘也不見了。");
    for (int i = 0; i < 4; ++i) CHECK(d.Advance() == nullptr);
    CHECK(d.CurrentLine() == "你也是被偷的嗎？可以幫我把傘找回來嗎……");
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
    CHECK(d.CurrentLine() == "……我的傘也不見了。");      // (a) opener line 0
    // Step through the 5 opener lines into choice mode.
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    REQUIRE(d.Choices().size() == 2);
    // Table order: subState 1 (b) first, subState 2 (c) second.
    // T1 first-person POV: the (c) ignore choice no longer carries a
    // 「玩家」 subject — re-authored to 「別過頭，當作沒看見」.
    CHECK(d.Choices()[0].label == "我去幫你追");
    CHECK(d.Choices()[1].label == "別過頭，當作沒看見");
    // Pick the help branch (index 0) -> its (b) consequence plays.
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "我去幫你追");
    CHECK(c->setsFlag == nccu::kFlagPromisedVictim);
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
    CHECK(d.Choices()[0].label == "詢問雨傘");
    CHECK(d.Choices()[1].label == "購買醜綠傘");
    CHECK(d.Choices()[2].label == "請阿姨喝一杯熱咖啡");
    // Cycle-8 audit F1: the Ch1 阿姨 (c) buy branch is now a pure
    // narrative seed — it sets NO flag (the inert Flag_KnowsUglyUmbrella
    // annotation was removed per the B3 precedent; src/include never
    // read it). Ending C's real trigger is the Ch4 集英樓 Vendor
    // (EndingGate.cpp:66 on Flag_BoughtUglyUmbrella).
    d.MoveChoice(1);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "購買醜綠傘");
    CHECK(c->setsFlag == "");     // F1: no flag (was Flag_KnowsUglyUmbrella)
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
    CHECK(coffee.setsFlag == nccu::kFlagBoughtCoffeeForAuntie);
    CHECK(coffee.flagValue == true);
    CHECK(coffee.karmaDelta == 5);
    // Confirm the choice end-to-end through the GameController applier.
    d.MoveChoice(2);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag(nccu::kFlagBoughtCoffeeForAuntie));
    CHECK(p.GetKarma() == k0 + 5);
}

// Item 5a regression: the Ch1 福利社阿姨 請咖啡 choice is ONCE-ONLY. Once
// Flag_BoughtCoffeeForAuntie_Ch1 is set (first pick), re-confirming the
// same choice on a re-talk must NOT re-apply the +5 karma (no farming).
// The guard lives in ApplyDialogChoice (a self-flagging choice the player
// already satisfied is inert). The inert (b)/(c) flavour choices (karma
// +0 / no flag) stay re-pickable — verified they don't move karma either.
TEST_CASE("Item 5a: shop_auntie coffee is once-only (no karma re-farm)") {
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();

    // First visit: pick coffee -> +5, flag set.
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();    // opener lines
        REQUIRE(d.AtChoice());
        d.MoveChoice(2);                            // 請咖啡
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.HasFlag(nccu::kFlagBoughtCoffeeForAuntie));
    CHECK(p.GetKarma() == k0 + 5);

    // Second visit: pick coffee AGAIN -> karma must NOT move (already done).
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();
        REQUIRE(d.AtChoice());
        d.MoveChoice(2);
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.GetKarma() == k0 + 5);                  // STILL +5, not +10

    // The inert (b) 詢問雨傘 choice (karma +0, no flag) is re-pickable and
    // moves nothing — confirms the guard only catches reward choices.
    {
        DialogState d;
        nccu::OpenNpcDialog(d, p, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
        for (int i = 0; i < 4; ++i) d.Advance();
        REQUIRE(d.AtChoice());
        d.MoveChoice(0);                            // 詢問雨傘
        const nccu::DialogChoice* c = d.Advance();
        REQUIRE(c != nullptr);
        nccu::ApplyDialogChoice(p, *c);
    }
    CHECK(p.GetKarma() == k0 + 5);                  // unchanged
}

TEST_CASE("ResolveOpenerSubState: ta gated by fetch-quest flags") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 0);   // fresh
    p.SetFlag(nccu::kFlagFoundForm);
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // reward
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // stays 1
}

TEST_CASE("ResolveOpenerSubState: victim recap gated by promise / grant flags") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 0);  // (a) fresh
    p.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 1);  // (b) promised
    // 善有善報: once the真傘 is granted (the返還 happened) the victim routes
    // to the (d) 重逢致謝 recap, outranking the promise recap.
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 3);  // (d) reunion
}

TEST_CASE("ResolveOpenerSubState: non-quest NPC always subState 0") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
    p.SetFlag(nccu::kFlagFoundForm);
    p.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
}

TEST_CASE("Player overload: ta reward applies karma/flag exactly once") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagFoundForm);
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");  // ta sub-1 line 0
    CHECK(p.HasFlag(nccu::kFlagHelpedTACh1));
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
    CHECK_FALSE(p2.HasFlag(nccu::kFlagHelpedTACh1));
}

TEST_CASE("Player overload: victim no flag still opens the 1b-2 choice") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘也不見了。");      // (a) opener line 0
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
}

TEST_CASE("Player overload: victim with promise flag is line-only recap") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagPromisedVictim);
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "真的？謝謝你……");        // victim sub-1 line 0
    CHECK(p.GetKarma() == k0);   // +5 was the choice's job, flag preset -> skip
}

// ---------------------------------------------------------------------------
// Ch1 suit_senior (b) seeds the Flag_ScoldedSenior arc KEY. The flag drives
// the cross-chapter "保持距離" arc — DialogOpener Ch2 suit_senior → (c)
// 尷尬讓開, Ch3 距離, Ch4 不出場 (World spawn filter) — so it must be
// reachable from a Ch1 choice.
//
// T1 reframe (CHANGELOG): the (b) choice is no longer a hostile 斥責 (-5).
// It is now a RATIONAL firm call-out — 「理性指出他品行不該，要回雨傘」,
// karma +3 — and the downstream reactions were softened from resentment to
// mild embarrassment (Chapter2Quest's Ch2 ScoldedSenior ripple is now
// karma-neutral). The flag KEY is retained so no branch goes dead; only the
// framing and karma sign changed. First-person POV: the choice label carries
// no 「玩家」 subject. Revert-verify: revert the chapter1.md (b) change and
// every CHECK below fails (choice 0 carries no flag → ScoldedSenior never
// set → the whole arc unreachable).
TEST_CASE("T1/F2: Ch1 suit_senior choice 0 (b) seeds Flag_ScoldedSenior (+3)") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    DialogState d;
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();

    // A1 (hard-gate): the 西裝學長 only presents his choice menu AFTER the
    // player has met the 苦主 and promised to chase the umbrella
    // (Flag_PromisedVictim). Without it he brushes the stranger off (a
    // line-only redirect, no menu). Set the promise so this test exercises
    // the genuine choice menu, as it always meant to.
    p.SetFlag(nccu::kFlagPromisedVictim);

    nccu::OpenNpcDialog(d, p, "suit_senior", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
    // Step through the 5 opener lines into choice mode.
    for (int i = 0; i < 5; ++i) d.Advance();
    REQUIRE(d.AtChoice());
    REQUIRE(d.Choices().size() == 3);

    // Choice index 0 is the (b) call-out branch. T1 reframe: it is now a
    // RATIONAL firm call-out (+3), not a hostile 斥責 (-5). It still sets
    // Flag_ScoldedSenior — kept as the "保持距離" arc KEY — so the Ch2/3/4
    // routing is unchanged; only the framing (embarrassment, not resentment)
    // and the karma sign moved. First-person label, no 「玩家」 subject.
    // (DialogOpener.cpp packs substates ≥ 1 ascending, so b→0, c→1, d→2 —
    // the ending_a.txt `choose 2` for suit_senior still resolves to (d)
    // HelpedSenior. State.jsonl byte-parity preserved.)
    const nccu::DialogChoice& scolded = d.Choices()[0];
    CHECK(scolded.label == "理性指出他品行不該，要回雨傘");
    CHECK(scolded.setsFlag == nccu::kFlagScoldedSenior);
    CHECK(scolded.flagValue == true);
    CHECK(scolded.karmaDelta == 3);

    // End-to-end via the GameController applier — the live confirm path.
    d.MoveChoice(0);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    nccu::ApplyDialogChoice(p, *c);
    CHECK(p.HasFlag(nccu::kFlagScoldedSenior));
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedSenior));   // mirror, not set
    CHECK(p.GetKarma() == k0 + 3);
}

TEST_CASE("F2: Ch2 suit_senior routes to (c) cold when Flag_ScoldedSenior") {
    // DialogOpener.cpp:101 was a read with no setter. After F2 the (b)
    // 斥責 path lands the flag and this branch fires.
    const auto Ch2 = SemesterState::Chapter2_Midterms;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag(nccu::kFlagScoldedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, p) == 2);   // (c)

    Player q{nccu::gfx::Vec2{0, 0}};
    // No flag → (a) opener — the default. (Mirror sanity check; if this
    // ever changes, the audit-F2 routing is no longer the only path.)
    CHECK(nccu::ResolveOpenerSubState("suit_senior", Ch2, q) == 0);
}
