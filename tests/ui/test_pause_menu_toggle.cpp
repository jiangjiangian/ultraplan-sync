/**
 * @file test_pause_menu_toggle.cpp
 * @brief 驗證暫停選單對「減少動畫 / 擴大目標」兩個無障礙旗標的 UI 接線：
 *        透過 GameController 真實輸入迴圈，確認 6 列選單的游標環狀走訪、
 *        兩個切換列 Enter 就地翻轉對應旗標且互相獨立、可往復切換，以及位移後的
 *        破壞性列（重新開始 / 離開）仍對應正確的 AppAction；另含選單只開於 M 不開
 *        於 ESC。
 */
//
// World 旗標與環境變數掛鉤已在先前加入，但玩家仍沒有非環境變數的方式可切換它們；
// 此處把暫停選單兩個新切換列的 Enter 接到 World::SetReducedMotion /
// SetLargeTargets，與既有的輸入迴圈相同。透過 GameController 真實輸入迴圈
// （而非直接呼叫 world.SetMenuCursor）固定以下，使游標範圍由 4 → 6 列與切換列的
// 動作都被端到端地走過：
//
//   (a) kMenuItemCount 為 6，且 ↓ ↑ 走訪完整的 0..5 範圍並環狀環繞（新邊界處
//       無差一錯誤）。
//   (b) 在第 2 列按 Enter 翻轉 World::ReducedMotion()（選單保持開啟、游標停在
//       第 2 列，使 [開]/[關] 標籤變化對玩家可見）。
//   (c) 在第 3 列按 Enter 以相同的就地語意翻轉 World::LargeTargets()；兩個旗標
//       相互獨立（切換其一不影響另一 —— 與 test_large_targets 在原始 setter 上
//       固定的獨立性相同）。
//   (d) 兩個切換都可往復：在同一列再按一次 Enter 會還原先前狀態（故游標不會卡死）。

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/controller/GameController.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <cstdlib>
#include <set>

using nccu::GameController;
using nccu::World;
using nccu::engine::input::Key;

namespace {

// 與 test_menu_help.cpp 相同的 TestInput —— GameController 經由 Input 靜態介面
// 查詢的最小 InputSource。Tap() 會為某一幀排入一次邊緣觸發的 IsPressed；每次
// c.Update() 之後都必須呼叫 EndFrame()，自動釋放才會執行。
class TestInput final : public nccu::engine::input::InputSource {
public:
    void Hold(Key k) {
        if (down_.insert(static_cast<int>(k)).second)
            pressed_.insert(static_cast<int>(k));
    }
    void Release(Key k) {
        if (down_.erase(static_cast<int>(k)))
            released_.insert(static_cast<int>(k));
    }
    void Tap(Key k) {
        Hold(k);
        autoUp_.insert(static_cast<int>(k));
    }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) {
            if (down_.erase(k)) released_.insert(k);
        }
        autoUp_.clear();
    }
    bool IsDown(Key k) const noexcept override {
        return down_.count(static_cast<int>(k)) != 0;
    }
    bool IsPressed(Key k) const noexcept override {
        return pressed_.count(static_cast<int>(k)) != 0;
    }
    bool IsReleased(Key k) const noexcept override {
        return released_.count(static_cast<int>(k)) != 0;
    }
private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

void Frame(GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

// 暫停選單現有 6 列，游標環繞於 0..5。
TEST_CASE("暫停選單現有 6 列，游標環繞於 0..5") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);                         // 安頓
    CHECK(World::kMenuItemCount == 6);

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());
    CHECK(world.MenuCursor() == 0);

    // 往下走五次 —— 游標必須命中 1..5 的每個列索引。
    for (int expected = 1; expected <= 5; ++expected) {
        in.Tap(Key::Down);
        Frame(controller, in);
        CHECK(world.MenuCursor() == expected);
    }

    // 再往下一次環繞到 0（環狀的 MoveMenuCursor）。
    in.Tap(Key::Down);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 0);

    // 從 0 往上環繞到 5（同一模數的另一方向）。
    in.Tap(Key::Up);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 5);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 在第 2 列按 Enter 會翻轉 World.ReducedMotion()。
TEST_CASE("在第 2 列按 Enter 會翻轉 World.ReducedMotion()") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);
    REQUIRE_FALSE(world.ReducedMotion());

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // 走到第 2 列（減少動畫）。
    in.Tap(Key::Down);
    Frame(controller, in);
    in.Tap(Key::Down);
    Frame(controller, in);
    REQUIRE(world.MenuCursor() == 2);

    // Enter 把 ReducedMotion 打開；選單保持開啟；游標不變。
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.ReducedMotion());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 2);
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // 再按一次 Enter 把它關回去 —— 往復切換，無鎖存。
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK_FALSE(world.ReducedMotion());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 2);

    // 全程 LargeTargets 必須維持預設 —— 兩個切換相互獨立。
    CHECK_FALSE(world.LargeTargets());

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 在第 3 列按 Enter 會翻轉 World.LargeTargets()。
TEST_CASE("在第 3 列按 Enter 會翻轉 World.LargeTargets()") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);
    REQUIRE_FALSE(world.LargeTargets());

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // 走到第 3 列（擴大目標）。
    for (int i = 0; i < 3; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    REQUIRE(world.MenuCursor() == 3);

    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.LargeTargets());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 3);
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // 再按一次 Enter 關回去。
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK_FALSE(world.LargeTargets());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 3);

    // 全程 ReducedMotion 必須維持預設 —— 兩個切換相互獨立。
    CHECK_FALSE(world.ReducedMotion());

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 列位移後，破壞性列仍對應正確的 AppAction。
TEST_CASE("列位移後破壞性列仍正確對應 AppAction") {
    // 新的 6 列順序把重新開始由索引 2 → 4、離開由 3 → 5。兩者都固定：第 4 列的
    // 重新開始必須請求 AppAction::Restart（非 None、非 Quit）；第 5 列的離開必須
    // 請求 AppAction::Quit。純屬意圖 —— 實際動作由 main.cpp 外層迴圈執行；此處只
    // 驗證 controller 從正確索引路由出正確列舉。
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    SUBCASE("第 4 列 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::engine::input::Input::SetSource(&in);
        Frame(controller, in);

        in.Tap(Key::M);
        Frame(controller, in);
        for (int i = 0; i < 4; ++i) {
            in.Tap(Key::Down);
            Frame(controller, in);
        }
        REQUIRE(world.MenuCursor() == 4);

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);

        // 切換旗標未受影響。
        CHECK_FALSE(world.ReducedMotion());
        CHECK_FALSE(world.LargeTargets());

        nccu::engine::input::Input::SetSource(nullptr);
    }

    SUBCASE("第 5 列 → Quit") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::engine::input::Input::SetSource(&in);
        Frame(controller, in);

        in.Tap(Key::M);
        Frame(controller, in);
        for (int i = 0; i < 5; ++i) {
            in.Tap(Key::Down);
            Frame(controller, in);
        }
        REQUIRE(world.MenuCursor() == 5);

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Quit);

        nccu::engine::input::Input::SetSource(nullptr);
    }

    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 選單只開於 M，絕不開於 ESC（ESC 是程式的離開鍵）。
TEST_CASE("選單只開於 M，絕不開於 ESC（ESC 是程式的離開鍵）") {
    // 暫停選單原本綁在 ESC 上切換。但 ESC 是 raylib 預設的離開鍵，main.cpp 的
    // WindowShouldClose 迴圈會因它而離開 —— 把選單綁在 ESC 也會在視窗關閉前閃現
    // 選單一幀。選單現在改在 M；GameController 從不讀取 ESC，故凍結中的 Update
    // 視其為無操作，ESC 在實際程式中唯一的角色就是由 main.cpp 負責的乾淨離開。
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                         // 安頓

    // ESC 不可開啟選單。
    in.Tap(Key::Escape);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());

    // M 開啟它。
    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // ESC 也不可關閉它（不會以 ESC 恢復遊戲）。
    in.Tap(Key::Escape);
    Frame(controller, in);
    CHECK(world.MenuOpen());

    // M 把它切換回關閉。
    in.Tap(Key::M);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
