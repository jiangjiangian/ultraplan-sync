#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/ChapterGate.h"
#include "game/quest/ChapterQuestItems.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/entities/Player.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/entities/CursedUmbrella.h"
#include "game/entities/TransparentUmbrella.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {

Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }

// Captures every UmbrellaClaimed payload + the LAST ShowMessage. A scoped
// subscription keyed on free locals (the test_chapter_transitions idiom).
struct Capture {
    std::vector<std::string> umbrellaClaims;
    std::string              lastMessage;
    EventBus::Subscription   subUmb;
    EventBus::Subscription   subMsg;
};

[[nodiscard]] Capture MakeCapture() {
    Capture c;
    c.subUmb = EventBus::Instance().ScopedSubscribe(
        EventType::UmbrellaClaimed,
        [&c](const Event& e) { c.umbrellaClaims.push_back(e.text); });
    c.subMsg = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&c](const Event& e) { c.lastMessage = e.text; });
    return c;
}

}  // namespace

// REGRESSION 1 — the reciprocity grant (T2: a real exchange SCENE before
// the chapter clears). Returning the 苦主 his umbrella sets
// Flag_HasTrueUmbrella + HasUmbrella ONLY when the player BOTH promised AND
// holds the victim's umbrella; every other state (wrong state/npc, no
// promise, promised-but-empty-handed, already-granted) is a no-op. T2: the
// grant NO LONGER publishes UmbrellaClaimed inline — that is deferred to
// LiftChapter1Clear (so the (d) exchange dialogue plays first).
TEST_CASE("TryReturnVictimUmbrella: grants the真傘 only after promise + return") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();

    // Wrong state / wrong npc -> no-op.
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter2_Midterms);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "ta", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());

    // Promised? NO -> no-op (the (a)/(b) dialogue owns the promise; nothing
    // to return yet). No grant.
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());

    // Promised but WITHOUT the victim's umbrella -> reminder only, no grant.
    p.SetFlag(nccu::kFlagPromisedVictim);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK_FALSE(p.HasUmbrella());
    CHECK(cap.umbrellaClaims.empty());
    CHECK_FALSE(cap.lastMessage.empty());           // a nudge was shown

    // Promised AND holding the victim's umbrella -> the GRANT fires (flags
    // only — T2: NO UmbrellaClaimed yet, the (d) exchange must play first).
    p.SetFlag(nccu::kFlagHasVictimUmbrella);
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));        // Ending A's condition
    CHECK(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasVictimUmbrella));  // 苦主 took his傘
    CHECK(cap.umbrellaClaims.empty());               // T2: clear is DEFERRED
    CHECK_FALSE(p.HasFlag(nccu::kFlagClearChapter1)); // not fired yet

    // Idempotent on a re-talk: already granted -> still no UmbrellaClaimed.
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(cap.umbrellaClaims.empty());
    EventBus::Instance().Clear();
}

// REGRESSION 1b — the grant must NOT fire if the player holds the victim's
// umbrella but never promised (defensive: both conditions are required).
TEST_CASE("TryReturnVictimUmbrella: umbrella without a promise never grants") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagHasVictimUmbrella);          // umbrella, but no promise
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(cap.umbrellaClaims.empty());
    EventBus::Instance().Clear();
}

// REGRESSION 1c (T2) — the exchange SEQUENCES correctly: the grant is silent
// (the (d) exchange dialogue plays first), and only LiftChapter1Clear, AFTER
// the dialogue has CLOSED, publishes UmbrellaClaimed → the EventWiring Ch1
// sibling-if transitions Ch1→Interlude (returnTo Ch2). While the dialogue is
// still ACTIVE, the clear must NOT fire (the player must read the scene).
TEST_CASE("T2: victim exchange plays BEFORE the Ch1 clear (deferred)") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    std::string buildingName;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, buildingName);
    Capture cap = MakeCapture();
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagPromisedVictim);
    p.SetFlag(nccu::kFlagHasVictimUmbrella);

    // 1) The grant: flags set, but NO transition yet (UmbrellaClaimed held).
    nccu::TryReturnVictimUmbrella(EventBus::Instance(), p, "victim", m.Current());
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // still Ch1
    CHECK(cap.umbrellaClaims.empty());

    // 2) The (d) exchange dialogue is on screen -> the clear is BLOCKED.
    nccu::DialogState d;
    d.Open({"這就是我的傘，太好了，謝謝你！", "還你。雨還沒停，路上小心。"});
    REQUIRE(d.Active());
    nccu::LiftChapter1Clear(EventBus::Instance(), p, m.Current(), d);
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);   // not yet
    CHECK(cap.umbrellaClaims.empty());

    // 3) The player finishes reading; the dialogue closes -> NOW it fires.
    d.Close();
    nccu::LiftChapter1Clear(EventBus::Instance(), p, m.Current(), d);
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "TrueUmbrella");
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter2_Midterms);

    // 4) Once-guard: a later poll does not re-publish / re-transition.
    nccu::LiftChapter1Clear(EventBus::Instance(), p, SemesterState::Chapter1_AddDrop, d);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// REGRESSION 2 — the three-ending architecture is untouched: a CursedUmbrella
// claim still works (its BeClaimed sets Flag_TookCursedUmbrella + publishes
// UmbrellaClaimed → Ending B path), independent of the victim grant.
TEST_CASE("Ch1 morality umbrellas still claimable (Ending B path preserved)") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // Cursed claim is QuestGate-d on the promise (TransparentUmbrella), so
    // promise first, then claim (this mirrors the in-game gate).
    p.SetFlag(nccu::kFlagPromisedVictim);
    CursedUmbrella cursed{nccu::engine::math::Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);

    CHECK(p.HasUmbrella());
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));      // Ending B seed
    CHECK(p.GetCursedTaint() == 1);                   // P2: taint bumped at pickup
    CHECK(p.GetKarma() == k0);                        // P2: pickup-time karma-neutral
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "CursedUmbrella"); // its own clear path
    // Idempotent (CLAUDE.md §5 / BUGLEDGER L2).
    cursed.BeClaimed(&p);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// REGRESSION 3 — the findable victim's-umbrella pickup sets its flag (the
// QuestFlagPickup the player collects out in the world).
TEST_CASE("Ch1 victim's-umbrella pickup sets Flag_HasVictimUmbrella") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();

    // It is the single Ch1 quest item, at (1450,1450), no completion set.
    const auto& ch1 = nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 1);
    const auto& qi = ch1[0];
    CHECK(qi.flag == nccu::kFlagHasVictimUmbrella);

    Player p = MakePlayer();
    QuestFlagPickup pickup{qi.pos, qi.flag, qi.message, qi.completionFlags,
                           qi.completionKarma, qi.countMessages};
    pickup.OnPickup(&p);
    CHECK(p.HasFlag(nccu::kFlagHasVictimUmbrella));
    CHECK(cap.lastMessage == qi.message);             // its own pickup line
    EventBus::Instance().Clear();
}

// A1 REGRESSION — the 西裝學長 must REDIRECT (line-only, no menu) before the
// player has met the 苦主 (Flag_PromisedVictim). The spine is hard-gated
// 苦主 → 學長 → 傘 → 苦主; talking to the 學長 first must NOT open the
// ripple-critical choice menu (which would let the player commit the 學長
// choice — and even claim a morality umbrella — before the chapter's first
// beat). After the promise, the genuine menu shows. Revert-verify: drop the
// `!Flag_PromisedVictim` redirect branch in DialogOpener and the first half
// fails (a menu opens before the promise).
TEST_CASE("A1: 西裝學長 redirects (no menu) until the 苦主 is met") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();
    const auto Ch1 = SemesterState::Chapter1_AddDrop;

    // BEFORE the promise: line-only redirect, NO choice menu ever.
    Player p = MakePlayer();
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "suit_senior", Ch1);
    REQUIRE(d.Active());
    bool reachedMenu = false;
    for (int i = 0; i < 32 && d.Active(); ++i) {
        if (d.AtChoice()) { reachedMenu = true; break; }
        d.Advance();
    }
    CHECK_FALSE(reachedMenu);                 // brushed off, never a menu
    CHECK(d.Choices().empty());

    // AFTER the promise: the genuine (b)/(c)/(d) choice menu opens.
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagPromisedVictim);
    nccu::DialogState d2;
    nccu::OpenNpcDialog(d2, q, "suit_senior", Ch1);
    REQUIRE(d2.Active());
    bool reachedMenu2 = false;
    for (int i = 0; i < 32 && d2.Active(); ++i) {
        if (d2.AtChoice()) { reachedMenu2 = true; break; }
        d2.Advance();
    }
    CHECK(reachedMenu2);                       // now the branch menu shows
    CHECK_FALSE(d2.Choices().empty());
}

// A1 REGRESSION — the 苦主's umbrella pickup is DEFERRED: it does NOT exist
// in the world at Ch1 entry, appears ONLY after Flag_SuitSeniorChoiceMade
// (World::MaybeSpawnChapter1VictimUmbrella, the sibling of
// MaybeSpawnChapter2Notes / MaybeSpawnChapter3Umbrella), spawns exactly once,
// and is roster-swept on a state change. So the player cannot grab the
// umbrella before the 學長 step — the spine is unskippable. Revert-verify:
// re-add the Ch1 entry-spawn (drop the Chapter1_AddDrop skip in
// SpawnChapterNpcs) and the first CHECK fails (an umbrella exists at entry).
namespace {
// Counts every QuestFlagPickup in the world. At Ch1 entry exactly ONE exists
// (the ctor-spawned 申請書, Flag_FoundForm); the deferred victim-umbrella
// pickup is the only other Ch1 quest item, so the count moving 1 -> 2 is
// exactly "the victim umbrella spawned" (the 申請書 is never swept — it is
// not roster-tracked).
std::size_t CountQuestPickups(const nccu::World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const QuestFlagPickup*>(o.get())) ++n;
    return n;
}
}  // namespace

TEST_CASE("A1: 苦主's umbrella defers until Flag_SuitSeniorChoiceMade, then sweeps") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);

    // At Ch1 entry (the ctor's default state) the victim-umbrella pickup does
    // NOT exist — only the ctor 申請書 (Flag_FoundForm) is present.
    CHECK(CountQuestPickups(w) == 1);

    // Without the choice flag the deferred spawn is a no-op every frame.
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());
    CHECK(CountQuestPickups(w) == 1);

    // Committing the 西裝學長 choice (Flag_SuitSeniorChoiceMade) makes the
    // victim umbrella appear ONCE (count 1 申請書 -> 2: + the umbrella).
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagSuitSeniorChoiceMade);
    CHECK(w.MaybeSpawnChapter1VictimUmbrella());        // spawns this frame
    CHECK(CountQuestPickups(w) == 2);
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());  // one-shot
    CHECK(CountQuestPickups(w) == 2);

    // Leaving Ch1 sweeps the roster-tracked umbrella (re-arms the one-shot);
    // the ctor 申請書 is NOT roster-tracked, so it persists (count back to 1).
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    CHECK(CountQuestPickups(w) == 1);
    EventBus::Instance().Clear();
}

TEST_CASE("A1: MaybeSpawnChapter1VictimUmbrella is a no-op outside Ch1") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagSuitSeniorChoiceMade);   // flag set, but...

    // Ch2: no Ch1-umbrella spawn (it is Ch1-scoped). The ctor 申請書 is NOT
    // roster-tracked so it persists across the transition (count stays 1), and
    // Ch2 spawns no quest items at entry (notes are deferred on the wake flag),
    // so MaybeSpawnChapter1VictimUmbrella adds nothing.
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK_FALSE(w.MaybeSpawnChapter1VictimUmbrella());
    CHECK(CountQuestPickups(w) == 1);                   // 申請書 only
    EventBus::Instance().Clear();
}

// B3 REGRESSION — the Ch1 福利社阿姨 (c) 購買醜綠傘 is a REAL purchase
// (the bug: 沒扣錢 — it was a "narrative seed" with no money/umbrella).
// TryBuyAuntieUglyUmbrella must deduct exactly 80 元, grant the HELD ugly
// umbrella, emit a 花費/餘額 toast, and CRUCIALLY NOT set
// Flag_BoughtUglyUmbrella (the Ch4 Vendor owns that Ending-C lock). The
// poor / idempotent / wrong-context paths must be clean no-ops.
TEST_CASE("B3: Ch1 阿姨 醜傘 buy deducts 80 + grants held Ugly, no Ending-C flag") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    const int money0 = p.GetMoney();                    // ctor default 100
    REQUIRE(money0 >= nccu::kCh1UglyUmbrellaPrice);      // affordable from start
    const int karma0 = p.GetKarma();

    // Wrong state / wrong npc / wrong label -> all no-ops (no charge, no傘).
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter4_Finals));
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "victim", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "shop_auntie", "詢問雨傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);

    // The real buy: 80 元 deducted, ugly umbrella in hand, toast shown.
    CHECK(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0 - nccu::kCh1UglyUmbrellaPrice);   // 100 -> 20
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Ugly);
    CHECK(p.HasUmbrella());                              // auto-shelter armed
    CHECK(p.GetKarma() == karma0);                       // // karma +0
    CHECK_FALSE(p.HasFlag(nccu::kFlagBoughtUglyUmbrella));   // NOT the C lock
    CHECK_FALSE(cap.lastMessage.empty());                // 花費/餘額 toast
    CHECK(cap.lastMessage.find("80") != std::string::npos);   // the spend
    CHECK(cap.lastMessage.find("20") != std::string::npos);   // the balance

    // Idempotent: a re-talk picks the menu again, but already holding Ugly
    // must NOT re-deduct another 80.
    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == money0 - nccu::kCh1UglyUmbrellaPrice);   // still 20
    EventBus::Instance().Clear();
}

TEST_CASE("B3: Ch1 阿姨 醜傘 buy is fund-guarded (poor → no charge, no umbrella)") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    // Drain the purse below the price (DeductMoney returns false → no buy).
    REQUIRE(p.DeductMoney(p.GetMoney() - 10));           // leave 10 < 80
    REQUIRE(p.GetMoney() < nccu::kCh1UglyUmbrellaPrice);

    CHECK_FALSE(nccu::TryBuyAuntieUglyUmbrella(EventBus::Instance(), 
        p, "shop_auntie", "購買醜綠傘", SemesterState::Chapter1_AddDrop));
    CHECK(p.GetMoney() == 10);                           // purse untouched
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);   // no umbrella granted
    CHECK(cap.lastMessage == "你錢不夠");                 // the poor toast
    EventBus::Instance().Clear();
}
