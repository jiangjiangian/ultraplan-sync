#include "ui/CharacterSelect.h"
#include "ui/PressLatch.h"
#include "engine/render/DrawScope.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "engine/render/Texture.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {
namespace {

constexpr int kWinW       = 800;
constexpr int kWinH       = 450;
constexpr int kFrameSize  = 32;  // each Pipoya cell is 32x32
constexpr int kIdleCol    = 1;   // middle column of the 3-frame walk strip
constexpr int kDownRow    = 0;   // first row faces the camera

constexpr gfx::Color kHighlight{255, 153, 0, 255};
constexpr gfx::Color kPanel    { 20,  22,  30, 210};
constexpr gfx::Color kDim      {170, 170, 170, 255};

gfx::Rect IdleSrc() {
    return gfx::Rect{
        static_cast<float>(kIdleCol * kFrameSize),
        static_cast<float>(kDownRow * kFrameSize),
        static_cast<float>(kFrameSize),
        static_cast<float>(kFrameSize)};
}

} // namespace

CharacterSelectResult RunCharacterSelect(gfx::Window& win) {
    using namespace gfx;
    CharacterSelectResult result;

    constexpr int kCount = static_cast<int>(kPersonas.size());  // 5
    // Texture is move-only with no default ctor → a vector built by
    // push_back (mirrors the previous character-select implementation).
    std::vector<Texture> previews;
    previews.reserve(static_cast<std::size_t>(kCount));
    for (int i = 0; i < kCount; ++i)
        previews.push_back(Texture::Load(
            std::string{kPersonas[static_cast<std::size_t>(i)]
                            .spritePath}));

    // Five tiles in a single horizontal row; the selected one is boxed
    // and its name/blurb shown in a panel below. Each preview is colour-
    // modulated with its persona tint so personas sharing a base sheet
    // still read as visually distinct (RED LINE: no new asset files).
    constexpr int kTile = 96;
    constexpr int kGap  = 22;
    constexpr int kRowW = kCount * kTile + (kCount - 1) * kGap;
    constexpr int kRowX = (kWinW - kRowW) / 2;
    constexpr int kRowY = 150;
    int cursor = 0;
    // Ignore the Enter inherited from the title's 開始遊戲 (still held when
    // this screen starts) — without this, that one press would confirm
    // persona 0 on frame 1 and the player would never see this screen.
    PressLatch confirm;

    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::Right) || Input::IsPressed(Key::D))
            cursor = (cursor + 1) % kCount;
        if (Input::IsPressed(Key::Left)  || Input::IsPressed(Key::A))
            cursor = (cursor - 1 + kCount) % kCount;
        if (confirm.Fired(Input::IsDown(Key::Enter),
                          Input::IsPressed(Key::Enter))) {
            const Persona& p = kPersonas[static_cast<std::size_t>(cursor)];
            result.spritePath = std::string{p.spritePath};
            result.tint       = p.tint;
            result.closed     = false;
            return result;
        }
        // ESC is intentionally inert here (player request): the player
        // picks a persona with ← → + Enter; there is no ESC-to-title.

        {
            DrawScope frame;
            Renderer r;
            r.Clear(Colors::RayWhite);

            TextBuilder{"選擇你的角色"}
                .At(Vec2{kWinW / 2.0f - 84.0f, 56.0f})
                .Size(30).Color(Colors::DarkGray).Draw();
            TextBuilder{"五位政大山下的學生，沒有性別之分"}
                .At(Vec2{kWinW / 2.0f - 198.0f, 100.0f})
                .Size(18).Color(kDim).Draw();

            const Rect src = IdleSrc();
            for (int i = 0; i < kCount; ++i) {
                const Persona& pe =
                    kPersonas[static_cast<std::size_t>(i)];
                const Rect dest{
                    static_cast<float>(kRowX + i * (kTile + kGap)),
                    static_cast<float>(kRowY),
                    static_cast<float>(kTile),
                    static_cast<float>(kTile)};
                r.TextureRect(previews[static_cast<std::size_t>(i)],
                              src, dest, pe.tint);
                if (i == cursor)
                    r.RectLines(dest, kHighlight, 3.0f);
            }

            // Name + blurb panel for the highlighted persona.
            const Persona& sel =
                kPersonas[static_cast<std::size_t>(cursor)];
            constexpr float kPanelY = 280.0f;
            constexpr float kPanelH = 78.0f;
            const Rect panel{60.0f, kPanelY,
                             static_cast<float>(kWinW) - 120.0f, kPanelH};
            r.Rect(panel, kPanel);
            TextBuilder{std::string{sel.label}}
                .At(Vec2{panel.x + 16.0f, kPanelY + 12.0f})
                .Size(24).Color(Colors::Gold).Draw();
            TextBuilder{std::string{sel.blurb}}
                .At(Vec2{panel.x + 16.0f, kPanelY + 44.0f})
                .Size(18).Color(Colors::White).Draw();

            TextBuilder{"← → 選擇    Enter 確認"}
                .At(Vec2{kWinW / 2.0f - 120.0f,
                         static_cast<float>(kWinH) - 44.0f})
                .Size(18).Color(Colors::DarkGray).Draw();
        }
    }

    result.closed = true;
    return result;
}

} // namespace nccu
