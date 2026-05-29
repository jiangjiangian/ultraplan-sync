#include "app/scenes/LoadingScene.h"

#include "engine/render/Renderer.h"
#include "game/world/TexturePreload.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"

#include <utility>

namespace nccu::app {
namespace {

constexpr int kWinW = 800;
constexpr int kWinH = 450;

constexpr nccu::engine::math::Color kBackdrop{ 14,  16,  22, 255};
constexpr nccu::engine::math::Color kLabel   {235, 235, 240, 255};
constexpr nccu::engine::math::Color kHint    {150, 150, 160, 255};

constexpr int kLabelSize = 34;
constexpr int kHintSize  = 16;

// How many frames to hold the loading screen AFTER the (synchronous)
// warm — pre-Phase-3's kHoldFrames. At 60 fps this is a short
// fraction of a second: enough to register "loading", short enough
// not to feel like a stall on a clean clone where the warm is
// instant. The TOTAL on-screen time is one frame longer because
// LoadingScene::Enter()'s warm happens BEFORE the manager's first
// Update/Draw pair (the screen already shows "載入中…" while
// PreloadGameTextures runs).
constexpr int kHoldFrames = 18;

} // namespace

LoadingScene::LoadingScene(NextSceneFactory next)
    : next_(std::move(next)) {}

void LoadingScene::Enter() {
    // Warm the texture cache: the one-time disk read + GPU upload of
    // the worldmap / buildings / sprites happens HERE, behind the
    // label, instead of stuttering the first gameplay frame. Cheap
    // no-op on a clean clone (every path is a missing-file cache
    // miss). The pre-Phase-3 free function called this between the
    // first DrawFrame and the kHoldFrames loop; doing it in Enter
    // keeps the same intent — the warm runs ONCE, before any
    // Update/Draw pair fires.
    nccu::game::world::PreloadGameTextures();
}

SceneCommand LoadingScene::Update(float /*dt*/) {
    ++frameCount_;
    if (frameCount_ >= kHoldFrames && next_) {
        auto next = next_;
        return SceneCommand{SceneCommand::Kind::Replace, std::move(next)};
    }
    return {};
}

void LoadingScene::Draw(nccu::engine::render::IRenderer& /*renderer*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;
    Renderer r;
    r.Clear(kBackdrop);

    TextBuilder label{"載入中…"};
    label.Size(kLabelSize).Color(kLabel);
    const Vec2 m = label.Measure();
    label.At(Vec2{(kWinW - m.x) * 0.5f, (kWinH - m.y) * 0.5f - 12.0f}).Draw();

    TextBuilder hint{"正在準備政大山下的雨天…"};
    hint.Size(kHintSize).Color(kHint);
    const Vec2 hm = hint.Measure();
    hint.At(Vec2{(kWinW - hm.x) * 0.5f,
                 (kWinH - hm.y) * 0.5f + 28.0f}).Draw();
}

} // namespace nccu::app
