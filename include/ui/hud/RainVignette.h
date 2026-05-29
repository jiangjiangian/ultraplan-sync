#ifndef UI_HUD_RAIN_VIGNETTE_H_
#define UI_HUD_RAIN_VIGNETTE_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file RainVignette.h
 * @brief 雨「壓力」暗角（自 View::RenderHud 抽出）。
 */

/**
 * @brief 繪製雨壓暗角：由當前玩家 RainMeter 驅動、分兩段的螢幕邊緣變暗，純屬視
 *        覺回饋。
 *
 * 以四條邊框帶繪製（而非整張全螢幕貼圖）：便宜、無逐幀配置、具決定性且可無頭
 * spy 測試。反應式：為 `World::GetPlayer()->GetRainMeter()` 的純函式（≥60 微弱、
 * ≥85 較強）；無 Player 或讀數低於 60 時提前返回，每幀呼叫皆安全。純渲染（MVC）：
 * 以 const 讀取 World、絕不變更。
 */
void DrawRainVignette(nccu::engine::render::IRenderer& r,
                      const World& world,
                      float screenW,
                      float screenH);

}  // namespace nccu

#endif  // UI_HUD_RAIN_VIGNETTE_H_
