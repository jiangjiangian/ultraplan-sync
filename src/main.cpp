#include "World.h"
#include "View.h"
#include "GameController.h"
#include "CharacterSelect.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"

// MVC composition root. Model = World (pure data), View = rendering,
// Controller = input + simulation + event wiring. The game loop is the
// only logic here: tick the controller, then draw the model through the
// view. Everything else is one-time setup / teardown.
int main() {
    constexpr int kWinW = 800;
    constexpr int kWinH = 450;

    auto win = nccu::gfx::Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    auto selection = nccu::RunCharacterSelect(win);
    if (selection.closed) return 0;

    // Declaration order matters: reverse-destruction runs the controller
    // dtor (EventBus::Clear) BEFORE the World refs its subscribers
    // captured die. Do not reorder these three.
    nccu::World          world{selection.spritePath};
    nccu::View           view{kWinW, kWinH};
    nccu::GameController  controller{world};

    while (!win.ShouldClose()) {
        controller.Update();
        {
            nccu::gfx::DrawScope frame;
            view.Draw(world);
        }
    }
    return 0;
}
