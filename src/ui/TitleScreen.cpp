#include "ui/TitleScreen.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"  // shared 遊戲說明 page renderer (de-dup with View)
#include "ui/PressLatch.h"
#include "engine/render/DrawScope.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include <array>
#include <string>
#include <string_view>

namespace nccu {
namespace {

constexpr int kWinW = 800;
constexpr int kWinH = 450;

constexpr gfx::Color kHighlight{255, 153, 0, 255};
constexpr gfx::Color kPanel    { 18,  20,  28, 200};
constexpr gfx::Color kDim      {170, 170, 170, 255};

// REQUIREMENT #9: 「遊戲說明」 is a title-internal page, not a public
// TitleChoice (the screen flow in main.cpp still only sees Start/Quit).
// MenuAction is the on-screen action; Help loops back to the menu.
enum class MenuAction { Start, Help, Quit };

struct MenuItem {
    std::string_view label;
    MenuAction       action;
};

constexpr std::array<MenuItem, 3> kItems{{
    {"開始遊戲", MenuAction::Start},
    {"遊戲說明", MenuAction::Help},
    {"離開",     MenuAction::Quit},
}};

// Renders the shared 遊戲說明 page (same GameHelp text as the in-game
// 說明 overlay — single source of truth, never drifts). Blocks until
// the player presses Enter / E, then returns to the title menu (ESC quits).
// Returns false iff the window was closed while on the help page (so
// the caller can exit cleanly instead of looping forever).
bool RunHelpPage(gfx::Window& win) {
    using namespace gfx;
    // The Enter that opened this page is still held on the first frame (no
    // input poll runs between RunTitleScreen reading it and here), so gate
    // Enter through a release-latch — otherwise the page would close on the
    // same press that opened it. E is never inherited from the title menu,
    // so it may dismiss immediately. (ESC is the program's quit key — it
    // falls through to WindowShouldClose below, exiting cleanly.)
    PressLatch enter;
    // U2-T4: the shared help is now PAGED (kGameHelpPages). The title page
    // keeps its page index LOCAL (no World here); ←/→ flip it, matching the
    // in-game overlay's nav. Starts on page 1.
    int page = 0;
    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::E))
            return true;                       // back to the title menu
        if (enter.Fired(Input::IsDown(Key::Enter), Input::IsPressed(Key::Enter)))
            return true;
        if (Input::IsPressed(Key::Right))
            page = (page + 1) % nccu::kGameHelpPageCount;
        if (Input::IsPressed(Key::Left))
            page = (page - 1 + nccu::kGameHelpPageCount) %
                   nccu::kGameHelpPageCount;
        {
            DrawScope frame;
            Renderer r;
            r.Clear(Colors::RayWhite);
            // Shared 遊戲說明 page body (review MINOR de-dup with View.cpp's
            // in-game overlay). The title-screen values: 200α panel (kPanel),
            // dim-grey indicator, "Enter / E 返回" chip at -56. Pixel-
            // identical to the prior inline draw.
            nccu::ui::DrawHelpPage(
                [&r](Rect rc, Color c) { r.Rect(rc, c); },
                nccu::ui::HelpPageStyle{
                    static_cast<float>(kWinW), static_cast<float>(kWinH),
                    page, kPanel, Color{120, 120, 130, 255},
                    "Enter / E 返回", -56.0f});
        }
    }
    return false;                              // window closed
}

} // namespace

TitleChoice RunTitleScreen(gfx::Window& win) {
    using namespace gfx;
    constexpr int kCount = static_cast<int>(kItems.size());
    int cursor = 0;
    // Gate the confirm key so a press inherited from a previous screen
    // (the Enter that returned from the help page, or a Restart/back-out)
    // can't auto-trigger a menu item — it must be released and pressed
    // again here. After firing for 遊戲說明 the latch stays disarmed, so
    // the held dismiss-Enter coming back from the help page won't re-open
    // it.
    PressLatch confirm;

    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::Up) || Input::IsPressed(Key::W))
            cursor = (cursor - 1 + kCount) % kCount;
        if (Input::IsPressed(Key::Down) || Input::IsPressed(Key::S))
            cursor = (cursor + 1) % kCount;
        if (confirm.Fired(Input::IsDown(Key::Enter),
                          Input::IsPressed(Key::Enter))) {
            switch (kItems[static_cast<std::size_t>(cursor)].action) {
                case MenuAction::Start: return TitleChoice::StartGame;
                case MenuAction::Quit:  return TitleChoice::Quit;
                case MenuAction::Help:
                    // Show the help page; if the window was closed there,
                    // exit cleanly, else fall back to this title loop.
                    if (!RunHelpPage(win)) return TitleChoice::Quit;
                    continue;
            }
        }

        {
            DrawScope frame;
            Renderer r;
            r.Clear(Colors::RayWhite);

            // A dark banner behind the title so the CJK glyphs pop on the
            // bright RayWhite background (same idiom as the in-game HUD).
            const Rect banner{0.0f, 86.0f,
                              static_cast<float>(kWinW), 132.0f};
            r.Rect(banner, kPanel);

            TextBuilder{"尋傘記"}
                .At(Vec2{kWinW / 2.0f - 96.0f, 104.0f})
                .Size(64).Color(Colors::Gold).Draw();
            TextBuilder{"政大山下篇"}
                .At(Vec2{kWinW / 2.0f - 100.0f, 172.0f})
                .Size(30).Color(Colors::White).Draw();

            constexpr float kMenuY = 268.0f;
            constexpr float kLineH = 52.0f;
            for (int i = 0; i < kCount; ++i) {
                const float y = kMenuY + i * kLineH;
                const bool sel = (i == cursor);
                const std::string label =
                    std::string{kItems[static_cast<std::size_t>(i)].label};
                // The selected row gets a marker + highlight colour so the
                // keyboard cursor is unambiguous without a mouse.
                const std::string row =
                    (sel ? "> " : "  ") + label;
                TextBuilder{row}
                    .At(Vec2{kWinW / 2.0f - 70.0f, y})
                    .Size(28)
                    .Color(sel ? kHighlight : kDim)
                    .Draw();
            }

            TextBuilder{"↑ ↓ 選擇    Enter 確認"}
                .At(Vec2{kWinW / 2.0f - 132.0f,
                         static_cast<float>(kWinH) - 44.0f})
                .Size(18).Color(Colors::DarkGray).Draw();
        }
    }
    return TitleChoice::Quit;   // window closed → exit cleanly
}

} // namespace nccu
