#include "ui/EndingView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
using namespace nccu::game::gfx;  // game/gfx 輔助函式

using namespace nccu::engine::render;
using namespace nccu::engine::math;

bool IsEndingState(SemesterState s) noexcept {
    return s == SemesterState::Ending_A ||
           s == SemesterState::Ending_B ||
           s == SemesterState::Ending_D ||
           s == SemesterState::Ending_C;
}

namespace {

// The opening 字卡 of each ending, grounded in 遊戲企劃與敘事架構.md §陸:
//   A 結局A 完美通關 (True)  — 真相大白：雨過天晴，傘回到你手上。
//   B 結局B 屠龍者終成惡龍   — 詛咒傳承：the achievement card the GDD
//       literally specifies, 「你成為了你曾經最討厭的那種人」.
//   C 結局C 破財消災 (Normal) — unchanged, the opening 字卡 already in
//       docs/content/ending_c.md §2.
// Kept short (≤ the dialog-box feel) and free of any external tool /
// model brand string (forbidden-string rule).
std::string_view caption(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A:
            return "「雨過天晴，傘還在你手上。」";
        case SemesterState::Ending_B:
            return "「你成為了你曾經最討厭的那種人」";
        case SemesterState::Ending_D:
            return "「傘破了，但你沒丟下任何人。」";
        case SemesterState::Ending_C:
            return "「這樣以後再也不會有人拿錯你的傘了。」";
        default:
            return "";
    }
}

// Item 1 — the in-fiction "why you're here" STORY lines, transcribed
// concise from the `> 字卡：「…」` blocks at the top of docs/content/
// ending_{a,b,c}.md (the .md stays the narrative bible; THIS table is
// what actually renders, because View.cpp early-returns before the .md
// is ever drawn). Each line is short enough for the screen width; every
// glyph here is baked into nccu::engine::render::Font.h UiLiteralChars() (5c) so the card
// never tofus. 3 lines/ending keeps the card inside the 450px viewport.
const std::vector<std::string>& reasonLines(SemesterState s) {
    static const std::vector<std::string> kA = {
        "你把不屬於你的傘，一把一把都還了回去。",
        "面對抱錯傘的人，你沒有把氣討回來，",
        "只說了一句「辛苦了」。雨，就這樣停了。",
    };
    static const std::vector<std::string> kB = {
        "該放手的時候你沒放，",
        "該體諒的時候你選了把氣討回來。",
        "到頭來，你也成了會在傘架前伸手的人。",
    };
    static const std::vector<std::string> kC = {
        "你沒有去追那把丟掉的傘，",
        "也沒有計較是誰拿走的——",
        "你掏光口袋的錢，買過了這整場雨。",
    };
    // Ending_D (G1) — 風雨同行: chose 體諒 but did not earn A. A kind
    // heart, but the umbrella was worn through by a whole semester of rain.
    static const std::vector<std::string> kD = {
        "你選擇了體諒，沒把這口氣討回來。",
        "只是這學期的風吹雨打，把傘磨破了——",
        "傘骨還在，雨會滲進來，但你撐著它走完了。",
    };
    static const std::vector<std::string> kNone;
    switch (s) {
        case SemesterState::Ending_A: return kA;
        case SemesterState::Ending_B: return kB;
        case SemesterState::Ending_D: return kD;
        case SemesterState::Ending_C: return kC;
        default:                      return kNone;
    }
}

// Path label per ending (the 結算 card footer) — the GDD's own naming.
std::string_view pathLabel(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A: return "完美結局（True）";
        case SemesterState::Ending_B: return "墮落結局";
        case SemesterState::Ending_D: return "風雨同行結局";
        case SemesterState::Ending_C: return "務實結局（Normal）";
        default:                      return "";
    }
}

// Build the SHORT checklist of conditions that ACTUALLY fired for this
// run — mirrors EndingGate.cpp's per-ending logic so the card explains
// the verdict, not a generic rubric. Each entry is prefixed with the ✓
// "met" mark (U+2713, baked into the atlas per 5c). Only fired
// conditions appear (B/C have multiple possible triggers; show those
// that hold). A always shows its full 3-clause AND.
std::vector<std::string> conditionsFired(const EndingSummary& g) {
    std::vector<std::string> out;
    const std::string mark = "\xE2\x9C\x93 ";   // U+2713 ✓ + space
    switch (g.state) {
        case SemesterState::Ending_A:
            // EndingGate A: karma>80 && Flag_HasTrueUmbrella &&
            // Flag_ConsoledTA — all three are the AND, always shown.
            out.push_back(mark + "業力 > 80");
            out.push_back(mark + "還回真傘");
            out.push_back(mark + "體諒助教");
            break;
        case SemesterState::Ending_B: {
            // EndingGate B: Flag_TookCursedUmbrella || karma<0 ||
            // coldFinale(finaleChoiceMade && !consoledTA). Show the
            // disjuncts that actually hold.
            if (g.tookCursed) out.push_back(mark + "拿了詛咒傘");
            if (g.karma < 0)  out.push_back(mark + "業力低於零");
            if (g.finaleChoiceMade && !g.consoledTA)
                out.push_back(mark + "最後質問助教");
            break;
        }
        case SemesterState::Ending_D:
            // EndingGate D (G1): Flag_ConsoledTA reached here ⇒ 體諒 but
            // not A (karma ≤ 80) and not B. Show the two clauses that
            // landed the player on the 風雨同行 path.
            out.push_back(mark + "體諒助教");
            out.push_back(mark + "業力 ≤ 80（差一點點圓滿）");
            break;
        case SemesterState::Ending_C:
            // EndingGate C: Flag_BoughtUglyUmbrella ||
            // Flag_TaFinaleChoiceMade (the平穩收尾 default). Show which.
            if (g.boughtUgly) out.push_back(mark + "買了醜傘");
            if (g.finaleChoiceMade && !g.boughtUgly)
                out.push_back(mark + "平穩收尾（未體諒、未買綠傘）");
            break;
        default:
            break;
    }
    return out;
}

// Horizontal centring without a renderer-side MeasureText (IRenderer
// exposes none — see MessageView.cpp). nccu::dialog::CellWidth is the
// project's font-independent text-measure helper (East-Asian-Width: CJK
// = 2 cells, ASCII = 1), the same cell estimation View.cpp uses for the
// objective panel. At font size `sz` a full-width glyph advances ~`sz`
// px and a narrow one ~`sz/2`, i.e. ~`sz/2` px per cell — so the pixel
// width of `s` is ≈ CellWidth(s) * sz/2. Returns the x that centres it.
float CenteredX(const std::string& s, int sz, float screenW) {
    const float w = static_cast<float>(nccu::dialog::CellWidth(s)) *
                    (static_cast<float>(sz) * 0.5f);
    const float x = screenW * 0.5f - w * 0.5f;
    return x < 0.0f ? 0.0f : x;
}

// Pixel width estimate of `s` at font size `sz` (the cell model above) —
// used to size the 結算 panel so it hugs its widest row.
float TextWidthPx(const std::string& s, int sz) {
    return static_cast<float>(nccu::dialog::CellWidth(s)) *
           (static_cast<float>(sz) * 0.5f);
}

// UI-B-3: how many EAW cells fit in `widthPx` at font size `sz`, given the
// shared ~sz/2 px-per-cell model. Used to wrap long card text so it can
// never spill the black box (the cards were hand-fitted to 800×450 with no
// wrap). Floors to at least 1 so an absurdly narrow box still emits rows.
int CellsForWidth(float widthPx, int sz) {
    const float perCell = static_cast<float>(sz) * 0.5f;
    return std::max(1, static_cast<int>(widthPx / perCell));
}

// Wrap `s` to the on-screen text width (a margin in from both edges) using
// the project's EAW-aware nccu::dialog::WrapToCells, then draw each row
// centred at `y`, advancing by `lineH`. Returns the y AFTER the last row.
// Keeps every card row inside the panel without a renderer-side MeasureText
// (IRenderer has none — same constraint the rest of this file works under).
float DrawCenteredWrapped(IRenderer& r, const std::string& s, int sz,
                          float screenW, float marginPx, float y,
                          float lineH, Color col) {
    const float textW = std::max(40.0f, screenW - marginPx * 2.0f);
    for (const std::string& row :
         nccu::dialog::WrapToCells(s, CellsForWidth(textW, sz))) {
        r.DrawText(row, Vec2{CenteredX(row, sz, screenW), y}, sz, col);
        y += lineH;
    }
    return y;
}

// Ending B is the 灰暗 (grey-toned) timeline per the GDD ("畫面色調永久
// 轉為灰暗"): desaturate the otherwise-white title/caption toward grey so
// the bad ending reads visibly colder than A/C. Pure presentation.
Color endingTextColor(SemesterState s, unsigned char a) {
    if (s == SemesterState::Ending_B) return Color{150, 150, 155, a};
    return Color{255, 255, 255, a};
}

// The owner's "Ch4 結局顯示最終雨傘樣貌": the card shows the umbrella the run
// ENDED with, drawn with the SAME shared glyph the in-world umbrellas / the
// pickup use. Keying it off the ENDING (not raw flags) guarantees it can never
// mismatch the verdict — the fix for "體諒卻顯示醜傘":
//   A 完美結局 → 真傘 (blue)   B 墮落結局 → 詛咒傘 (dark purple)
//   C 務實結局 → 醜傘 (green)
[[nodiscard]] nccu::game::gfx::UmbrellaLook endingUmbrellaLook(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A: return nccu::game::gfx::UmbrellaLook::TrueBlue;
        case SemesterState::Ending_B: return nccu::game::gfx::UmbrellaLook::CursedPurple;
        // Ending_D (G1): the 破傘 — kind heart, but the canopy is gone and
        // only the bent ribs/handle remain (the owner's "風吹雨打把傘磨破了").
        case SemesterState::Ending_D: return nccu::game::gfx::UmbrellaLook::FragileBroken;
        case SemesterState::Ending_C: return nccu::game::gfx::UmbrellaLook::UglyGreen;
        default:                      return nccu::game::gfx::UmbrellaLook::TrueBlue;
    }
}

// A-T3: the bottom-menu label table — the single source of truth for the
// three option strings. Kept here (not inline literals) so EndingCardStrings
// can feed them to the glyph-scan test and the renderer reads the same text.
std::string_view endingMenuLabel(EndingMenuChoice c) {
    switch (c) {
        case EndingMenuChoice::BackToTitle: return "回首頁";
        case EndingMenuChoice::RestartGame: return "重新開始";
        case EndingMenuChoice::Quit:        return "結束";
    }
    return "";
}

}  // namespace

EndingMenuChoice EndingMenuChoiceAt(int index) noexcept {
    // Clamp into the valid set so a stray cursor never selects "nothing".
    switch (((index % 3) + 3) % 3) {
        case 0:  return EndingMenuChoice::BackToTitle;
        case 1:  return EndingMenuChoice::RestartGame;
        default: return EndingMenuChoice::Quit;
    }
}

std::string_view EndingMenuLabel(EndingMenuChoice c) noexcept {
    return endingMenuLabel(c);
}

std::vector<std::string> EndingCardStrings() {
    std::vector<std::string> out;
    // Static, state-independent strings drawn on every card.
    out.emplace_back("── 為什麼你走到這裡 ──");
    out.emplace_back("結算");
    out.emplace_back("業力 karma：0");   // the karma label glyphs (digits ASCII)
    out.emplace_back("結局類型：");
    // A-T3: the bottom-menu option labels + the nav hint, so the glyph-scan
    // test (test_font_ui_glyph_scan.cpp scans EndingCardStrings) verifies
    // every menu glyph is baked into the atlas — no silent tofu.
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::BackToTitle)));
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::RestartGame)));
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::Quit)));
    out.emplace_back("← → 選擇   E 確認");
    const SemesterState states[] = {SemesterState::Ending_A,
                                    SemesterState::Ending_B,
                                    SemesterState::Ending_D,
                                    SemesterState::Ending_C};
    for (SemesterState s : states) {
        out.emplace_back(caption(s));
        out.emplace_back(pathLabel(s));
        for (const std::string& ln : reasonLines(s)) out.push_back(ln);
        // Force EVERY condition branch to fire so every label is captured
        // (the live card shows only the fired subset; the test needs all).
        EndingSummary g;
        g.state            = s;
        g.karma            = -1;     // makes B's "業力低於零" fire
        g.hasTrueUmbrella  = true;
        g.consoledTA       = false;  // makes B/C's finale branches fire
        g.tookCursed       = true;
        g.boughtUgly       = true;
        g.finaleChoiceMade = true;
        for (const std::string& c : conditionsFired(g)) out.push_back(c);
        // C's "平穩收尾" only fires when !boughtUgly — capture it too.
        EndingSummary g2 = g;
        g2.boughtUgly = false;
        for (const std::string& c : conditionsFired(g2)) out.push_back(c);
    }
    return out;
}

void DrawEndingCard(IRenderer& r, const EndingSummary& summary,
                    std::string_view title, float alpha,
                    float screenW, float screenH, int menuCursor) {
    alpha = std::min(1.0f, std::max(0.0f, alpha));
    const unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
    const SemesterState state = summary.state;

    // Self-contained fade: the backdrop carries the same alpha so the
    // spy test sees a real card even though View also early-returns.
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, a});

    // ---- The final umbrella (owner: "Ch4 結局顯示最終雨傘樣貌") --------
    // A hero swatch of the umbrella this run ended with, drawn with the
    // SAME shared glyph the in-world umbrellas + the pickup use, keyed off
    // the ENDING so it can never mismatch the verdict (A 真傘藍 / B 詛咒傘
    // 暗紫 / C 醜傘綠). Centred at the top, alpha-scaled with the card fade.
    constexpr float kUmbW = 56.0f;
    constexpr float kUmbH = 50.0f;
    nccu::game::gfx::DrawUmbrellaGlyph(
        r, endingUmbrellaLook(state),
        Rect{screenW * 0.5f - kUmbW * 0.5f, 8.0f, kUmbW, kUmbH}, a);

    // ---- Title + opening 字卡 (centred via the cell model) ------------
    // Ending B is greyed per the GDD; A/C stay white. Sizes/positions
    // chosen for the 800x450 window so the umbrella swatch, title, caption,
    // reason block and 結算 card all fit one screen (single-screen —
    // preserves the simple endingAlpha_ fade; no paging needed).
    constexpr int kTitleSize   = 28;
    constexpr int kCaptionSize = 18;
    constexpr int kReasonSize  = 16;
    constexpr int kStatSize    = 15;
    const Color tint = endingTextColor(state, a);
    const std::string ttl{title};
    const std::string cap{caption(state)};
    // UI-B-3: a side margin every card row wraps inside, so even a long
    // caption / reason line auto-wraps within the black box instead of
    // running off the 800px edge. The title stays one line (it is short by
    // construction and centred).
    constexpr float kSideMargin = 28.0f;
    r.DrawText(ttl, Vec2{CenteredX(ttl, kTitleSize, screenW), 64.0f},
               kTitleSize, tint);
    DrawCenteredWrapped(r, cap, kCaptionSize, screenW, kSideMargin, 100.0f,
                        22.0f, tint);

    // ---- "why you're here" reason block (the STORY lines) -------------
    // A faint section label, then the 2-3 in-fiction lines, centred and
    // wrapped so a long line never spills the box.
    float y = 136.0f;
    {
        const std::string hdr = "── 為什麼你走到這裡 ──";
        r.DrawText(hdr, Vec2{CenteredX(hdr, kReasonSize, screenW), y},
                   kReasonSize, Color{200, 200, 205, a});
        y += 26.0f;
        for (const std::string& ln : reasonLines(state))
            y = DrawCenteredWrapped(r, ln, kReasonSize, screenW, kSideMargin,
                                    y, 22.0f, tint);
    }

    // ---- 結算 stats card: a panel hugging the karma + checklist -------
    // Centred panel below the reason block. Width is the widest row by
    // the cell model (cannot MeasureText here — same constraint as the
    // HUD); height fits the header + karma + each fired condition + the
    // path label. Built render-only from the DTO.
    const std::string statHdr = "結算";
    char kbuf[32] = {0};
    std::snprintf(kbuf, sizeof(kbuf), "業力 karma：%d", summary.karma);
    const std::string karmaLine = kbuf;
    const std::vector<std::string> conds = conditionsFired(summary);
    const std::string path = std::string("結局類型：") +
                             std::string(pathLabel(state));

    // Widest row drives the panel width.
    float contentW = std::max(TextWidthPx(karmaLine, kStatSize),
                              TextWidthPx(path, kStatSize));
    contentW = std::max(contentW, TextWidthPx(statHdr, kStatSize + 3));
    for (const std::string& c : conds)
        contentW = std::max(contentW, TextWidthPx(c, kStatSize));

    constexpr float kPad = 14.0f;
    // Rows: header, karma, each condition, path.
    const int rows = 3 + static_cast<int>(conds.size());
    const float rowH   = 22.0f;
    const float panelW = contentW + kPad * 2.0f;
    const float panelH = static_cast<float>(rows) * rowH + kPad * 2.0f;
    float panelX = screenW * 0.5f - panelW * 0.5f;
    if (panelX < 8.0f) panelX = 8.0f;
    // Sit the card just under the reason block; clamp so it never spills
    // past the bottom edge of the 450px window.
    float panelY = y + 12.0f;
    const float maxPanelY = screenH - panelH - 8.0f;
    if (panelY > maxPanelY) panelY = maxPanelY < 0.0f ? 0.0f : maxPanelY;

    // Panel backing — a touch lighter than the pure-black backdrop so the
    // card reads as a distinct element; alpha-scaled with the fade.
    const unsigned char panelA = static_cast<unsigned char>(alpha * 220.0f);
    r.DrawRect(Rect{panelX, panelY, panelW, panelH},
               Color{28, 30, 40, panelA});

    float sy = panelY + kPad;
    // Header (gold so the 結算 card is visually anchored).
    r.DrawText(statHdr, Vec2{panelX + kPad, sy}, kStatSize + 3,
               Color{255, 200, 70, a});
    sy += rowH;
    r.DrawText(karmaLine, Vec2{panelX + kPad, sy}, kStatSize, tint);
    sy += rowH;
    for (const std::string& c : conds) {
        r.DrawText(c, Vec2{panelX + kPad, sy}, kStatSize,
                   Color{120, 230, 140, a});   // green = condition met
        sy += rowH;
    }
    r.DrawText(path, Vec2{panelX + kPad, sy}, kStatSize,
               Color{200, 200, 205, a});

    // ---- A-T3: the bottom 3-option menu (回首頁 / 重新開始 / 結束) -------
    // The ending screen is now a STABLE interactive screen; this row is the
    // player's only agency here. ←/→ move the cursor (handled in
    // GameController), E/Enter confirm → World::PendingAppAction. The View
    // ONLY renders: the highlighted option gets a caret + gold; the others
    // are dim. Laid out horizontally centred near the bottom edge so it
    // never collides with the 結算 panel (which is clamped above
    // screenH - panelH - 8). Drawn at full card alpha so it fades IN with
    // the rest of the screen and is steady once shown.
    constexpr int   kMenuSize = 18;
    constexpr float kHintSize = 13.0f;
    constexpr int   kMenuItems = 3;   // |EndingMenuChoice|
    const int cursor = ((menuCursor % kMenuItems) + kMenuItems) % kMenuItems;
    // Build the three labels with a leading caret on the selected one so
    // the selection is unambiguous even in greyscale (audit-style a11y).
    // The caret is the ASCII "> " the pause menu uses (always in the atlas).
    std::string opts[kMenuItems];
    float optW[kMenuItems];
    float totalW = 0.0f;
    constexpr float kGap = 26.0f;
    for (int i = 0; i < kMenuItems; ++i) {
        const std::string lbl{endingMenuLabel(EndingMenuChoiceAt(i))};
        opts[i] = (i == cursor ? std::string("> ") : std::string("  ")) + lbl;
        optW[i] = TextWidthPx(opts[i], kMenuSize);
        totalW += optW[i];
    }
    totalW += kGap * 2.0f;
    // Place the row near the bottom; a thin panel behind it so it reads as
    // a control bar distinct from the story text above.
    const float menuH = static_cast<float>(kMenuSize) + 14.0f;
    const float menuY = screenH - menuH - 22.0f;
    const float barX  = screenW * 0.5f - totalW * 0.5f - 12.0f;
    const float barW  = totalW + 24.0f;
    r.DrawRect(Rect{barX, menuY - 5.0f, barW, menuH},
               Color{18, 20, 28, static_cast<unsigned char>(alpha * 220.0f)});
    float ox = screenW * 0.5f - totalW * 0.5f;
    for (int i = 0; i < kMenuItems; ++i) {
        const Color c = (i == cursor) ? Color{255, 200, 70, a}    // gold = selected
                                      : Color{170, 170, 180, a};  // dim = unselected
        r.DrawText(opts[i], Vec2{ox, menuY}, kMenuSize, c);
        ox += optW[i] + kGap;
    }
    // Nav hint just below the option row.
    const std::string hint = "← → 選擇   E 確認";
    r.DrawText(hint,
               Vec2{CenteredX(hint, static_cast<int>(kHintSize), screenW),
                    menuY + static_cast<float>(kMenuSize) + 2.0f},
               static_cast<int>(kHintSize), Color{150, 150, 160, a});
}

} // namespace nccu
