#include "game/controller/InputHandler.h"

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出，以此引入

bool InputHandler::TickDialogAdvance(float dt) noexcept {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;

    // 按住計時器：E 按住時遞增，放開時歸零。冷卻計數也在放開時重置，使下一次按住
    // 重新開始。
    if (Input::IsDown(Key::E)) {
        eHoldMs_ += dt * 1000.0f;
    } else {
        eHoldMs_ = 0.0f;
        eAutoAdvanceCooldown_ = 0;
    }

    const bool edgeE = Input::IsPressed(Key::E);

    // 只有「E 按住但非剛按下」時才走自動觸發分支。下方邊緣按下路徑負責「每次按下
    // 觸發一次」的語意，使單次按下絕不在同一幀同時邊緣觸發又自動觸發（否則會把對話框
    // 推進兩次）。
    bool autoE = false;
    if (!edgeE && Input::IsDown(Key::E) && eHoldMs_ >= kHoldAdvanceMs) {
        if (eAutoAdvanceCooldown_ > 0) {
            --eAutoAdvanceCooldown_;
        } else {
            autoE = true;
            eAutoAdvanceCooldown_ = kAutoCooldownFrames;
        }
    }

    return edgeE || autoE;
}

} // namespace nccu
