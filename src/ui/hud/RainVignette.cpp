#include "ui/hud/RainVignette.h"

#include "game/entities/Player.h"
#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <algorithm>

namespace nccu {

void DrawRainVignette(nccu::engine::render::IRenderer& r,
                      const World& world,
                      float screenW,
                      float screenH) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 雨勢「壓力」暈影——純 View 繪製，僅由 GetRainMeter() 推導，不碰
    // 模擬／狀態／輸入（遵守 MVC §5）。此處雨勢不致命，回饋純為視覺。
    // 螢幕邊緣依雨量分兩段加深（≥60 微弱、≥85 較強），以四條邊框帶繪出
    // （成本低、不需全螢幕貼圖或配置記憶體，且結果具決定性）。
    const Player* p = world.GetPlayer();
    if (!p) return;

    const float rm = p->GetRainMeter();
    unsigned char va = 0;
    if (rm >= 85.0f)      va = 90;
    else if (rm >= 60.0f) va = 45;
    if (va == 0) return;

    const Color v{0, 0, 0, va};
    const float W = screenW;
    const float H = screenH;
    const float b = std::min(W, H) * 0.12f;  // 邊框帶厚度
    r.DrawRect(Rect{0.0f, 0.0f, W, b}, v);          // 上
    r.DrawRect(Rect{0.0f, H - b, W, b}, v);         // 下
    r.DrawRect(Rect{0.0f, 0.0f, b, H}, v);          // 左
    r.DrawRect(Rect{W - b, 0.0f, b, H}, v);         // 右
}

}  // namespace nccu
