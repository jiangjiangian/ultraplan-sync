/**
 * @file test_large_targets.cpp
 * @brief 驗證「擴大目標」無障礙設定（與減少動畫同樣的形態）：World 上的旗標
 *        預設值、setter 雙向切換、與減少動畫互相獨立，以及環境變數
 *        UMBRELLA_LARGE_TARGETS 的接線。
 */
//
// 三個相互獨立的面向：
//   (a) 預設：World::LargeTargets() 為 false（與先前行為等價，仍是每個測試所
//       依據的 8px 互動距離）。
//   (b) setter：SetLargeTargets(true) 雙向切換；開啟時把 E 互動探測距離放寬到
//       16 px（有效對話框 56x56 而非 40x40），但不改動移動碰撞體 —— 玩家仍無法
//       穿過 NPC，只是對話可及範圍變大。此處斷言的是「旗標狀態」而非重新推導
//       距離；距離常數由 GameController::Update / ScriptInput::ResolvePlan 依
//       同一旗標讀取，故旗標狀態即等同行為。
//   (c) 環境變數：UMBRELLA_LARGE_TARGETS=1 接進 World 建構子（引擎側觸發，
//       早於任何暫停選單 UI），與減少動畫的環境變數做法一致。

#include "doctest/doctest.h"
#include "game/world/World.h"
#include <cstdlib>

using nccu::World;

// 擴大目標：預設值與 setter 的雙向切換。
TEST_CASE("擴大目標無障礙設定：預設值與 setter") {
    SUBCASE("預設旗標為 false") {
        // 清除其他測試殘留的環境變數，使建構子讀到「未設定」→ 預設關閉。預設必須
        // 維持 false：每個測試與先前的試玩都以原本的 8px 距離為基準；改動預設會在
        // 無人選擇開啟的情況下破壞逐位元的確定性。
        unsetenv("UMBRELLA_LARGE_TARGETS");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.LargeTargets());
    }

    SUBCASE("setter 雙向翻轉旗標") {
        unsetenv("UMBRELLA_LARGE_TARGETS");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.LargeTargets());

        w.SetLargeTargets(true);
        CHECK(w.LargeTargets());

        // 未來的暫停選單 UI 也必須能再切回關閉 —— 它不是像 Flag_HasTrueUmbrella
        // 那種單向鎖存。
        w.SetLargeTargets(false);
        CHECK_FALSE(w.LargeTargets());
    }

    SUBCASE("旗標與 ReducedMotion 互相獨立（無交叉耦合）") {
        // 兩個無障礙旗標刻意設計為互相獨立的軸向 —— 手抖的玩家可能想要較大的
        // 目標、但仍希望雨景持續動畫；對閃光敏感的玩家可能想減少動畫、但不需要
        // 較大的目標。兩個 setter 不共用任何狀態；此處固定其獨立性，避免日後
        // 重構把它們混為一談。
        unsetenv("UMBRELLA_LARGE_TARGETS");
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.LargeTargets());
        REQUIRE_FALSE(w.ReducedMotion());

        w.SetLargeTargets(true);
        CHECK(w.LargeTargets());
        CHECK_FALSE(w.ReducedMotion());

        w.SetReducedMotion(true);
        CHECK(w.LargeTargets());                     // 不受影響
        CHECK(w.ReducedMotion());
    }
}

// 環境變數 UMBRELLA_LARGE_TARGETS=1 會在建構時開啟旗標。
TEST_CASE("UMBRELLA_LARGE_TARGETS=1 會在建構時開啟旗標") {
    // getenv 讀取現位於 ReadWorldOptionsFromEnv() —— main.cpp 呼叫它一次，再把
    // 解析後的 WorldOptions 傳給 World 建構子。測試同時驗證兩半：環境變數讀取器
    // 是否正確解讀，以及 World 建構子是否遵循其結果。
    SUBCASE("env=1 開啟旗標") {
        setenv("UMBRELLA_LARGE_TARGETS", "1", /*overwrite=*/1);
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK(opts.largeTargets);
        World w("", /*loadSprites=*/false, opts);
        CHECK(w.LargeTargets());
        unsetenv("UMBRELLA_LARGE_TARGETS");
    }

    SUBCASE("env=0 維持旗標關閉（只有 '1' 才啟用）") {
        // 與 UMBRELLA_REDUCED_MOTION 相同 —— 只接受字面 "1"，故未設定／"0"／
        // "true"／空字串都維持預設 false，排除來自殘留環境變數的誤啟用。
        setenv("UMBRELLA_LARGE_TARGETS", "0", /*overwrite=*/1);
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK_FALSE(opts.largeTargets);
        World w("", /*loadSprites=*/false, opts);
        CHECK_FALSE(w.LargeTargets());
        unsetenv("UMBRELLA_LARGE_TARGETS");
    }

    SUBCASE("env 未設定 → 旗標維持關閉") {
        unsetenv("UMBRELLA_LARGE_TARGETS");
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK_FALSE(opts.largeTargets);
        World w("", /*loadSprites=*/false, opts);
        CHECK_FALSE(w.LargeTargets());
    }
}
