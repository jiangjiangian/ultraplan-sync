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

/**
 * @file CharacterSelectScene.cpp
 * @brief 角色選擇場景的實作：載入預覽紋理、處理游標與確認、繪製角色格與選取面板。
 */

namespace nccu::app {
using namespace nccu::engine::input;  // 輸入型別位於 nccu::engine::input 命名空間
namespace {

constexpr int kWinW      = 800;
constexpr int kWinH      = 450;
constexpr int kFrameSize = 32;  // Pipoya 素材的單格尺寸
constexpr int kIdleCol   = 1;   // 三幀行走條的中間欄（待機格）
constexpr int kDownRow   = 0;   // 第一列為面向鏡頭的朝下方向

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
    // 掛載時一次性載入預覽紋理。Texture 為 move-only 且無預設建構子，故以 push_back
    // 建構 vector。紋理隨場景 RAII 釋放：確認後管理器以 GameplayScene 取代本場景時，
    // 該切換邊界上 GL 仍存活（SceneManager 跑在 Window 範圍之內）。
    previews_.reserve(static_cast<std::size_t>(kCount));
    for (int i = 0; i < kCount; ++i)
        previews_.push_back(nccu::engine::render::Texture::Load(
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
        // 以工廠產生的遊玩場景取代自身；被捕獲的工廠會帶著 composition root 擁有的
        // audioDevice／harness／window 參考建構 GameplayScene。此 SceneCommand thunk
        // 於本幀 Draw 之後才延後套用，故 previews_ 在收尾繪製期間仍存活。
        auto fact = gameplay_;
        return SceneCommand{
            SceneCommand::Kind::Replace,
            [fact = std::move(fact), result]() mutable {
                return fact(std::move(result));
            }};
    }
    // ESC 在此刻意為惰性：玩家以 ← → ＋ Enter 選擇角色，沒有「ESC 返回標題」。
    return {};
}

void CharacterSelectScene::Draw(nccu::engine::render::IRenderer& /*renderer*/) {
    using namespace nccu::engine::render;
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
