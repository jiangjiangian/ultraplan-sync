/**
 * @file test_menu_help.cpp
 * @brief 驗證遊戲內暫停選單的「說明」項：透過 GameController 真實輸入迴圈確認
 *        說明會開啟就地的說明遮罩（而非觸發 AppAction）、可被 M/E/Enter 關回
 *        仍開著的選單、整段期間模擬凍結；以及說明文字的內容與分頁、暫停可延長
 *        雨壓力的提示，與 ←/→ 翻頁行為。
 */
//
// 遊戲內暫停選單新增了「說明」項。本檔固定 (a) 透過 GameController 真實輸入迴圈
// 確認選單順序中「說明」位於索引 1、(b) 說明會開啟就地的說明遮罩（World::HelpOpen）
// 而非觸發 AppAction、(c) 說明遮罩可被 M/E/Enter 關回仍開著的選單、(d) 整段期間
// 模擬凍結（不移動／雨量不前進）—— 故新增說明絕不會擾動遊玩或確定性的結局腳本。
// 後來選單列由 4 → 6（在索引 2、3 插入減少動畫／擴大目標切換列），但「說明」仍在
// 索引 1（本測試的不變量）。

#include "doctest/doctest.h"
#include "game/world/World.h"
#include "game/controller/GameController.h"
#include "game/entities/Player.h"
#include "ui/GameHelp.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include "engine/math/Vec2.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

using nccu::World;
using nccu::GameController;
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

void Frame(GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

// 暫停選單的「說明」會開啟／關閉說明遮罩，且整段期間模擬凍結。
TEST_CASE("REQ#9: pause menu 說明 opens/closes a help overlay, sim frozen") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);                       // 安頓（名冊等）
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const nccu::engine::math::Vec2 pos0 = p->GetPosition();
    const float rain0 = p->GetRainMeter();

    // 選單現在是 6 列（在說明與重新開始之間加入了減少動畫與擴大目標切換列）。
    // 本測試所固定的「說明」項仍在索引 1。
    CHECK(World::kMenuItemCount == 6);

    // 開啟暫停選單（M —— ESC 保留給離開程式）。
    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());
    CHECK(world.MenuCursor() == 0);              // 起始於繼續
    CHECK_FALSE(world.HelpOpen());

    // 移到索引 1（說明）並確認 → 說明遮罩開啟；選單仍在其後開著；未請求 AppAction。
    in.Tap(Key::Down);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 1);
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.HelpOpen());
    CHECK(world.MenuOpen());
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // 說明開著時模擬凍結：餵入移動鍵與多幀，玩家不可移動、雨量不可前進。
    for (int f = 0; f < 30; ++f) {
        in.Hold(Key::D);
        Frame(controller, in);
    }
    in.Release(Key::D);
    Frame(controller, in);
    CHECK(p->GetPosition().x == doctest::Approx(pos0.x));
    CHECK(p->GetPosition().y == doctest::Approx(pos0.y));
    CHECK(p->GetRainMeter() == doctest::Approx(rain0));
    CHECK(world.HelpOpen());                     // 仍開著 —— D 沒有關掉它

    // E 把說明關回選單（選單仍開著、游標保持不變）。
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK_FALSE(world.HelpOpen());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 1);

    // 此時從選單按 M 恢復遊戲（關閉選單也會透過 SetMenuOpen(false) 清掉任何
    // 說明鎖存）。
    in.Tap(Key::M);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());
    CHECK_FALSE(world.HelpOpen());

    // 說明文字非空，且短到能放進面板（最壞情況 ≤ 24 個全形字寬 —— 面板比對話框窄）。
    auto cells = [](std::string_view s) {
        // 計算 UTF-8 前導位元組數；此處中文以約 1 字寬作粗略上界，每行說明都是
        // 中英混合且刻意精簡。（精確的東亞寬度是對話框的事；說明面板較寬，只需
        // 一個合理上界。）
        int n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    };
    CHECK(nccu::kGameHelpLines.size() > 6);
    for (std::string_view ln : nccu::kGameHelpLines) CHECK(cells(ln) <= 24);
    CHECK_FALSE(nccu::kGameHelpClosing.empty());
    CHECK(cells(nccu::kGameHelpClosing) <= 24);

    // 把「暫停可暫停雨壓力」的提示固定進說明文字，讓玩家能發現「以暫停換取時間」
    // 的操作。此提示必須位於「單一行」；逐行檢查「同時含『暫停』與『雨壓力』」
    // 的斷言與按鍵無關（選單鍵已由 ESC 改為 M，但暫停操作不變）。日後改寫措辭但
    // 仍把兩個關鍵詞留在同一行仍會通過；只有刪除該提示（或拆成兩行）才會失敗。
    bool sawPauseRainHint = false;
    for (std::string_view ln : nccu::kGameHelpLines) {
        const bool hasPause = ln.find("暫停")   != std::string_view::npos;
        const bool hasRain  = ln.find("雨壓力") != std::string_view::npos;
        if (hasPause && hasRain) sawPauseRainHint = true;
    }
    CHECK(sawPauseRainHint);

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// 說明遮罩分成兩頁，且分頁內容與扁平的 kGameHelpLines 保持一致。
// 固定：(a) 兩頁、(b) 分頁視圖加收尾行等於扁平的 kGameHelpLines（使逐項走訪
// 扁平清單的字形掃描不致過時）、(c) 新的經濟／使用提示存在、(d) 每行都在面板的
// 字寬上限內。
TEST_CASE("U2-T4: GameHelp is split into two consistent pages") {
    CHECK(nccu::kGameHelpPageCount == 2);
    REQUIRE(nccu::kGameHelpPages.size() == 2);

    // 把各頁攤平，去掉結尾的收尾行（它是第 2 頁的最後一項，同時也保留為
    // kGameHelpClosing）。攤平後必須與扁平的 kGameHelpLines 逐項相等。
    std::vector<std::string_view> paged;
    for (const auto page : nccu::kGameHelpPages)
        for (std::string_view ln : page) paged.push_back(ln);
    REQUIRE_FALSE(paged.empty());
    CHECK(paged.back() == nccu::kGameHelpClosing);
    paged.pop_back();
    REQUIRE(paged.size() == nccu::kGameHelpLines.size());
    for (std::size_t i = 0; i < paged.size(); ++i)
        CHECK(paged[i] == nccu::kGameHelpLines[i]);

    // 每行都在約 24 個全形字寬（面板上限）內。
    auto cells = [](std::string_view s) {
        int n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    };
    for (const auto page : nccu::kGameHelpPages)
        for (std::string_view ln : page) CHECK(cells(ln) <= 24);

    // 道具相關提示應出現在說明中的某處。
    auto helpHas = [](std::string_view needle) {
        for (std::string_view ln : nccu::kGameHelpLines)
            if (ln.find(needle) != std::string_view::npos) return true;
        return false;
    };
    CHECK(helpHas("跨章節"));        // 金幣跨章節保留
    CHECK(helpHas("清空"));          // 其他道具在離開市集時清空
    CHECK(helpHas("減緩雨量"));      // 多數道具能減緩雨量
    CHECK(helpHas("自動減緩"));      // 傘會自動減緩雨量
}

// 說明遮罩開著時，←/→ 翻頁，索引會環繞，且（重新）開啟時重置到第 0 頁。
// 透過 GameController 真實輸入迴圈驅動以固定接線。頁碼是純 UI 狀態
// （World::HelpPage），不被序列化（工具不輸出游標／頁碼），故分頁說明讓
// state.jsonl 維持逐位元相同 —— 整段期間模擬同樣凍結。
TEST_CASE("U2-T4: ←/→ page the 說明 overlay; page resets on open, sim frozen") {
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::engine::input::Input::SetSource(&in);

    Frame(controller, in);                       // 安頓
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const nccu::engine::math::Vec2 pos0 = p->GetPosition();
    const float rain0 = p->GetRainMeter();

    // 開選單 → 說明（索引 1）→ Enter 開啟說明於第 0 頁。
    in.Tap(Key::M);          Frame(controller, in);
    in.Tap(Key::Down);       Frame(controller, in);
    in.Tap(Key::Enter);      Frame(controller, in);
    REQUIRE(world.HelpOpen());
    CHECK(world.HelpPage() == 0);                // 開於第一頁

    // → 前進到第 1 頁；再 → 環繞回 0（共 2 頁）。
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 1);
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 0);
    // ← 反方向環繞：0 → 最後一頁（1）。
    in.Tap(Key::Left);       Frame(controller, in);
    CHECK(world.HelpPage() == 1);

    // 整段翻頁期間模擬都維持凍結（不移動／雨量不前進）。
    CHECK(p->GetPosition().x == doctest::Approx(pos0.x));
    CHECK(p->GetPosition().y == doctest::Approx(pos0.y));
    CHECK(p->GetRainMeter() == doctest::Approx(rain0));

    // 關閉說明（E）再重開 → 回到第 0 頁（SetHelpOpen 會重置它）。
    in.Tap(Key::E);          Frame(controller, in);
    CHECK_FALSE(world.HelpOpen());
    in.Tap(Key::Enter);      Frame(controller, in);   // 游標仍在說明
    REQUIRE(world.HelpOpen());
    CHECK(world.HelpPage() == 0);

    // 翻到第 1 頁，再恢復遊戲（關閉說明後從選單按 M）—— SetMenuOpen(false)
    // 也必須清掉說明頁碼的鎖存。
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 1);
    in.Tap(Key::E);          Frame(controller, in);   // 說明 → 選單
    in.Tap(Key::M);          Frame(controller, in);   // 選單 → 遊戲
    CHECK_FALSE(world.MenuOpen());
    CHECK(world.HelpPage() == 0);                      // 恢復時重置

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
