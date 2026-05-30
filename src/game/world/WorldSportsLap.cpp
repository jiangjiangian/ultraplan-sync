#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter3Quest.h"          // kSportsTrackCx / Cy + kFlagSportsLapDone
#include "engine/math/Vec2.h"
#include <cmath>

/**
 * @file WorldSportsLap.cpp
 * @brief World 的校慶運動會跑圈邏輯：UpdateSportsLap 每幀累計繞圈角度並於滿圈時設旗標，
 *        兩個 const 取值器則把進度餵給 HUD 的跑圈環。
 *
 * 為控制單檔長度而與 World.cpp 拆到獨立 TU；皆為 World 的成員，宣告與成員存取皆不變。
 * 以 atan2 取角度、加減 π 求最短有號角差，避免跨越 ±π 邊界時誤算方向。
 */

namespace nccu {

void World::UpdateSportsLap() noexcept {
    if (semester_.Current() != SemesterState::Chapter3_SportsDay) return;
    if (!player_ || player_->HasFlag(kFlagSportsLapDone)) return;
    const float dx = player_->GetPosition().x - kSportsTrackCx;
    const float dy = player_->GetPosition().y - kSportsTrackCy;
    const float dist = std::hypot(dx, dy);
    // 只在位於／接近體育場跑道環帶時才累計——逗留在中心或遊蕩到離場地很遠處都不計入
    // 圈數。
    if (dist < 90.0f || dist > 320.0f) return;
    const float ang = std::atan2(dy, dx);
    if (!lapStarted_) {                       // 首個進入環帶的幀：定錨
        lapStarted_   = true;
        lapPrevAngle_ = ang;
        lapSwept_     = 0.0f;
        return;
    }
    constexpr float kPi = 3.14159265358979323846f;
    float d = ang - lapPrevAngle_;            // 取最短的有號角差
    while (d >  kPi) d -= 2.0f * kPi;
    while (d < -kPi) d += 2.0f * kPi;
    lapSwept_    += d;
    lapPrevAngle_ = ang;
    if (std::fabs(lapSwept_) >= 2.0f * kPi * 0.92f)   // 約一整圈（留 8% 寬容）
        player_->SetFlag(kFlagSportsLapDone);
}

float World::SportsLapProgress() const noexcept {
    if (player_ && player_->HasFlag(kFlagSportsLapDone)) return 1.0f;
    constexpr float kTwoPi = 6.28318530717958647692f;
    const float f = std::fabs(lapSwept_) / kTwoPi;
    return f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f);
}

bool World::SportsLapActive() const noexcept {
    return semester_.Current() == SemesterState::Chapter3_SportsDay
        && player_ != nullptr && !player_->HasFlag(kFlagSportsLapDone);
}


} // namespace nccu
