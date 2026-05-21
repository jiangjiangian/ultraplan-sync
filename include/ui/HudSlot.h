#ifndef HUD_SLOT_H_
#define HUD_SLOT_H_

namespace nccu {

// Cycle 9.G — two independent HUD message channels so a chapter-clear
// toast and a regular ShowMessage (arrival hint / karma / pickup) can
// coexist on the same frame without clobbering each other. Before this,
// World held a single hudMessage_ slot; the 9.A.2 chapter toast lived
// 0.02 s (1 frame) at every Ch->IL transition because the IL arrival
// hint published the next frame and overwrote it (cycle9f-post-iteration-
// diagnosis §B). Plan A (publish-order swap from 9.B) only solved the
// TrueUmbrella vs chapter-clear race; the Ch->IL arrival hint clobber
// stayed live until Plan B (this) split the channel in two.
//
// `Top`    -> chapter / ending major-progress toasts. Rendered above
//             the bottom band (~25 px gap) so both lines are legible.
// `Bottom` -> everything else (pickup messages, karma deltas, arrival
//             hints, vendor purchase text, exit prep). The default for
//             the Event::slot field so existing publishers stay
//             behaviour-identical on the Bottom channel — only the
//             three Top-priority sites (ChapterToast / EndingGate /
//             future ending-screen toasts) opt in to Top.
//
// Pure data — no raylib, no rendering. Lives in its own tiny header so
// World / Event / MessageView / ChapterToast / EventWiring can include
// it without pulling in either the renderer surface or the EventBus.
enum class HudSlot {
    Top,
    Bottom,
};

} // namespace nccu

#endif // HUD_SLOT_H_
