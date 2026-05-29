#include "app/scenes/LoadingScene.h"

#include "engine/render/Renderer.h"
#include "game/world/TexturePreload.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Vec2.h"

#include <utility>

/**
 * @file LoadingScene.cpp
 * @brief 載入場景的實作：在 Enter() 暖機紋理快取，停留固定幀數後切換至下一場景。
 */

namespace nccu::app {
namespace {

constexpr int kWinW = 800;
constexpr int kWinH = 450;

constexpr nccu::engine::math::Color kBackdrop{ 14,  16,  22, 255};
constexpr nccu::engine::math::Color kLabel   {235, 235, 240, 255};
constexpr nccu::engine::math::Color kHint    {150, 150, 160, 255};

constexpr int kLabelSize = 34;
constexpr int kHintSize  = 16;

// 同步暖機之後，載入畫面要再停留的幀數。於 60 fps 下約為零點幾秒：足以讓人意識到
// 「正在載入」，又短到在乾淨 clone（暖機瞬間完成）上不致感覺卡住。實際在畫面上的
// 總時間會多一幀，因為 LoadingScene::Enter() 的暖機發生在管理器第一個 Update／Draw
// 配對之前（PreloadGameTextures 執行時畫面已顯示「載入中…」）。
constexpr int kHoldFrames = 18;

} // namespace

LoadingScene::LoadingScene(NextSceneFactory next)
    : next_(std::move(next)) {}

void LoadingScene::Enter() {
    // 暖機紋理快取：世界地圖／建築／角色的一次性磁碟讀取與 GPU 上傳在此（標題後方）
    // 完成，而非卡頓在首個遊玩幀。乾淨 clone 上為廉價的 no-op（每條路徑都是缺檔的
    // 快取未命中）。放在 Enter 確保暖機只跑「一次」，且早於任何 Update／Draw 配對。
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
