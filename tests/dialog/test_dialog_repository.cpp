// DialogRepository（可注入、每個實例自有對話快取與內容目錄）取代了 DialogSource.cpp
// 內原本的檔案靜態可變狀態。自由函式 dialog::Entries / Reload / SetContentDir 行為
// 不變，因為它們委派給 dialog::Repository()——若有設定 SetRepository 覆寫則回傳該實例，
// 否則回傳行程預設實例。
//
// 本檔驗證：
//   1. 預設實例路徑與先前一致（既有呼叫端不受影響）。
//   2. SetRepository(myRepo) 會把 Entries() 導向該實例，SetRepository(nullptr)
//      則還原為預設。
//   3. 兩個內容目錄不同的 repository 提供不同的 Entries——證明每個實例自有快取與
//      內容目錄是真正解耦的（不只是改名）。

#include "doctest/doctest.h"
#include "game/dialog/DialogRepository.h"
#include "game/dialog/DialogSource.h"
#include "game/state/SemesterState.h"

#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

/**
 * @file test_dialog_repository.cpp
 * @brief 驗證 DialogRepository 的可注入設計：SetRepository(nullptr) 走預設實例、
 *        SetRepository(&repo) 改導 Entries() 到該實例、不同內容目錄的實例彼此快取
 *        隔離，且對某實例 Reload() 不會影響另一實例。
 */

using nccu::SemesterState;

// SetRepository(nullptr) 時 Entries() 走預設實例。
TEST_CASE("Phase 2.5: SetRepository(nullptr) routes through the default instance") {
    // 先還原預設，避免先前呼叫過 SetRepository 的案例外溢到這裡。
    nccu::dialog::SetRepository(nullptr);
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);

    // 預設路徑：Entries 應透過預設 repository 的快取拿到真正的章節內容。
    const auto& subs =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    // Ch1 助教區段至少有 (a)（申請書跑腿開場），所以 entries 不得為空。
    // 確切數量由 test_dialog_source 另外驗證；本案只證明這個接縫的回退可運作。
    CHECK_FALSE(subs.empty());
}

// SetRepository(&myRepo) 把 Entries 改導到該實例。
TEST_CASE("Phase 2.5: SetRepository(&myRepo) reroutes Entries to that instance") {
    nccu::dialog::SetRepository(nullptr);   // 從乾淨狀態開始

    // 建一個指向同一個 fixture 目錄的私有 repository 以便有內容可查詢。
    // 它的快取與 contentDir 與預設 repository 隔離。
    nccu::dialog::DialogRepository myRepo;
    myRepo.SetContentDir(TEST_CONTENT_DIR);

    nccu::dialog::SetRepository(&myRepo);
    REQUIRE(&nccu::dialog::Repository() == &myRepo);

    const auto& viaSeam =
        nccu::dialog::Entries("ta", SemesterState::Chapter1_AddDrop);
    const auto& viaDirect =
        myRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    // 同一實例 => 同一個 vector 位址。
    CHECK(&viaSeam == &viaDirect);

    // 還原給後續案例。
    nccu::dialog::SetRepository(nullptr);
}

// 每個實例的快取彼此隔離（不同內容目錄）。
TEST_CASE("Phase 2.5: per-instance caches are isolated (different content dirs)") {
    nccu::dialog::SetRepository(nullptr);

    // 兩個 repository——一個指向真正的 fixture，另一個指向不存在的目錄。
    // 即使 fixture 目錄的 repo 已快取了真實內容，無效目錄的 repo 仍必須回傳空；
    // 若快取間有外洩，這裡就會看到第二個 repo 提供第一個的內容。
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

    // fixture repo 的快取必須仍保有先前載入的內容——再次查詢會回傳同一個 vector
    // 參考（節點穩定的 map），且內容仍非空。
    const auto& fixtureSubsAgain =
        fixtureRepo.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK(&fixtureSubsAgain == &fixtureSubs);
    CHECK_FALSE(fixtureSubsAgain.empty());
}

// 對某個實例 Reload() 不會使另一個實例失效。
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

    // 只 Reload repoB。repoA 的參考必須存活（節點穩定的 map；reload 只清除
    // 這個實例自己的快取）。
    repoB.Reload();
    const auto& aAfter =
        repoA.Entries("ta", SemesterState::Chapter1_AddDrop);
    CHECK(&aAfter == &aBefore);

    // repoB 在下一次 Entries() 呼叫時會重新從磁碟讀取。
    const auto& bAfter =
        repoB.Entries("ta", SemesterState::Chapter1_AddDrop);
    // bAfter 與 bBefore 不一定共用位址（快取已清除，bBefore 在邏輯上已懸空——
    // 若要比較值，請先把內容複製出來）。這裡只斷言新查詢回傳非空 vector。
    (void)bBefore;
    CHECK_FALSE(bAfter.empty());
}
