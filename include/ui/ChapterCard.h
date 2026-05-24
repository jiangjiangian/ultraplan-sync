#ifndef CHAPTER_CARD_H_
#define CHAPTER_CARD_H_
#include "state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
namespace gfx { class IRenderer; }

// U1-T2 — the chapter BOOKEND big card: a large, centred, brief
// full-attention beat distinct from the small transient HUD toast
// (ChapterToast.h / MessageView). Two flavours:
//
//   Lost  「傘又掉了」 — shown on every chapter START (Ch1 uses the apt
//                       inciting variant 「傘，不見了」). The inciting beat.
//   Found 「找到傘了」 — shown on every chapter END, AFTER the chapter's
//                       closing narration + reclaim beat (it rides the
//                       deferred-transition gap: a chapter only clears once
//                       its closing dialog has closed, so the card slots in
//                       as the FSM hops Chapter* -> Interlude_Market).
//
// Ch4 ends in an Ending screen (EndingView), so no Found card fires there.
//
// DESIGN — driven entirely render-side off the FSM the View already reads
// each frame (View compares world.Semester().Current() to its remembered
// previous state). NO new EventType, NO publish, NO model mutation: the
// card is pure View flourish exactly like endingAlpha_ / decorationClock_,
// so the autoplay harness state.jsonl is byte-IDENTICAL (the harness dumps
// pos/karma/flags/npcs/events/hud — never this card's timer). Determinism
// comes from Time::DeltaSeconds() (the fixed 1/60 step under the harness),
// so a scripted run renders the card at the same frames every replay.

enum class ChapterCardKind { None, Lost, Found };

// Which bookend card (if any) a SemesterState transition `from -> to`
// triggers. Pure classification:
//   to is a Chapter*                      -> Lost  (a chapter is starting)
//   from is Chapter1/2/3 and to==Interlude-> Found (a chapter just cleared)
//   anything involving an Ending_* state  -> None  (EndingView owns it)
//   to == Interlude from a non-chapter    -> None
[[nodiscard]] ChapterCardKind ChapterCardForTransition(
    SemesterState from, SemesterState to) noexcept;

// The big headline for a card. For Lost it depends on `to` (Ch1 gets the
// inciting 「傘，不見了」; Ch2-4 get 「傘又掉了」). For Found it is always
// 「找到傘了」. Empty for None. Every glyph is baked into gfx::Font.h
// UiLiteralChars() (the glyph-scan test covers ChapterCardStrings()).
[[nodiscard]] std::string_view ChapterCardHeadline(
    ChapterCardKind kind, SemesterState to) noexcept;

// A small one-line subtitle under the headline (the chapter name for a
// Lost card so the player knows which chapter began; a short coda for a
// Found card). Built from the destination state.
[[nodiscard]] std::string ChapterCardSubtitle(
    ChapterCardKind kind, SemesterState to);

// Every literal string a card can render, for the gfx::Font glyph-scan
// test (so a headline/subtitle glyph missing from the atlas fails the
// build's coverage check, never silently tofus). Pure data.
[[nodiscard]] std::vector<std::string> ChapterCardStrings();

// Render-side, deterministic timer state machine for one card. Holds the
// active kind + the headline/subtitle text + an elapsed-seconds clock.
// View owns ONE of these. Trigger() arms it on a chapter-boundary
// transition; Step() advances the fade-in / hold / fade-out each drawn
// frame; Dismiss() lets a key-press cut it short. No raylib, no GL — the
// fade is plain float math (mirrors endingAlpha_), so it is unit-testable.
class ChapterCardState {
public:
    // Total on-screen time and the symmetric fade windows (seconds). Hold
    // = kTotal - 2*kFade. ~2.2 s total reads as a brief beat without
    // stalling the player (the world keeps simulating underneath).
    static constexpr float kFade  = 0.3f;
    static constexpr float kTotal = 2.2f;

    [[nodiscard]] bool            Active()   const noexcept { return kind_ != ChapterCardKind::None; }
    [[nodiscard]] ChapterCardKind Kind()     const noexcept { return kind_; }
    [[nodiscard]] std::string_view Headline() const noexcept { return headline_; }
    [[nodiscard]] const std::string& Subtitle() const noexcept { return subtitle_; }
    [[nodiscard]] float           Elapsed()  const noexcept { return elapsed_; }

    // Fade level in [0,1]: ramps up over kFade, holds at 1, ramps down over
    // the final kFade. reducedMotion (audit D8 / SC 2.3.3) makes it opaque
    // for the whole life (no luminance ramp) then a hard cut at kTotal.
    [[nodiscard]] float Alpha(bool reducedMotion = false) const noexcept;

    // Arm the card for a fresh bookend beat (resets the clock). A kind of
    // None clears the card (used when a transition triggers nothing).
    void Trigger(ChapterCardKind kind, std::string_view headline,
                 std::string subtitle) noexcept;

    // Advance the clock by `dt`; auto-clears (->None) once kTotal elapses.
    // Idempotent no-op while inactive. Deterministic: dt is the harness
    // fixed step under the autoplay harness.
    void Step(float dt) noexcept;

    // Player skipped it (a key press): clear immediately.
    void Dismiss() noexcept;

private:
    ChapterCardKind kind_     = ChapterCardKind::None;
    std::string     headline_;
    std::string     subtitle_;
    float           elapsed_  = 0.0f;
};

// Draw the big card from its state (a no-op while inactive). A dimmed
// full-screen backdrop, a centred bold headline + subtitle, and a large
// umbrella-glyph cue (Lost -> the 破傘 ribs, "the umbrella is gone again";
// Found -> the 真傘 blue canopy, "you got it back"), the whole card fading
// with Alpha(). Self-contained and spy-testable like DrawEndingCard /
// DrawHudMessage. reducedMotion forwards to Alpha().
void DrawChapterCard(nccu::gfx::IRenderer& r, const ChapterCardState& card,
                     float screenW, float screenH,
                     bool reducedMotion = false);

} // namespace nccu
#endif // CHAPTER_CARD_H_
