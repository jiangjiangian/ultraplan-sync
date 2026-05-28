#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "quest/Chapter2Quest.h"
#include "controller/EventBus.h"
#include "entities/NPC.h"
#include "entities/Player.h"
#include "entities/GameObject.h"
#include "world/World.h"
#include "state/SemesterState.h"
#include "state/SemesterStateMachine.h"
#include "gfx/Vec2.h"
#include <string_view>

using nccu::SemesterState;
using nccu::World;

// G-3 regressions. In the Ch2→Ch3 Interlude ONLY, while the player still
// holds the 管理員 loaner, returning it at the 中正圖書館 front grants karma
// +10 ONCE, clears the held umbrella + Flag_LibrarianUmbrella, and shows a
// thank-you. It grants NO ending-affecting umbrella flag, and if skipped the
// loaner still auto-clears on Ch3 entry (no karma) — a purely positive
// optional 責任感 choice.

namespace {

Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }

// Put the player in the "holding the Ch2 loaner" state.
void GiveLoaner(Player& p) {
    p.SetHeldUmbrella(HeldUmbrella::Loaner);
    p.SetFlag(nccu::kFlagLibrarianUmbrella);
}

bool HasReturnMarker(const World& w) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(nccu::kNpcLibrarianReturn))
            return true;
    return false;
}

} // namespace

TEST_CASE("G-3: returning 管理員的傘 grants +10 ONCE, clears the loaner") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    GiveLoaner(p);
    const int k0 = p.GetKarma();

    // The +10 lands only in the Interlude that returns to Ch3.
    nccu::TryReturnLibrarianUmbrella(
        p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay);

    CHECK(p.GetKarma() == k0 + 10);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);   // loaner surrendered
    CHECK_FALSE(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagLibrarianUmbrella));
    CHECK(p.HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
    // Crucially NOT an ending flag — the loaner never unlocks Ending A.
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));

    // Idempotent: a re-talk replays a closure line but grants NO second +10.
    nccu::TryReturnLibrarianUmbrella(
        p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay);
    CHECK(p.GetKarma() == k0 + 10);
    EventBus::Instance().Clear();
}

TEST_CASE("G-3: the loaner-return is scoped to the Ch2→Ch3 market + the marker") {
    EventBus::Instance().Clear();

    // Wrong return-destination market (e.g. Ch1→Ch2, which returns to Ch2):
    // no grant even at the return-point holding the loaner.
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(
            p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
            SemesterState::Chapter2_Midterms);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);   // still held
    }
    // Wrong state (not the Interlude): no grant.
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(
            p, nccu::kNpcLibrarianReturn, SemesterState::Chapter2_Midterms,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    }
    // Wrong npcId (a different NPC at the same market): no grant.
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(
            p, "shop_auntie", SemesterState::Interlude_Market,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    }
    // Not holding the loaner (empty-handed): no grant, no spurious flag.
    {
        Player p = MakePlayer();
        nccu::TryReturnLibrarianUmbrella(
            p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK_FALSE(p.HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
    }
    EventBus::Instance().Clear();
}

TEST_CASE("G-3: the return-point marker spawns ONLY in the Ch2→Ch3 market "
          "while holding the loaner") {
    World w("", /*loadSprites=*/false);

    // Ch1 (no Interlude): no marker.
    CHECK_FALSE(HasReturnMarker(w));

    // Interlude returning to Ch2 (the Ch1→Ch2 market): the loaner cannot be
    // held yet, so even forcing the held kind, the destination gate blocks it.
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
    w.Semester().Transition(SemesterState::Interlude_Market);
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());
    CHECK_FALSE(w.MaybeSpawnInterludeLibrarianReturn());
    CHECK_FALSE(HasReturnMarker(w));

    // Interlude returning to Ch3 (the Ch2→Ch3 market), holding the loaner:
    // the marker appears, exactly once.
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    // Re-arm the one-shot via a roster respawn (a fresh Interlude entry).
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());
    CHECK(w.MaybeSpawnInterludeLibrarianReturn());        // spawns
    CHECK(HasReturnMarker(w));
    CHECK_FALSE(w.MaybeSpawnInterludeLibrarianReturn());  // one-shot

    // The Player invariant survives the deferred spawn.
    CHECK(w.Objects().front().get() ==
          static_cast<GameObject*>(w.GetPlayer()));
}

TEST_CASE("G-3: skipping the return is safe — the loaner auto-clears on Ch3") {
    // The marker only appears while holding the loaner; if the player walks
    // past it, SceneRouter's Ch3-entry reset still empties the held umbrella
    // (no karma). This pins that the return is purely optional, never a gate:
    // a player who never returns it still loses it on Ch3 entry and proceeds.
    World w("", /*loadSprites=*/false);
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    w.Semester().Transition(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());

    // No return performed. The loaner is still held in the Interlude.
    CHECK(w.GetPlayer()->HeldUmbrellaKind() == HeldUmbrella::Loaner);
    // (The actual Ch3-entry clear is SceneRouter's SetHasUmbrella(false),
    // covered by the chapter-transition tests; here we only assert that NOT
    // returning leaves no karma debt and no returned-flag — i.e. it is a
    // no-op skip, not a soft-lock.)
    CHECK(w.GetPlayer()->GetKarma() == 50);
    CHECK_FALSE(w.GetPlayer()->HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
}
