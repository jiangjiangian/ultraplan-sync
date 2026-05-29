#include "game/entities/CursedUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/quest/Flags.h"

void CursedUmbrella::BeClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // P2 cursed-taint policy. Pickup no longer takes karma in one shot;
    // instead IncCursedTaint() bumps the counter and the per-chapter
    // ApplyCursedTaintDecay (SceneRouter Ch2/3/4 entry) bleeds -5 * taint
    // each transition. So the FIRST cursed pickup costs -5 per remaining
    // chapter transition (≤ -15 over Ch2/3/4 entries); a second pickup
    // raises the rate to -10/transition; a third to -15/transition — the
    // moral stain compounds with re-offending. Flag_TookCursedUmbrella is
    // still set unconditionally (Ending B precondition is unchanged), but
    // because A→B precedence holds in EndingGate, a player who later
    // earns karma > 80 + the gentle finale still REACHES Ending A — the
    // taint never hard-locks B, it just makes redemption mathematically
    // harder. SetHeldUmbrella records the kind for the bag row (B2.1).
    player->SetHeldUmbrella(HeldUmbrella::Cursed)
           .IncCursedTaint()
           .SetFlag(nccu::kFlagTookCursedUmbrella);
    isActive_ = false;
    // No KarmaChanged publish on this frame (taint decay handles karma at
    // chapter boundaries via AddKarma, which fires its own signed-delta
    // KarmaChanged). The pickup-time narrative cue stays as the same
    // ShowMessage line — the player still hears "成為了你最討厭的人" the
    // instant the deed is done.
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "CursedUmbrella" });
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你順手牽羊了！成為了你最討厭的人。" });
}
