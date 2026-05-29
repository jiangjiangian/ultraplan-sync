#ifndef GAME_WORLD_HUD_TIMING_H_
#define GAME_WORLD_HUD_TIMING_H_

/**
 * @file HudTiming.h
 * @brief 遊戲層的 HUD 訊息計時常數（存留秒數與淡出長度）。
 */

namespace nccu {

/**
 * @brief HUD 橫幅在畫面上的存留秒數。
 *
 * 放在遊戲層而非 ui/MessageView.h，讓 World（在 HudExpired() / DismissHud()
 * 以 kHudTtl 對 HUD 訊息槽計齡）能讀取而不必引入 ui 渲染標頭，斷開
 * game→ui 的反向相依。ui/MessageView.h 仍負責定義 DrawHudMessage 並透過本
 * 標頭讀取相同的常數。
 */
inline constexpr float kHudTtl  = 4.0f;
/// @brief 尾端淡出長度（kHudTtl 的最後 kHudFade 秒）。在「減少動畫」無障礙設定下會塌縮為硬切。
inline constexpr float kHudFade = 1.0f;

} // namespace nccu

#endif // GAME_WORLD_HUD_TIMING_H_
