/**
 * @file test_ending_menu.cpp
 * @brief 驗證結局畫面底部的三選項選單（回首頁 / 重新開始 / 結束）：純索引→選項
 *        對應，以及實際 GameController 接線 —— 結局狀態下世界凍結、←/→ 環狀移動
 *        游標、E/Enter 把游標對應到正確的 AppAction（回首頁/重新開始→Restart、
 *        結束→Quit）。
 */
//
// 結局畫面原本是個沒有互動的被動卡片；後來需求是「結局加三個左右選項」——
// 一個可用 ←/→ 選擇、E/Enter 確認的選單，能回到標題、重新開始全新遊戲、或真正
// 離開。本檔固定兩層：
//
//   (1) 純索引→選項對應（EndingMenuChoiceAt），與任何 World／渲染器無關 ——
//       使選單的語意可獨立測試。
//
//   (2) 實際的 GameController 接線：當學期處於結局狀態時世界凍結、結局選單按鍵
//       被消耗 —— ←/→ 環狀移動 World::EndingMenuCursor，E/Enter 把游標對應到
//       World::PendingAppAction。透過 controller 真實的輸入迴圈驅動（而非直接呼叫
//       setter），與 test_pause_menu_toggle 的破壞性列測試相同，故游標範圍與
//       游標→AppAction 的切換都被端到端地走過。

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/controller/GameController.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "ui/EndingView.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <cstdlib>
#include <set>

using nccu::GameController;
using nccu::World;
using nccu::EndingMenuChoice;
using nccu::engine::input::Key;

namespace {

// 與 test_pause_menu_toggle / test_menu_help 相同的最小 InputSource。
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
    void Tap(Key k) { Hold(k); autoUp_.insert(static_cast<int>(k)); }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) if (down_.erase(k)) released_.insert(k);
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

void Frame(GameController& c, TestInput& in) { c.Update(); in.EndFrame(); }

// 把 World 置入指定的結局狀態（判定邏輯由別處負責；選單只在意「我們處於結局」）。
void EnterEnding(World& w, nccu::SemesterState s) {
    w.Semester().Transition(s);
    REQUIRE(nccu::IsEndingState(w.Semester().Current()));
}

}  // namespace

// ---- (1) 純索引→選項對應 -----------------------------------------------
// EndingMenuChoiceAt 把 0/1/2 對應到 回首頁/重新開始/結束。
TEST_CASE("A-T3: EndingMenuChoiceAt maps 0/1/2 to Title/Restart/Quit") {
    CHECK(nccu::EndingMenuChoiceAt(0) == EndingMenuChoice::BackToTitle);
    CHECK(nccu::EndingMenuChoiceAt(1) == EndingMenuChoice::RestartGame);
    CHECK(nccu::EndingMenuChoiceAt(2) == EndingMenuChoice::Quit);
    // 超出範圍會夾進有效集合（絕不會「什麼都不是」）。
    CHECK(nccu::EndingMenuChoiceAt(3)  == EndingMenuChoice::BackToTitle);
    CHECK(nccu::EndingMenuChoiceAt(-1) == EndingMenuChoice::Quit);
    CHECK(nccu::World::kEndingMenuItemCount == 3);
    // 每個選項都有非空標籤（渲染器／字形掃描依賴它）。
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::BackToTitle).empty());
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::RestartGame).empty());
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::Quit).empty());
}

// ---- (2) 實際的 controller 接線 ----------------------------------------
// 在結局畫面，←/→ 會環狀移動游標。
TEST_CASE("A-T3: on the ending screen ←/→ move the cursor (modular)") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    EnterEnding(world, nccu::SemesterState::Ending_D);
    Frame(controller, in);                 // 在結局畫面安頓
    CHECK(world.EndingMenuCursor() == 0);  // 起始於回首頁

    in.Tap(Key::Right);
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 1);  // 重新開始

    in.Tap(Key::Right);
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 2);  // 結束

    in.Tap(Key::Right);                    // 環繞 2 -> 0
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 0);

    in.Tap(Key::Left);                     // 反方向環繞 0 -> 2
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 2);

    // 尚未按確認 ⇒ 未請求任何 app 動作。
    CHECK(world.PendingAppAction() == World::AppAction::None);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 結局選單的確認會把游標對應到正確的 AppAction。
TEST_CASE("A-T3: ending-menu confirm maps the cursor to the right AppAction") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    // 回首頁（游標 0）→ Restart（回到標題）。
    SUBCASE("回首頁 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::engine::input::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_A);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 0);
        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);
        nccu::engine::input::Input::SetSource(nullptr);
    }

    // 重新開始（游標 1）→ Restart（經由標題開始全新遊戲）。
    SUBCASE("重新開始 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::engine::input::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_B);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 1);
        in.Tap(Key::E);                    // E 也能確認（不只 Enter）
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);
        nccu::engine::input::Input::SetSource(nullptr);
    }

    // 結束（游標 2）→ Quit（唯一會關閉視窗的路徑）。
    SUBCASE("結束 → Quit") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::engine::input::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_C);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 2);
        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Quit);
        nccu::engine::input::Input::SetSource(nullptr);
    }

    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 結局畫面上世界凍結（無模擬、無移動）。
TEST_CASE("A-T3: the world is FROZEN on the ending screen (no sim, no movement)") {
    // 結局取代遊玩：即使按住移動鍵，玩家也不可移動，任意幀數都不可推進模擬
    // （在物件 tick／移動／掃描之前就提前返回）。這正是讓結局畫面成為穩定、
    // 只剩選單操作的畫面之關鍵。
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const auto before = p->GetPosition();

    EnterEnding(world, nccu::SemesterState::Ending_D);
    in.Hold(Key::D);                       // 按住「向右移動」
    for (int i = 0; i < 30; ++i) Frame(controller, in);
    const auto after = p->GetPosition();
    CHECK(after.x == doctest::Approx(before.x));   // 凍結 — 未移動
    CHECK(after.y == doctest::Approx(before.y));
    // 純粹的移動鍵不會請求任何 AppAction。
    CHECK(world.PendingAppAction() == World::AppAction::None);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
