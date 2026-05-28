#include "entities/TrueUmbrella.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "quest/Flags.h"

void TrueUmbrella::beClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // B2.1: record WHICH umbrella is now in the bag (sets HasUmbrella too).
    player->SetHeldUmbrella(HeldUmbrella::True);
    // TrueUmbrella-specific marker (S5e-2b): HasUmbrella() is set by
    // EVERY umbrella, so it cannot tell Ending A's 持-TrueUmbrella
    // apart. This flag is TrueUmbrella-only; GameController clears it
    // on Ch4 entry (chapter4.md 傘再度失蹤), so in Ch4 it means exactly
    // "re-claimed the Ch4 TrueUmbrella" — Ending A's precise condition,
    // with no leak from a stray ctor Fragile/ProfTrap.
    player->SetFlag(nccu::kFlagHasTrueUmbrella);
    isActive_ = false; // mark for end-of-frame sweep
    // Publish ORDER matters (Cycle 9.B follow-up to 9.A.2): the
    // UmbrellaClaimed subscriber wired by EventWiring publishes the
    // chapter-clear ShowMessage ("✓ 章節清關 — 進入幕間市集") as its
    // side effect. Both ShowMessage payloads land on the same single-
    // slot HUD channel, so whichever publishes LAST is what the player
    // actually reads. ShowMessage first → UmbrellaClaimed second leaves
    // the chapter toast as the visible banner; the umbrella line gets
    // mirrored to cout for the log and to HUD for ~0 frames before the
    // chapter toast overwrites it. Reversing this pair re-introduces
    // the 9.A.2 regression where the umbrella string masked the chapter
    // toast. Pinned by tests/test_chapter_transitions.cpp.
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你撿到了 TrueUmbrella，雨停了。" });
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
}
