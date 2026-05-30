// 驗證兩個「跳過」操作（無障礙：可暫停、停止、隱藏自動更新內容）：
//
//   (a) 長按 E 快速推進對話。原本的邊緣觸發（連點 E）路徑保留；長按 E 達
//       300 ms 後，會以約 4 幀一次的慢速節流自動推進，讓囉嗦的 NPC 可被略讀
//       而不傷手指。
//   (b) 退格鍵關閉螢幕上的 HUD 提示。立即強制讓橫幅過期，已讀完該行的玩家
//       不必空等 4 秒的 kHudTtl。
//
// 為此在 nccu::engine::input::Key 新增了 Backspace 鍵；World 新增 DismissHud()
// 把 hudAge_ 直接跳到 kHudTtl（與 HudExpired() / MessageView 提早返回所依據的
// 邊界相同，因此算繪行為不變——下一個 View 階段就只是不畫任何東西）。

#include "doctest/doctest.h"
#include "game/world/World.h"
#include "game/controller/GameController.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "ui/MessageView.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include "engine/math/Vec2.h"

#include <set>
#include <string>
#include <vector>

using nccu::GameController;
using nccu::World;
using nccu::engine::input::Key;

/**
 * @file test_dialog_skip.cpp
 * @brief 驗證兩個跳過操作：長按 E 超過 300 ms 自動快速推進對話（且連點 E 仍可用、
 *        放開會重置計時），以及退格鍵立即關閉 HUD 提示（無提示時為空操作、且只認
 *        退格鍵）。
 */

namespace {

// 一個輕量、手動驅動的 InputSource，讓我們能把按鍵邊緣與真實裝置解耦。
// Hold(k) 設定 IsDown 為 true（並在首次轉態時送出一幀 IsPressed）；EndFrame()
// 清掉每幀的 pressed／released 集合，但保留 IsDown。Tap(k) 先按住，再於下一次
// EndFrame 自動放開，模擬單幀按下。
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

void Frame(GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

// 長按 E 超過 300 ms 會快速推進對話；連點 E（邊緣觸發）仍然可用。
TEST_CASE("長按 E 超過 300 ms 快速推進對話；連點 E 仍然可用") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);    // 固定 60 fps 以求決定性
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    // 以程式直接開啟多行對話——World 直接擁有 DialogState，因此不必走到某個
    // NPC。六行足以觀察按住門檻觸發後自動推進跨越多頁。
    std::vector<std::string> lines{"L0", "L1", "L2", "L3", "L4", "L5"};
    world.Dialog().Open(lines);
    REQUIRE(world.Dialog().Active());
    CHECK(world.Dialog().CurrentLine() == "L0");

    SUBCASE("連點 E 每次仍只推進一步（保留連點路徑）") {
        in.Tap(Key::E);
        Frame(controller, in);                       // L0 -> L1
        CHECK(world.Dialog().Active());
        CHECK(world.Dialog().CurrentLine() == "L1");

        // 沒有輸入的一幀不會推進——按住 E 的分支關閉（IsDown 為 false）。
        // 這固定了先前的「只認單點」語意：按與按之間不會殘留按住計時。
        Frame(controller, in);
        CHECK(world.Dialog().CurrentLine() == "L1");

        in.Tap(Key::E);
        Frame(controller, in);                       // L1 -> L2
        CHECK(world.Dialog().CurrentLine() == "L2");
    }

    SUBCASE("長按 E 達 300 ms 後在幀守門下自動推進") {
        // 第 0 幀：按下 E（邊緣觸發）-> L0 推進到 L1。第一幀的按下走既有的邊緣
        // 路徑；按住計時從這一幀起開始累積（IsDown 為 true）。
        in.Hold(Key::E);
        Frame(controller, in);                       // 邊緣：L0 -> L1
        CHECK(world.Dialog().CurrentLine() == "L1");

        // 再按住 10 幀（共約 167 ms）——遠低於 300 ms 門檻。不會再推進，仍在 L1。
        //（選 10 是為了即使考慮 1/60 秒 tick 的浮點捨入，也能穩穩低於門檻。）
        for (int f = 0; f < 10; ++f) {
            Frame(controller, in);
        }
        CHECK(world.Dialog().CurrentLine() == "L1");

        // 再按住約 40 幀（共約 833 ms）後，自動推進分支已在其約 4 幀的冷卻下
        // 觸發數次——至少一定一次，跨越 L1。測試斷言可觀察的結果：已不在 L1。
        for (int f = 0; f < 40; ++f) {
            Frame(controller, in);
        }
        CHECK((world.Dialog().CurrentLine() != "L1"));

        in.Release(Key::E);
        Frame(controller, in);
    }

    SUBCASE("放開 E 重置按住計時；不會跨次按下累積") {
        // 按住 10 幀（約 167 ms），放開，再按住——計時必須從 0 重新開始，
        // 不可接續先前的累積。
        in.Hold(Key::E);
        Frame(controller, in);                       // 邊緣：L0 -> L1
        for (int f = 0; f < 9; ++f) {
            Frame(controller, in);
        }
        CHECK(world.Dialog().CurrentLine() == "L1");

        in.Release(Key::E);
        Frame(controller, in);                       // 計時重置

        // 再按住僅 10 幀——若計時外洩，累積按住會達 20 幀以上，但我們預期沒有
        // 任何自動推進，因為單獨每次按下都 < 300 ms。
        in.Hold(Key::E);
        Frame(controller, in);                       // 邊緣：L1 -> L2
        for (int f = 0; f < 8; ++f) {
            Frame(controller, in);
        }
        // 仍在 L2——放開後按住計時從未跨過 300 ms，因此沒有自動推進觸發。
        CHECK(world.Dialog().CurrentLine() == "L2");
        in.Release(Key::E);
        Frame(controller, in);
    }

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 退格鍵強制讓 HUD 提示過期（跳過提示）。
TEST_CASE("退格鍵強制讓 HUD 提示過期（跳過提示）") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    SUBCASE("對新提示按退格鍵把 HudAge 直接跳到 kHudTtl") {
        world.SetHudMessage("transient banner");
        REQUIRE_FALSE(world.HudMessage().empty());
        REQUIRE(world.HudAge() == doctest::Approx(0.0f));
        REQUIRE_FALSE(world.HudExpired());

        in.Tap(Key::Backspace);
        Frame(controller, in);

        // 退格鍵的路徑必須把年齡直接跳到 kHudTtl——其守門 HudExpired() 現在為
        // true。MessageView::Draw 與 harness 的 HudExpired 路徑都在同一個邊界
        // 提早返回，因此此關閉對 View 可觀察。
        CHECK(world.HudExpired());
        CHECK(world.HudAge() >= nccu::kHudTtl);
    }

    SUBCASE("無提示時按退格鍵是空操作（不產生多餘改動）") {
        REQUIRE(world.HudMessage().empty());
        const float ageBefore = world.HudAge();

        in.Tap(Key::Backspace);
        Frame(controller, in);

        // hudMessage_ 維持空；年齡原本為 0（沒有訊息可計時），且 DismissHud 的
        // 守門拒絕寫入空提示。
        CHECK(world.HudMessage().empty());
        CHECK(world.HudAge() == doctest::Approx(ageBefore));
        CHECK_FALSE(world.HudExpired());
    }

    SUBCASE("關閉只由退格鍵觸發——Enter／E 不會關閉") {
        world.SetHudMessage("transient banner");
        REQUIRE_FALSE(world.HudExpired());

        in.Tap(Key::E);
        Frame(controller, in);
        CHECK_FALSE(world.HudExpired());

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK_FALSE(world.HudExpired());

        // 現在退格鍵終於關閉提示。
        in.Tap(Key::Backspace);
        Frame(controller, in);
        CHECK(world.HudExpired());
    }

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
