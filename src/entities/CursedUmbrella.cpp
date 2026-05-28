#include "entities/CursedUmbrella.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"
#include "quest/Flags.h"

void CursedUmbrella::beClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // Fluent mutators — all return Player&, so the state changes read as
    // one atomic transaction. SetFlag is the ripple seed (F.9-b): taking
    // the cursed umbrella in Ch1 is what drives Ending B and the Ch2-4
    // 學霸/環境 cold reactions; without it that path was unreachable.
    // B2.1: SetHeldUmbrella records the held kind AND sets HasUmbrella, so
    // the bag shows the 詛咒傘 row while it is actually held (and it clears
    // on a later SetHasUmbrella(false)). The Flag_TookCursedUmbrella ending
    // marker is set separately below and persists for the run (EndingGate B).
    player->SetHeldUmbrella(HeldUmbrella::Cursed)
           .decreaseKarma(karmaPenalty_)
           .SetFlag(nccu::kFlagTookCursedUmbrella);
    isActive_ = false;
    // KarmaChanged is no longer published here directly: as of Cycle
    // 9.B H5, Player::AddKarma (which decreaseKarma forwards to)
    // publishes KarmaChanged itself with a signed-delta payload, so a
    // second publish from this site would emit a duplicate "業力 -30"
    // toast for the same penalty. The cursed-umbrella narrative cue is
    // carried by the ShowMessage line below — the bus subscriber turns
    // the AddKarma side effect into the numeric toast, and this
    // ShowMessage delivers the human-readable explanation.
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "CursedUmbrella" });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你順手牽羊了！成為了你最討厭的人。" });
}
