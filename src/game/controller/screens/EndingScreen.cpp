#include "game/controller/screens/EndingScreen.h"
#include "game/world/World.h"
#include "game/state/EndingMenuModel.h"  // IsEndingState + EndingMenuChoiceAt
#include "engine/input/Input.h"
#include "engine/input/Key.h"

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出，以此引入

bool HandleEndingMenu(World& world) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    if (!IsEndingState(world.Semester().Current())) return false;
    if (Input::IsPressed(Key::Left))  world.MoveEndingMenuCursor(-1);
    if (Input::IsPressed(Key::Right)) world.MoveEndingMenuCursor(1);
    if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
        switch (EndingMenuChoiceAt(world.EndingMenuCursor())) {
            case EndingMenuChoice::BackToTitle:
                // 回到標題畫面（完整拆解 -> 標題）。
                world.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::RestartGame:
                // 全新一局：同樣的 Restart 拆解 -> 標題，再由標題重新開始 Ch1
                // （狀態由 World 重建而完全重置）。
                world.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::Quit:
                // 真正離開——唯一關閉畫布的路徑。
                world.RequestAppAction(World::AppAction::Quit);
                break;
        }
    }
    return true;   // 停在結局畫面凍結，直到玩家選擇一個選項
}

} // namespace nccu
