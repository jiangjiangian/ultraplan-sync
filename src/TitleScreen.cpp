#include "TitleScreen.h"
#include "gfx/DrawScope.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
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

struct MenuItem {
    std::string_view label;
    TitleChoice      choice;
};

constexpr std::array<MenuItem, 2> kItems{{
    {"開始遊戲", TitleChoice::StartGame},
    {"離開",     TitleChoice::Quit},
}};

} // namespace

TitleChoice RunTitleScreen(gfx::Window& win) {
    using namespace gfx;
    constexpr int kCount = static_cast<int>(kItems.size());
    int cursor = 0;

    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::Up) || Input::IsPressed(Key::W))
            cursor = (cursor - 1 + kCount) % kCount;
        if (Input::IsPressed(Key::Down) || Input::IsPressed(Key::S))
            cursor = (cursor + 1) % kCount;
        if (Input::IsPressed(Key::Enter))
            return kItems[static_cast<std::size_t>(cursor)].choice;

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
