#include "doctest/doctest.h"
#include "game/controller/InputHandler.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include <cstdint>

/**
 * @file test_input_handler.cpp
 * @brief 驗證 InputHandler 從 GameController 抽出的 E 鍵 edge／長按計時行為：
 *        單格 tap 觸發、長按未達門檻不自動推進、長按超過門檻後依冷卻節奏自動推進、
 *        放開重置計時，以及 edge 與 auto 不會在同一格重複觸發。
 */

namespace {

// 最小的 InputSource stub，讓測試注入確定性的按鍵狀態，無需啟動真正的 raylib 堆疊或
// harness 的 ScriptInput。介面與 LiveInput / ScriptInput 相同，故行為與遊戲路徑等價。
class StubInput final : public nccu::engine::input::InputSource {
public:
    bool IsDown(nccu::engine::input::Key k) const noexcept override {
        return down_ & Bit(k);
    }
    bool IsPressed(nccu::engine::input::Key k) const noexcept override {
        return pressed_ & Bit(k);
    }
    bool IsReleased(nccu::engine::input::Key k) const noexcept override {
        return released_ & Bit(k);
    }

    void Hold(nccu::engine::input::Key k)    { down_ |= Bit(k); }
    void Release(nccu::engine::input::Key k) { down_ &= ~Bit(k); released_ |= Bit(k); }
    void Tap(nccu::engine::input::Key k) {
        down_ |= Bit(k);
        pressed_ |= Bit(k);
    }
    // 在「格」之間清除 edge——`down_` 保留，使按住的鍵跨格仍為 down，與 raylib +
    // ScriptInput 相同。
    void NextFrame() {
        pressed_ = 0;
        released_ = 0;
    }

private:
    static std::uint32_t Bit(nccu::engine::input::Key k) {
        // 將相關按鍵打包進位元。此處只測 E；helper 可輕易擴充其他鍵。落在 [0, 31]
        // 雜湊範圍外者會落到 bucket 0（E），對單鍵測試而言無妨。
        const int raw = static_cast<int>(k);
        return std::uint32_t{1} << (raw & 31);
    }
    std::uint32_t down_     = 0;
    std::uint32_t pressed_  = 0;
    std::uint32_t released_ = 0;
};

// 以 RAII 切換 input source，保證全域 Input::SetSource 一定被還原——本檔每個測試
// 不論執行順序皆保持隔離（hermetic）。
class ScopedInputSource {
public:
    explicit ScopedInputSource(StubInput* src) {
        nccu::engine::input::Input::SetSource(src);
    }
    ~ScopedInputSource() {
        nccu::engine::input::Input::SetSource(nullptr);  // 還原為 LiveInput
    }
    ScopedInputSource(const ScopedInputSource&)            = delete;
    ScopedInputSource& operator=(const ScopedInputSource&) = delete;
};

constexpr float kFrameDt = 1.0f / 60.0f;       // 約 16.67 ms——固定值

} // namespace

// 單格 tap 的 edge-E 觸發一次推進。
TEST_CASE("InputHandler：單格 tap 的 edge-E 觸發一次推進") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 第 1 格：E 被 tap（本格 edge-press 且為 down）。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));      // edge 觸發
    CHECK(ih.HoldMs() > 0.0f);                  // 開始累積毫秒
    CHECK(ih.Cooldown() == 0);                  // edge 不等於 auto

    // 第 2 格：tap edge 已清除、鍵已放開。不推進。
    stub.NextFrame();
    stub.Release(nccu::engine::input::Key::E);
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() == 0.0f);                 // 放開即重置
    CHECK(ih.Cooldown() == 0);
}

// 長按 E 未滿 300 ms 不會自動推進。
TEST_CASE("InputHandler：長按 E 未滿 300 ms 不會自動推進") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 第 1 格：tap E（edge 觸發推進）。接著按住。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));      // edge 推進

    // 持續按住而不再 tap。edge 的 IsPressed 下一格即清除；只有 IsDown 在按住時維持。
    for (int f = 1; f < 17; ++f) {              // 16 格 * 16.67 約 266 ms
        stub.NextFrame();                       // 清 edge，鍵仍為 down
        CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));  // 尚未進入自動
    }
    // 17 格 * 16.67 ms 約 283 ms——仍未滿 300 ms。
    CHECK(ih.HoldMs() < nccu::InputHandler::kHoldAdvanceMs);
}

// 長按 E 超過 300 ms 後，依冷卻節奏自動推進。
TEST_CASE("InputHandler：長按 E 超過 300 ms 後依冷卻節奏自動推進") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 第 1 格：tap E。edge 推進觸發一次。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));

    // 按住 E 持續推進格數。HoldMs 跨過 300 ms 後第一次 auto 觸發；後續每
    //（kAutoCooldownFrames + 1）格觸發一次，節奏可讀。在足夠長的窗口內計算 auto 次數。
    int autoCount = 0;
    for (int f = 1; f < 40; ++f) {              // 約 640 ms 持續按住
        stub.NextFrame();                       // 清 edge，E 仍為 down
        if (ih.TickDialogAdvance(kFrameDt)) ++autoCount;
    }
    CHECK(autoCount >= 3);                      // 多次 auto 觸發
    // 每次觸發後冷卻重新生效（固定 4 格）。
    CHECK(nccu::InputHandler::kAutoCooldownFrames == 4);
}

// 兩次長按之間放開會重置計時。
TEST_CASE("InputHandler：兩次長按之間放開會重置計時") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 第一次長按：讓 HoldMs 非零，然後放開。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
    stub.NextFrame();
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() > 0.0f);

    // 放開。
    stub.Release(nccu::engine::input::Key::E);
    stub.NextFrame();
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() == 0.0f);                 // 重置

    // 稍後再次 tap——觸發全新的 edge，而非「殘留長按的 auto」。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
}

// ResetDialogAdvance 會清除長按計時與冷卻。
TEST_CASE("InputHandler：ResetDialogAdvance 清除長按計時與冷卻") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 累積長按時間並進入冷卻。
    stub.Tap(nccu::engine::input::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
    for (int f = 1; f < 30; ++f) {
        stub.NextFrame();
        ih.TickDialogAdvance(kFrameDt);
    }
    CHECK(ih.HoldMs() > 0.0f);

    // 重置（Controller 在對話關閉／從未開啟時呼叫）。
    ih.ResetDialogAdvance();
    CHECK(ih.HoldMs() == 0.0f);
    CHECK(ih.Cooldown() == 0);
}

// 同一格同時有 edge 與已達長按條件時，只走 edge 路徑、不重複觸發。
TEST_CASE("InputHandler：同一格同時有 edge 與長按時不重複觸發") {
    // 守護 edge-或-auto 的二擇一：剛按下的 E 若恰好也滿足「已按夠久」條件（罕見，但
    // Reset 後可能發生），只觸發 edge 路徑；跳過 auto 路徑，使單次按下絕不會把對話一次
    // 推進兩行。
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // 預先累積計時（模擬前一次長按未完全重置，雖然正常放開會重置——此為防禦性檢查）。
    // 先在按住狀態下推進累積 HoldMs，再重新 tap；edge 路徑必須優先。
    stub.Hold(nccu::engine::input::Key::E);
    for (int f = 0; f < 30; ++f) {
        stub.NextFrame();
        ih.TickDialogAdvance(kFrameDt);
    }
    // 在長按之上強制一次 edge-press。
    stub.NextFrame();
    stub.Tap(nccu::engine::input::Key::E);                // tap == down + pressed
    const int cooldownBefore = ih.Cooldown();
    CHECK(ih.TickDialogAdvance(kFrameDt));      // 回傳 true（edge）
    // auto 分支的冷卻戳記不應被重新設定：edge 路徑直接回傳 true，從不碰觸冷卻。
    CHECK(ih.Cooldown() == cooldownBefore);
}
