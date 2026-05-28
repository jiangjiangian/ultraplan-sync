#include "controller/screens/EndingScreen.h"
#include "world/World.h"
#include "ui/EndingView.h"        // A-T3: IsEndingState + EndingMenuChoiceAt
#include "engine/input/Input.h"
#include "engine/input/Key.h"

namespace nccu {

bool HandleEndingMenu(World& world) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    if (!IsEndingState(world.Semester().Current())) return false;
    if (Input::IsPressed(Key::Left))  world.MoveEndingMenuCursor(-1);
    if (Input::IsPressed(Key::Right)) world.MoveEndingMenuCursor(1);
    if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
        switch (EndingMenuChoiceAt(world.EndingMenuCursor())) {
            case EndingMenuChoice::BackToTitle:
                // Back to the title screen (full teardown → title).
                world.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::RestartGame:
                // A fresh new game: same Restart teardown → title, from
                // which the player starts Ch1 anew (state fully reset by
                // the World rebuild; CLAUDE.md "must fully reset state").
                world.RequestAppAction(World::AppAction::Restart);
                break;
            case EndingMenuChoice::Quit:
                // True quit — the only path that closes the canvas.
                world.RequestAppAction(World::AppAction::Quit);
                break;
        }
    }
    return true;   // frozen on the ending screen until an option is chosen
}

} // namespace nccu
