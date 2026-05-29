#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter3Quest.h"          // kSportsTrackCx / Cy + kFlagSportsLapDone
#include "engine/math/Vec2.h"
#include <cmath>

// P4 step 2: the 3 校慶運動會 sports-lap helpers lifted out of World.cpp
// into their own TU — class members of World, IMPLEMENTATION only;
// declarations + member access are unchanged. UpdateSportsLap is called
// per-frame from SpawnSystem (sim pipeline) and reads/writes lap meter
// state on the player; the two const accessors feed the HUD lap ring.
// Zero behaviour change; the same atan2 + π wrap + lap-done flag write.

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
