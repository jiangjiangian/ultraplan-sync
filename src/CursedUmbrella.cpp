#include "CursedUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void CursedUmbrella::beClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // Fluent mutators — all return Player&, so the state changes read as
    // one atomic transaction. SetFlag is the ripple seed (F.9-b): taking
    // the cursed umbrella in Ch1 is what drives Ending B and the Ch2-4
    // 學霸/環境 cold reactions; without it that path was unreachable.
    player->SetHasUmbrella(true)
           .decreaseKarma(karmaPenalty_)
           .SetFlag("Flag_TookCursedUmbrella");
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
