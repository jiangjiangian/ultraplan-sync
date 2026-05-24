#include "ui/ChapterCard.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include "gfx/UmbrellaGlyph.h"
#include "dialog/DialogLayout.h"   // CellWidth — the project text-measure helper
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

namespace {

// Is `s` a chapter (not the Interlude / an ending)? A chapter START fires
// the Lost card; a chapter -> Interlude fires the Found card.
bool IsChapter(SemesterState s) noexcept {
    switch (s) {
        case SemesterState::Chapter1_AddDrop:
        case SemesterState::Chapter2_Midterms:
        case SemesterState::Chapter3_SportsDay:
        case SemesterState::Chapter4_Finals:
            return true;
        default:
            return false;
    }
}

// Horizontal centring without a renderer-side MeasureText (IRenderer has
// none — same constraint EndingView/MessageView work under). CellWidth is
// the project's East-Asian-Width text measure (CJK = 2 cells, ASCII = 1);
// at font size `sz` a glyph advances ~sz/2 px per cell, so the pixel width
// of `s` is ~CellWidth(s)*sz/2. Returns the x that centres it.
float CenteredX(std::string_view s, int sz, float screenW) {
    const float w = static_cast<float>(nccu::dialog::CellWidth(std::string{s})) *
                    (static_cast<float>(sz) * 0.5f);
    const float x = screenW * 0.5f - w * 0.5f;
    return x < 0.0f ? 0.0f : x;
}

} // namespace

ChapterCardKind ChapterCardForTransition(SemesterState from,
                                         SemesterState to) noexcept {
    if (from == to) return ChapterCardKind::None;     // not a real transition
    // A chapter is beginning (game start into Ch1, or an Interlude exit into
    // Ch2/3/4): the inciting "傘又掉了" beat.
    if (IsChapter(to)) return ChapterCardKind::Lost;
    // A chapter just cleared (its closing narration closed, the FSM now hops
    // to the market): the "找到傘了" payoff. Ch4 never reaches here — it
    // transitions to an Ending_* state, which EndingView owns.
    if (to == SemesterState::Interlude_Market && IsChapter(from))
        return ChapterCardKind::Found;
    return ChapterCardKind::None;
}

std::string_view ChapterCardHeadline(ChapterCardKind kind,
                                     SemesterState to) noexcept {
    switch (kind) {
        case ChapterCardKind::Lost:
            // Ch1 is the inciting incident — the player's own clear umbrella
            // vanished from the 綜院 傘架 (chapter1.md 開場), so an apt first
            // variant; Ch2-4 are the recurring "again" beat.
            return to == SemesterState::Chapter1_AddDrop
                       ? "傘，不見了"
                       : "傘又掉了";
        case ChapterCardKind::Found:
            return "找到傘了";
        case ChapterCardKind::None:
        default:
            return "";
    }
}

std::string ChapterCardSubtitle(ChapterCardKind kind, SemesterState to) {
    switch (kind) {
        case ChapterCardKind::Lost: {
            // Name the chapter that is starting so the big beat doubles as a
            // chapter title card. Glyphs are the chapter names (atlas-covered
            // via the building/help sets + the ChapterCardStrings scan).
            switch (to) {
                case SemesterState::Chapter1_AddDrop: return "第一章 加退選";
                case SemesterState::Chapter2_Midterms: return "第二章 期中考";
                case SemesterState::Chapter3_SportsDay: return "第三章 運動會";
                case SemesterState::Chapter4_Finals:   return "第四章 期末";
                default: return "";
            }
        }
        case ChapterCardKind::Found:
            // A short coda before the market interlude.
            return "這一章，過去了";
        case ChapterCardKind::None:
        default:
            return "";
    }
}

std::vector<std::string> ChapterCardStrings() {
    std::vector<std::string> out;
    const SemesterState states[] = {SemesterState::Chapter1_AddDrop,
                                    SemesterState::Chapter2_Midterms,
                                    SemesterState::Chapter3_SportsDay,
                                    SemesterState::Chapter4_Finals};
    for (SemesterState s : states) {
        out.emplace_back(ChapterCardHeadline(ChapterCardKind::Lost, s));
        out.push_back(ChapterCardSubtitle(ChapterCardKind::Lost, s));
    }
    out.emplace_back(
        ChapterCardHeadline(ChapterCardKind::Found, SemesterState::Interlude_Market));
    out.push_back(
        ChapterCardSubtitle(ChapterCardKind::Found, SemesterState::Interlude_Market));
    return out;
}

float ChapterCardState::Alpha(bool reducedMotion) const noexcept {
    if (kind_ == ChapterCardKind::None) return 0.0f;
    if (reducedMotion) return 1.0f;            // no luminance ramp, hard cut
    if (elapsed_ < kFade)                      // fade in
        return std::clamp(elapsed_ / kFade, 0.0f, 1.0f);
    const float fadeOutStart = kTotal - kFade;
    if (elapsed_ > fadeOutStart)               // fade out
        return std::clamp((kTotal - elapsed_) / kFade, 0.0f, 1.0f);
    return 1.0f;                               // hold
}

void ChapterCardState::Trigger(ChapterCardKind kind, std::string_view headline,
                               std::string subtitle) noexcept {
    kind_     = kind;
    headline_ = std::string{headline};
    subtitle_ = std::move(subtitle);
    elapsed_  = 0.0f;
}

void ChapterCardState::Step(float dt) noexcept {
    if (kind_ == ChapterCardKind::None) return;
    elapsed_ += dt;
    if (elapsed_ >= kTotal) Dismiss();
}

void ChapterCardState::Dismiss() noexcept {
    kind_    = ChapterCardKind::None;
    elapsed_ = 0.0f;
    headline_.clear();
    subtitle_.clear();
}

void DrawChapterCard(IRenderer& r, const ChapterCardState& card,
                     float screenW, float screenH, bool reducedMotion) {
    if (!card.Active()) return;
    const float alpha = card.Alpha(reducedMotion);
    if (alpha <= 0.0f) return;
    const auto a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);

    // 1) Full-screen dim so the beat reads as a deliberate interruption,
    //    distinct from the corner toast. Lighter than the ending backdrop
    //    (the world stays faintly visible underneath — this is a momentary
    //    beat, not a screen takeover).
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH},
               Color{0, 0, 0, static_cast<unsigned char>(alpha * 170.0f)});

    // 2) A centred banner panel — bigger/bolder than the toast box. Sized to
    //    a fixed band across the middle of the screen.
    const float panelH = 150.0f;
    const float panelY = screenH * 0.5f - panelH * 0.5f;
    r.DrawRect(Rect{0.0f, panelY, screenW, panelH},
               Color{16, 18, 26, static_cast<unsigned char>(alpha * 225.0f)});
    // Gold rules top & bottom so the band frames the headline.
    r.DrawRect(Rect{0.0f, panelY, screenW, 3.0f}, Color{255, 200, 70, a});
    r.DrawRect(Rect{0.0f, panelY + panelH - 3.0f, screenW, 3.0f},
               Color{255, 200, 70, a});

    // 3) The umbrella-glyph cue, left of the headline: Lost -> the 破傘 ribs
    //    ("gone again"); Found -> the 真傘 blue canopy ("got it back"). Same
    //    shared glyph the in-world umbrellas / pickups / ending card use, so
    //    the icon language is consistent everywhere. Alpha-scaled with fade.
    const UmbrellaLook look = (card.Kind() == ChapterCardKind::Found)
                                  ? UmbrellaLook::TrueBlue
                                  : UmbrellaLook::FragileBroken;
    constexpr float kUmbW = 46.0f, kUmbH = 50.0f;
    DrawUmbrellaGlyph(r, look,
                      Rect{screenW * 0.5f - 150.0f, screenH * 0.5f - kUmbH * 0.5f,
                           kUmbW, kUmbH}, a);

    // 4) The big bold headline (centred) + the smaller subtitle below it.
    constexpr int kHeadSize = 40;
    constexpr int kSubSize  = 18;
    const std::string_view head = card.Headline();
    const std::string&     sub  = card.Subtitle();
    // Found = warm gold (relief); Lost = a cooler off-white (the loss beat).
    const Color headColor = (card.Kind() == ChapterCardKind::Found)
                                ? Color{255, 210, 90, a}
                                : Color{235, 235, 240, a};
    const float headY = screenH * 0.5f - static_cast<float>(kHeadSize) * 0.5f - 12.0f;
    r.DrawText(std::string{head},
               Vec2{CenteredX(head, kHeadSize, screenW), headY},
               kHeadSize, headColor);
    if (!sub.empty()) {
        r.DrawText(sub, Vec2{CenteredX(sub, kSubSize, screenW), headY + 50.0f},
                   kSubSize, Color{200, 200, 210, a});
    }
}

} // namespace nccu
