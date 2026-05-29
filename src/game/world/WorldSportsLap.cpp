#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter3Quest.h"          // kSportsTrackCx / Cy + kFlagSportsLapDone
#include "engine/math/Vec2.h"
#include <cmath>

// 校慶運動會的三個跑圈輔助函式，從 World.cpp 抽到獨立 TU——它們是 World 的成員，
// 此處「只」放實作；宣告與成員存取皆未變動。UpdateSportsLap 由模擬管線每幀呼叫，
// 讀寫玩家身上的跑圈進度狀態；兩個 const 取值器則餵給 HUD 的跑圈環。
// 行為完全不變；仍是同樣的 atan2 + π 取最短角差 + 寫入跑圈完成旗標。

namespace nccu {

void World::UpdateSportsLap() noexcept {
    if (semester_.Current() != SemesterState::Chapter3_SportsDay) return;
    if (!player_ || player_->HasFlag(kFlagSportsLapDone)) return;
    const float dx = player_->GetPosition().x - kSportsTrackCx;
    const float dy = player_->GetPosition().y - kSportsTrackCy;
    const float dist = std::hypot(dx, dy);
    // Only sweep while on/near the stadium track band — loitering the
    // centre or wandering far off the field does not count toward the lap.
    if (dist < 90.0f || dist > 320.0f) return;
    const float ang = std::atan2(dy, dx);
    if (!lapStarted_) {                       // first on-band frame: anchor
        lapStarted_   = true;
        lapPrevAngle_ = ang;
        lapSwept_     = 0.0f;
        return;
    }
    constexpr float kPi = 3.14159265358979323846f;
    float d = ang - lapPrevAngle_;            // shortest signed step
    while (d >  kPi) d -= 2.0f * kPi;
    while (d < -kPi) d += 2.0f * kPi;
    lapSwept_    += d;
    lapPrevAngle_ = ang;
    if (std::fabs(lapSwept_) >= 2.0f * kPi * 0.92f)   // ~one lap (8% slack)
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
