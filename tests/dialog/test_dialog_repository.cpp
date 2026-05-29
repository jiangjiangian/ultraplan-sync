// Blueprint Phase 2.5 regression — DialogRepository (the injectable
// per-instance dialog cache + content-dir owner) replaces the pre-Phase
// file-static mutables in DialogSource.cpp. The free functions
// dialog::Entries / Reload / SetContentDir keep working unchanged
// because they delegate to dialog::Repository() — which returns the
// SetRepository override if set, else the process default.
//
// What this test pins:
//   1. The default-instance path still works the same as the
//      pre-Phase Meyers-singleton (existing callers untouched).
//   2. SetRepository(myRepo) routes Entries() to that instance,
//      and SetRepository(nullptr) restores the default.
//   3. Two repositories with DIFFERENT content dirs serve DIFFERENT
//      Entries — proves the per-instance cache + content-dir
//      decoupling is real (not just a rename).
//
// Revert-verify (must FAIL if Phase 2.5 is unwound):
//   * Re-merge cache_ / contentDir_ back into a file-static Cache() /
//     ContentDir(): the two-instance isolation case fails because
//     the SECOND repo's SetContentDir overrides the global, so the
//     FIRST repo (using a stale cache key tied to the global) serves
//     the new dir's content. The default-path case still passes.

#include "doctest/doctest.h"
#include "game/dialog/DialogRepository.h"
#include "game/dialog/DialogSource.h"
#include "game/state/SemesterState.h"

#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

TEST_CASE("Phase 2.5: SetRepository(nullptr) routes through the default instance") {
    // Restore the default first so prior cases that called
    // SetRepository can't leak into this one.
    nccu::dialog::SetRepository(nullptr);
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // Default path: Entries should reach the real chapter content via
    // the default repository's cache.
    const auto& subs =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    // The Ch1 助教 section ships at minimum (a) (申請書 errand opener),
    // so the entries vector must NOT be empty. The exact count is
    // pinned by test_dialog_source separately; this case only proves
    // the seam fall-through works.
    CHECK_FALSE(subs.empty());
}

TEST_CASE("Phase 2.5: SetRepository(&myRepo) reroutes Entries to that instance") {
    nccu::dialog::SetRepository(nullptr);   // start clean

    // Build a private repository pointed at the same fixture dir so
    // we have content to query. Its cache + contentDir are ISOLATED
    // from the default repository's.
    nccu::dialog::DialogRepository myRepo;
    myRepo.SetContentDir(TEST_CONTENT_DIR);

    nccu::dialog::SetRepository(&myRepo);
    REQUIRE(&nccu::dialog::Repository() == &myRepo);

    const auto& viaSeam =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    const auto& viaDirect =
        myRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    // Same instance => same vector address.
    CHECK(&viaSeam == &viaDirect);

    // Restore for sibling cases.
    nccu::dialog::SetRepository(nullptr);
}

TEST_CASE("Phase 2.5: per-instance caches are isolated (different content dirs)") {
    nccu::dialog::SetRepository(nullptr);

    // Two repositories — one points at the real fixture, the other at
    // a nonsense dir. The nonsense-dir repo MUST yield empty entries
    // even when the fixture-dir repo has already cached real content;
    // a leak between caches would surface here as the second repo
    // serving the first's content.
    nccu::dialog::DialogRepository fixtureRepo;
    fixtureRepo.SetContentDir(TEST_CONTENT_DIR);
    const auto& fixtureSubs =
        fixtureRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(fixtureSubs.empty());

    nccu::dialog::DialogRepository emptyRepo;
    emptyRepo.SetContentDir("/no/such/dir/p25-isolation");
    const auto& emptySubs =
        emptyRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK(emptySubs.empty());

    // The fixture repo's cache must still hold its earlier load —
    // a re-query returns the SAME vector reference (node-stable
    // map) and the content is still non-empty.
    const auto& fixtureSubsAgain =
        fixtureRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK(&fixtureSubsAgain == &fixtureSubs);
    CHECK_FALSE(fixtureSubsAgain.empty());
}

TEST_CASE("Phase 2.5: Reload() on one instance does not invalidate another") {
    nccu::dialog::SetRepository(nullptr);

    nccu::dialog::DialogRepository repoA;
    repoA.SetContentDir(TEST_CONTENT_DIR);
    const auto& aBefore =
        repoA.Entries("ta", SemesterState::Chapter1_AddDrop);

    nccu::dialog::DialogRepository repoB;
    repoB.SetContentDir(TEST_CONTENT_DIR);
    const auto& bBefore =
        repoB.Entries("ta", SemesterState::Chapter1_AddDrop);

    // Reload only repoB. repoA's reference must SURVIVE (node-stable
    // map; reloads only clear THIS instance's cache).
    repoB.Reload();
    const auto& aAfter =
        repoA.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK(&aAfter == &aBefore);

    // repoB re-reads from disk on the next Entries() call.
    const auto& bAfter =
        repoB.Entries("ta", SemesterState::Chapter1_AddDrop);
    // bAfter and bBefore may or may not share an address (the cache
    // was cleared, so bBefore is dangling logically — capture
    // contents into a copy if anyone needs to compare values). We
    // only assert that the new query returns a non-empty vector.
    (void)bBefore;
    CHECK_FALSE(bAfter.empty());
}
