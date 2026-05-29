#ifndef GFX_TIME_H_
#define GFX_TIME_H_
#include "raylib.h"

namespace nccu::engine::platform {

/**
 * @file Time.h
 * @brief 幀時間門面：正常遊玩取真實幀時間，自動遊玩可釘定固定步長以求可重播。
 */

/** @brief 全程序幀時間查詢的靜態門面。 */
struct Time {
    /**
     * @brief 釘定固定時間步長，使腳本化執行可重現。
     * @param[in] s 每幀秒數；<= 0（預設）表示改用真實 raylib 幀時間。
     *
     * 自動遊玩釘定固定步長，讓腳本化執行不受軟體 GL 幀率波動影響而可重現；
     * 未設定時正常遊玩行為不變。
     */
    static void  SetFixedStep(float s) noexcept { fixed_ = s; }
    /** @brief 取得本幀時間差（秒）。@return 已釘定固定步長則回傳該值，否則回傳真實幀時間。 */
    static float DeltaSeconds() noexcept {
        return fixed_ > 0.0f ? fixed_ : ::GetFrameTime();
    }
    /** @brief 取得平均 FPS。@return raylib 回報的當前平均每秒幀數。 */
    static int   FpsAvg() noexcept { return ::GetFPS(); }

private:
    inline static float fixed_ = 0.0f;   ///< 釘定的固定步長；<= 0 表示用真實幀時間
};

} // namespace nccu::engine::platform

#endif // GFX_TIME_H_
