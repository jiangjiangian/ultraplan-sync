#ifndef UI_HUD_SPORTS_LAP_RING_H_
#define UI_HUD_SPORTS_LAP_RING_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file SportsLapRing.h
 * @brief 操場校慶圈速進度環（HUD；自 View::RenderHud 抽出）。
 */

/**
 * @brief 繪製操場圈速進度環：右上角一個 16 點的環，隨圈速進度順時針填滿；它是
 *        LapTrack 在世界座標繪製的地面跑道在螢幕上的對應物。
 *
 * 反應式：為 World::SportsLapActive 與 SportsLapProgress 的純函式，每幀呼叫皆安
 * 全——未進行圈速時提前返回。純渲染（MVC）：以 const 讀取 World、絕不變更。環以
 * 16 個小實心方塊繪製（不依賴 renderer 介面的 DrawCircle），維持可無頭 spy 測試。
 */
void DrawSportsLapRing(nccu::engine::render::IRenderer& r,
                       const World& world,
                       float screenW,
                       float screenH);

}  // namespace nccu

#endif  // UI_HUD_SPORTS_LAP_RING_H_
