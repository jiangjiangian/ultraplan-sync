#ifndef UI_WORLD_SPORTS_LAP_TRACK_H_
#define UI_WORLD_SPORTS_LAP_TRACK_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file SportsLapTrack.h
 * @brief 操場校慶跑道地面貼花（自 View::RenderWorld 抽出）。
 */

/**
 * @brief 繪製操場跑道地貼：在玩家繞圈的場地上畫一個虛線「體育場」輪廓（跑道形
 *        狀：上下直道由左右半圓相接）；已通過的點會消失，使輪廓隨圈速完成而縮小
 *        （走完動態消除）。
 *
 * 由呼叫端在 CameraScope「之內」繪製，故為世界座標。必須在繪製順序掃描「之前」
 * 畫，使綜合院館（覆蓋操場東緣）與跑者疊在其上：即「地圖 → 線條 → 綜院」的分層
 * 需求。反應式：為 World::SportsLapActive() 與 SportsLapProgress() 的純函式；未
 * 進行圈速時提前返回。純渲染（MVC）、唯讀 world。
 */
void DrawSportsLapTrack(nccu::engine::render::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_WORLD_SPORTS_LAP_TRACK_H_
