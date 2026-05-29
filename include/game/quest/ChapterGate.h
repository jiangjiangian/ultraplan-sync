#ifndef CHAPTER_GATE_H_
#define CHAPTER_GATE_H_
class Player;                 // global-namespace model object
class EventBus;               // Plan P2: bus is injected, not Instance()
namespace nccu {
class SemesterStateMachine;
class DialogState;
// Flag-driven chapter <-> Interlude progression spine, checked right
// after a dialog choice is applied (sibling of CheckEndingGates, same
// place / same args). Builds the re-entrant transit core so the whole
// game is traversable Ch1 -> 市 -> Ch2 -> 市 -> Ch3 -> 市 -> Ch4:
//   Chapter2_Midterms + Flag_Ch2Cleared   -> Interlude (returnTo Ch3)
//   Chapter3_SportsDay + Flag_Ch3Cleared   -> Interlude (returnTo Ch4)
//   Interlude_Market   + Flag_LeaveInterlude -> the stored returnTo
// Sibling-if shape, exactly like CheckEndingGates / the 1c UmbrellaClaimed
// gate; future gates = add sibling ifs here, do not generalise. The Ch1
// -> Interlude entry stays in EventWiring.h's UmbrellaClaimed subscriber
// (it also seeds returnTo = Chapter2_Midterms); duplicating it here would
// double-fire. Ch4 -> endings is EndingGate's job (later S5e), not here.
// On any transition it Close()s the open conversation so a stale Active
// dialog can't keep eating input behind the next chapter.
// Plan P2: `bus` is injected so the transition toast it publishes uses
// the caller's bus instance, NOT EventBus::Instance() at this layer.
void CheckChapterGates(EventBus& bus, Player& player,
                       SemesterStateMachine& semester, DialogState& dialog);
}
#endif // CHAPTER_GATE_H_
