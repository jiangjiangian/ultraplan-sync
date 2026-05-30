/**
 * @file test_reduced_motion.cpp
 * @brief 驗證「減少動畫」無障礙偏好：World 上的旗標與三個純動畫閘函式
 *        （幕間地標流動、結局卡淡入、HUD 提示淡出）在開啟時改為直接到位，
 *        以及預設值、setter、環境變數 UMBRELLA_REDUCED_MOTION 的接線。
 */
//
// 三個相互獨立的面向：
//   (a) 預設：World::ReducedMotion() 為 false（與先前行為等價）。
//   (b) 切換：SetReducedMotion(true) 使三個動畫閘各自改變 —— 幕間地標停止流動、
//       結局卡 alpha 在首次繪製即跳到 1.0、HUD 提示淡出改為硬切（整個生命週期
//       係數皆 1.0，到 TTL 才提前返回）。
//   (c) 環境變數：UMBRELLA_REDUCED_MOTION=1 接進 World 建構子（引擎側觸發，
//       早於暫停選單 UI）。

#include "doctest/doctest.h"
#include "ui/ReducedMotion.h"
#include "game/world/World.h"
#include <cstdlib>

using nccu::EndingFadeAlphaStep;
using nccu::HudToastFadeT;
using nccu::InterludeMarkerPhaseStep;
using nccu::World;

// 減少動畫：預設值與三個動畫閘函式的開關行為。
TEST_CASE("減少動畫：預設值與三個動畫閘函式的開關行為") {
    SUBCASE("預設旗標為 false") {
        // 清除前一個測試行程殘留的環境變數，使建構子讀到「未設定」→ 預設關閉。
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.ReducedMotion());
    }

    SUBCASE("setter 雙向翻轉旗標") {
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.ReducedMotion());

        w.SetReducedMotion(true);
        CHECK(w.ReducedMotion());

        w.SetReducedMotion(false);
        CHECK_FALSE(w.ReducedMotion());
    }

    SUBCASE("開啟減少動畫時 InterludeMarkerPhaseStep 原地凍結") {
        // dt 為 1/60 秒時，正常會前進約 0.5 px（30 px/s * 1/60）。開啟減少動畫後
        // 步進為 0.0 — 虛線流動原地停止（呼叫端仍會繪製地標，只是抑制動畫）。
        constexpr float dt = 1.0f / 60.0f;
        CHECK(InterludeMarkerPhaseStep(dt, false) > 0.0f);
        CHECK(InterludeMarkerPhaseStep(dt, true)  == doctest::Approx(0.0f));
    }

    SUBCASE("開啟減少動畫時 EndingFadeAlphaStep 立即跳到 1.0") {
        // 預設路徑在累積約一秒的 dt 內由 0 漸增到 1。減少動畫路徑在第一次呼叫即
        // 回傳 1.0，跳過約半秒的亮度漸變。
        constexpr float dt = 1.0f / 60.0f;
        const float ramp   = EndingFadeAlphaStep(0.0f, dt, false);
        CHECK(ramp > 0.0f);
        CHECK(ramp < 1.0f);                // 漸變途中，未跳到位
        CHECK(EndingFadeAlphaStep(0.0f, dt, true) == doctest::Approx(1.0f));
        // 即使從漸變途中的數值，減少動畫路徑仍會直接跳到位。
        CHECK(EndingFadeAlphaStep(0.4f, dt, true) == doctest::Approx(1.0f));
    }

    SUBCASE("開啟減少動畫時 HudToastFadeT 維持全不透明（到 TTL 才硬切）") {
        // 預設路徑：kHudFade = 1.0 時，淡出視窗過半處係數約為 0.5（漸變途中）。
        const float ramp = HudToastFadeT(0.5f, 1.0f, false);
        CHECK(ramp > 0.4f);
        CHECK(ramp < 0.6f);
        // 減少動畫：維持全不透明，直到呼叫端的 TTL 提前返回。
        CHECK(HudToastFadeT(0.5f, 1.0f, true) == doctest::Approx(1.0f));
        CHECK(HudToastFadeT(0.0f, 1.0f, true) == doctest::Approx(1.0f));
    }
}

// 環境變數 UMBRELLA_REDUCED_MOTION=1 會在建構時開啟旗標。
TEST_CASE("減少動畫：UMBRELLA_REDUCED_MOTION=1 會在建構時開啟旗標") {
    // getenv 移到 ReadWorldOptionsFromEnv()，World 相對其引數即為純函式。
    // 測試同時驗證兩半：環境變數讀取器 + 建構子是否遵循其結果。
    SUBCASE("env=1 在建構時開啟旗標") {
        setenv("UMBRELLA_REDUCED_MOTION", "1", /*overwrite=*/1);
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK(opts.reducedMotion);
        World w("", /*loadSprites=*/false, opts);
        CHECK(w.ReducedMotion());
        unsetenv("UMBRELLA_REDUCED_MOTION");  // 還原，避免影響其他測試
    }

    SUBCASE("env=0 維持旗標關閉（只有 '1' 才啟用）") {
        setenv("UMBRELLA_REDUCED_MOTION", "0", /*overwrite=*/1);
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK_FALSE(opts.reducedMotion);
        World w("", /*loadSprites=*/false, opts);
        CHECK_FALSE(w.ReducedMotion());
        unsetenv("UMBRELLA_REDUCED_MOTION");
    }

    SUBCASE("env 未設定 → 旗標維持關閉") {
        unsetenv("UMBRELLA_REDUCED_MOTION");
        const nccu::WorldOptions opts = nccu::ReadWorldOptionsFromEnv();
        CHECK_FALSE(opts.reducedMotion);
        World w("", /*loadSprites=*/false, opts);
        CHECK_FALSE(w.ReducedMotion());
    }
}
