#ifndef REDUCED_MOTION_H_
#define REDUCED_MOTION_H_
#include <algorithm>

namespace nccu {

/**
 * @file ReducedMotion.h
 * @brief View／MessageView 共用的純動畫閘門，讓「減少動畫」無障礙偏好
 *        （World::ReducedMotion()）能在單一處關閉所有連續時間動畫。
 *
 * 每個 helper 都是單行自由函式、不依賴 raylib，故可無頭單元測試。預設行為
 * （reduced == false）與先前的內聯算式逐位元等價，因此接上它們不會擾動既有
 * 自動跑流程在「旗標關閉」預設情形下的決定性結果。
 */

/**
 * @brief Interlude 出口地面標記：本幀要累加的相位像素數。
 * @return 開啟減少動畫時回傳 0（凍結掃動，呼叫端以相位 0 畫虛線）。
 */
constexpr float InterludeMarkerPhaseStep(float dt, bool reduced) noexcept {
    return reduced ? 0.0f : dt * 30.0f;
}

/**
 * @brief 結局字卡淡入：預設約一秒內把 alpha 由 0 漸升到 1。
 * @return 開啟減少動畫時直接回傳 1（首幀即全不透明），使對閃光敏感的玩家
 *         免於半秒的亮度漸變。
 */
constexpr float EndingFadeAlphaStep(float currentAlpha,
                                    float dt, bool reduced) noexcept {
    return reduced ? 1.0f
                   : std::min(1.0f, currentAlpha + dt);
}

/**
 * @brief HUD 提示框（DrawHudMessage）末段淡出係數，落在 [0,1]。
 * @return 預設在最後 `fade` 秒內線性由 1 漸降到 0；減少動畫玩家則在 TTL
 *         邊界硬切（提示框維持全不透明，到時一幀內消失）。
 */
constexpr float HudToastFadeT(float remaining, float fade,
                              bool reduced) noexcept {
    if (reduced) return 1.0f;
    const float t = remaining / fade;
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

} // namespace nccu

#endif // REDUCED_MOTION_H_
