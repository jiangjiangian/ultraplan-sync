#include "World.h"
#include "View.h"
#include "GameController.h"
#include "CharacterSelect.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/Font.h"

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

    // raylib's default font is ASCII-only; load the CJK font now that the
    // GL context exists, before any text (incl. character-select) draws.
    nccu::gfx::EnsureFont();

    auto selection = nccu::RunCharacterSelect(win);
    if (selection.closed) {
        nccu::gfx::ShutdownFont();   // free the glyph atlas while GL is live
        return 0;
    }

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

    // Unload the font BEFORE the Window dtor runs ::CloseWindow(): a
    // static-lifetime font would otherwise destruct after the GL context
    // is gone. `win` is still alive on this line, so GL is valid.
    nccu::gfx::ShutdownFont();
    return 0;
}
