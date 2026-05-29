#include "app/scenes/CharacterSelectScene.h"

#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <cstddef>
#include <string>
#include <utility>

namespace nccu::app {
using namespace nccu::engine::input;  // Phase 4 §B: input types moved out of nccu::gfx
namespace {

constexpr int kWinW      = 800;
constexpr int kWinH      = 450;
constexpr int kFrameSize = 32;  // Pipoya cell
constexpr int kIdleCol   = 1;   // middle column of the 3-frame walk strip
constexpr int kDownRow   = 0;   // first row faces the camera

constexpr nccu::engine::math::Color kHighlight{255, 153,   0, 255};
constexpr nccu::engine::math::Color kPanel    { 20,  22,  30, 210};
constexpr nccu::engine::math::Color kDim      {170, 170, 170, 255};

constexpr int kCount = static_cast<int>(nccu::kPersonas.size());
constexpr int kTile = 96;
constexpr int kGap  = 22;
constexpr int kRowW = kCount * kTile + (kCount - 1) * kGap;
constexpr int kRowX = (kWinW - kRowW) / 2;
constexpr int kRowY = 150;

nccu::engine::math::Rect IdleSrc() {
    return nccu::engine::math::Rect{
        static_cast<float>(kIdleCol * kFrameSize),
        static_cast<float>(kDownRow * kFrameSize),
        static_cast<float>(kFrameSize),
        static_cast<float>(kFrameSize)};
}

} // namespace

CharacterSelectScene::CharacterSelectScene(GameplayFactory gameplay)
    : gameplay_(std::move(gameplay)) {}

void CharacterSelectScene::Enter() {
    // Load preview textures once on attach. Texture is move-only with
    // no default ctor, so a vector built by push_back mirrors the
    // pre-Phase-3 RunCharacterSelect implementation. The textures
    // die with the scene (RAII) when the manager replaces this scene
    // with GameplayScene on confirm — GL is still alive on that
    // boundary (SceneManager runs INSIDE the Window scope), matching
    // the pre-Phase-3 lifetime.
    previews_.reserve(static_cast<std::size_t>(kCount));
    for (int i = 0; i < kCount; ++i)
        previews_.push_back(nccu::gfx::Texture::Load(
            std::string{nccu::kPersonas[static_cast<std::size_t>(i)]
                            .spritePath}));
}

SceneCommand CharacterSelectScene::Update(float /*dt*/) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;

    if (Input::IsPressed(Key::Right) || Input::IsPressed(Key::D))
        cursor_ = (cursor_ + 1) % kCount;
    if (Input::IsPressed(Key::Left)  || Input::IsPressed(Key::A))
        cursor_ = (cursor_ - 1 + kCount) % kCount;
    if (confirm_.Fired(Input::IsDown(Key::Enter),
                       Input::IsPressed(Key::Enter))) {
        nccu::CharacterSelectResult result;
        const nccu::Persona& p =
            nccu::kPersonas[static_cast<std::size_t>(cursor_)];
        result.spritePath = std::string{p.spritePath};
        result.tint       = p.tint;
        result.closed     = false;
        // Replace ourselves with the factory's gameplay scene; the
        // captured factory builds GameplayScene with the audioDevice/
        // harness/window refs the composition root owns. The
        // SceneCommand thunk is deferred-applied AFTER this frame's
        // Draw so previews_ stays alive for the closing paint.
        auto fact = gameplay_;
        return SceneCommand{
            SceneCommand::Kind::Replace,
            [fact = std::move(fact), result]() mutable {
                return fact(std::move(result));
            }};
    }
    // ESC is intentionally inert here (player request, mirrors
    // pre-Phase-3 RunCharacterSelect): the player picks a persona
    // with ← → + Enter; there is no ESC-to-title.
    return {};
}

void CharacterSelectScene::Draw(nccu::gfx::IRenderer& /*renderer*/) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;
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
        const nccu::Persona& pe =
            nccu::kPersonas[static_cast<std::size_t>(i)];
        const Rect dest{
            static_cast<float>(kRowX + i * (kTile + kGap)),
            static_cast<float>(kRowY),
            static_cast<float>(kTile),
            static_cast<float>(kTile)};
        r.TextureRect(previews_[static_cast<std::size_t>(i)],
                      src, dest, pe.tint);
        if (i == cursor_)
            r.RectLines(dest, kHighlight, 3.0f);
    }

    const nccu::Persona& sel =
        nccu::kPersonas[static_cast<std::size_t>(cursor_)];
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

} // namespace nccu::app
