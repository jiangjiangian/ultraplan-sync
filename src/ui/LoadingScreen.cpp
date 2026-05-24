#include "ui/LoadingScreen.h"
#include "gfx/TexturePreload.h"
#include "gfx/DrawScope.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"

namespace nccu {
namespace {

constexpr int kWinW = 800;
constexpr int kWinH = 450;

// A dim near-black backdrop (matches the title/select panel palette) so the
// label reads clearly. Opaque clear every frame.
constexpr gfx::Color kBackdrop{14, 16, 22, 255};
constexpr gfx::Color kLabel   {235, 235, 240, 255};
constexpr gfx::Color kHint    {150, 150, 160, 255};

constexpr int kLabelSize = 34;
constexpr int kHintSize  = 16;

// How many frames to hold the loading screen AFTER the (synchronous) warm,
// so it is actually perceptible rather than flashing past in one frame. At
// the title's 60 fps cap this is a short fraction of a second — enough to
// register "loading", short enough not to feel like a stall on the empty-
// resources path where the warm is instant.
constexpr int kHoldFrames = 18;

// Draw one centered loading frame. Centering uses TextBuilder::Measure()
// (exact CJK-aware pixel size) so the label sits dead-centre regardless of
// whether the CJK font loaded or text fell back to the ASCII default.
void DrawLoadingFrame() {
    using namespace gfx;
    DrawScope frame;
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

} // namespace

void RunLoadingScreen(gfx::Window& win) {
    // Show the label for one frame BEFORE the (possibly costly) warm, so the
    // framebuffer already reads "載入中…" while textures upload.
    if (win.ShouldClose()) return;
    DrawLoadingFrame();

    // Warm the texture cache: the one-time disk read + GPU upload of the
    // worldmap / buildings / sprites happens HERE, behind the label, instead
    // of stuttering the first gameplay frame. Cheap no-op on a clean clone
    // (every path is a missing-file cache miss).
    gfx::PreloadGameTextures();

    // Hold the label briefly so the screen is perceptible. Pump frames (no
    // wall-clock sleep) so the window stays responsive and a close request
    // during loading still ends cleanly.
    for (int i = 0; i < kHoldFrames && !win.ShouldClose(); ++i)
        DrawLoadingFrame();
}

} // namespace nccu
