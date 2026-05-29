#ifndef ENDING_GATE_H_
#define ENDING_GATE_H_
class Player;                 // global-namespace model object
class EventBus;               // Plan P2: bus is injected, not pulled from Instance()
namespace nccu {
class SemesterStateMachine;
class DialogState;
// Flag-driven ending resolution, polled each non-dialog frame. ALL gated to
// Chapter4_Finals; precedence A -> B -> D -> C (first match wins), and the
// gate is TOTAL once Flag_TaFinaleChoiceMade is set (exactly one of the four
// fires — no soft-lock). Endings come from EITHER touching an umbrella
// (TookCursed -> B / BoughtUgly -> C) OR the 助教 finale (體諒 -> A or the
// 風雨同行 D / 質問 cold-finale -> B). Resolution is DEFERRED while a dialog
// is Active() so the closing 自白/finale narration is read first; on the
// transition it Close()s the conversation. (The old Ch1 ugly-buy -> C
// sibling-if is gone — the real Ending-C trigger is the Ch4 集英樓 Vendor.)
//
// Plan P2: `bus` is injected so the transition toast it publishes uses the
// caller's bus instance, NOT EventBus::Instance() at this layer. In
// production main.cpp owns the single bus; tests pass their own.
void CheckEndingGates(EventBus& bus, Player& player,
                      SemesterStateMachine& semester, DialogState& dialog);
}
#endif // ENDING_GATE_H_
