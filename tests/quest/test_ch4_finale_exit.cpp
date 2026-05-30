/**
 * @file test_ch4_finale_exit.cpp
 * @brief 驗證 Ch4 助教結算選單的退出項：選退出零副作用、不上自鎖旗標，且選單之後仍可重新開啟。
 */
#include "game/quest/Flags.h"
// Ch4 助教結算選單必須帶有一個不提交的退出項。選它會以零狀態變動關閉對話
//（不設 Flag_TaFinaleChoiceMade、不設 Flag_ConsoledTA、不套用 karma），
// 玩家因此可以先走開、稍後再回來找助教決定結局。這等同於 vendor 的「先不買」
// 拒絕，也是防卡關的閘門：結算選單會以 Flag_TaFinaleChoiceMade 自鎖，
// 一旦誤觸提交就會永久關閉 Ending A 的可能。
//
// 透過真正的 GameController::Update() 迴圈、經由輸入層這個咽喉點來驅動
//（即正式版確認路徑），而非單元墊片，以確保涵蓋實際的確認分支。

#include "doctest/doctest.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::engine::input::Key;

namespace {

class TestInput final : public nccu::engine::input::InputSource {
public:
    void Hold(Key k)    { if (down_.insert(static_cast<int>(k)).second) pressed_.insert(static_cast<int>(k)); }
    void Release(Key k) { if (down_.erase(static_cast<int>(k)))         released_.insert(static_cast<int>(k)); }
    void Tap(Key k)     { Hold(k); autoUp_.insert(static_cast<int>(k)); }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) { if (down_.erase(k)) released_.insert(k); }
        autoUp_.clear();
    }
    bool IsDown(Key k)     const noexcept override { return down_.count(static_cast<int>(k)) != 0; }
    bool IsPressed(Key k)  const noexcept override { return pressed_.count(static_cast<int>(k)) != 0; }
    bool IsReleased(Key k) const noexcept override { return released_.count(static_cast<int>(k)) != 0; }
private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// 連按 E，直到對話進入選擇模式（開場台詞播完）。
void AdvanceToChoice(nccu::GameController& c, TestInput& in, World& w) {
    for (int f = 0; f < 24 && !w.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(c, in);
    }
}

}  // namespace

// 對助教結局選退出（我再想想…）不應改動任何狀態，且之後仍可重新開啟相同選單。
TEST_CASE("拒絕助教結局（我再想想…）不改動任何狀態且可重新開啟") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);
    Frame(controller, in);                              // 讓 Ch4 名冊就緒

    const GameObject* ta = FindNpc(world, "ta");
    REQUIRE(ta != nullptr);
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);

    // 前置基準：高 karma + Ch4 重新取得的真傘，使得「體諒」提交本可抵達
    // Ending A——藉此證明退出是真的把該結局擋下，而非只是條件本來就不滿足。
    p->AddKarma(40);                                    // 約 90
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    const int karma0 = p->GetKarma();
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagTaFinaleChoiceMade));
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagConsoledTA));

    // 走到助教身邊並開啟結算選單。
    p->SetPosition(nccu::engine::math::Vec2{ta->GetPosition().x - 8.0f,
                                   ta->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    AdvanceToChoice(controller, in, world);
    REQUIRE(world.Dialog().AtChoice());

    // 選單為 體諒 / 質問 / 退出——退出固定在最後。
    const std::size_t n = world.Dialog().Choices().size();
    REQUIRE(n == 3);
    CHECK(world.Dialog().Choices().back().label == nccu::kDialogExitLabel);

    // 把游標往下移到退出項並確認。
    for (std::size_t i = 0; i + 1 < n; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    CHECK(world.Dialog().ChoiceCursor() == static_cast<int>(n) - 1);
    in.Tap(Key::E);                                     // 確認「退出」
    Frame(controller, in);

    // 沒有任何提交：對話關閉、結局未定案、karma 未動、Ending A 未觸發，主幹仍在 Ch4。
    CHECK_FALSE(world.Dialog().Active());
    CHECK_FALSE(p->HasFlag(nccu::kFlagTaFinaleChoiceMade));
    CHECK_FALSE(p->HasFlag(nccu::kFlagConsoledTA));
    CHECK(p->GetKarma() == karma0);
    CHECK(world.Semester().Current() == SemesterState::Chapter4_Finals);

    // 而且結局仍可重新接觸——再次開啟時，相同的三個選項會回來，代表這個決定只是被延後。
    p->SetPosition(nccu::engine::math::Vec2{ta->GetPosition().x - 8.0f,
                                   ta->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    AdvanceToChoice(controller, in, world);
    REQUIRE(world.Dialog().AtChoice());
    CHECK(world.Dialog().Choices().size() == 3);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
