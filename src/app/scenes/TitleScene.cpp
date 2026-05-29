#include "app/scenes/TitleScene.h"

#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace nccu::app {
using namespace nccu::engine::input;  // Phase 4 §B: input types moved out of nccu::gfx
namespace {

constexpr int kWinW = 800;
constexpr int kWinH = 450;

constexpr nccu::engine::math::Color kHighlight{255, 153,   0, 255};
constexpr nccu::engine::math::Color kPanel    { 18,  20,  28, 200};
constexpr nccu::engine::math::Color kDim      {170, 170, 170, 255};

// REQUIREMENT #9: 「遊戲說明」 is a title-internal page (mirrors the
// pre-Phase-3 free-function MenuAction): Help loops back to the menu;
// only Start / Quit reach the scene boundary.
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
constexpr int kCount = static_cast<int>(kItems.size());

} // namespace

TitleScene::TitleScene(NextSceneFactory startGame)
    : startGame_(std::move(startGame)) {}

SceneCommand TitleScene::Update(float /*dt*/) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;

    // 遊戲說明 page is a title-internal sub-state — input + draw both
    // route here, the menu input is muted.
    if (showingHelp_) {
        if (Input::IsPressed(Key::E)) {
            showingHelp_ = false;
            return {};
        }
        if (helpBack_.Fired(Input::IsDown(Key::Enter),
                            Input::IsPressed(Key::Enter))) {
            showingHelp_ = false;
            return {};
        }
        if (Input::IsPressed(Key::Right))
            helpPage_ = (helpPage_ + 1) % nccu::kGameHelpPageCount;
        if (Input::IsPressed(Key::Left))
            helpPage_ = (helpPage_ - 1 + nccu::kGameHelpPageCount) %
                        nccu::kGameHelpPageCount;
        return {};
    }

    if (Input::IsPressed(Key::Up) || Input::IsPressed(Key::W))
        cursor_ = (cursor_ - 1 + kCount) % kCount;
    if (Input::IsPressed(Key::Down) || Input::IsPressed(Key::S))
        cursor_ = (cursor_ + 1) % kCount;
    if (confirm_.Fired(Input::IsDown(Key::Enter),
                       Input::IsPressed(Key::Enter))) {
        switch (kItems[static_cast<std::size_t>(cursor_)].action) {
            case MenuAction::Start:
                if (startGame_) {
                    auto fact = startGame_;
                    return SceneCommand{SceneCommand::Kind::Replace,
                                        std::move(fact)};
                }
                return {};
            case MenuAction::Quit:
                return SceneCommand{SceneCommand::Kind::Quit, {}};
            case MenuAction::Help:
                showingHelp_ = true;
                // helpBack_ is freshly constructed (or reset by a prior
                // return-to-menu); its IsHeld latch will wait for the
                // current Enter press to be released before it can fire,
                // so the dismiss-Enter never re-opens the menu on the
                // same press that opened Help.
                return {};
        }
    }
    return {};
}

void TitleScene::Draw(nccu::engine::render::IRenderer& /*renderer*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    if (showingHelp_) {
        Renderer r;
        r.Clear(Colors::RayWhite);
        nccu::ui::DrawHelpPage(
            [&r](Rect rc, Color c) { r.Rect(rc, c); },
            nccu::ui::HelpPageStyle{
                static_cast<float>(kWinW), static_cast<float>(kWinH),
                helpPage_, kPanel, Color{120, 120, 130, 255},
                "Enter / E 返回", -56.0f});
        return;
    }

    Renderer r;
    r.Clear(Colors::RayWhite);

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
        const bool sel = (i == cursor_);
        const std::string label =
            std::string{kItems[static_cast<std::size_t>(i)].label};
        const std::string row = (sel ? "> " : "  ") + label;
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

} // namespace nccu::app
