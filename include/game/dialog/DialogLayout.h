#ifndef DIALOG_LAYOUT_H_
#define DIALOG_LAYOUT_H_
#include <cstddef>
#include <string>
#include <vector>

// Pure presentation helper (no raylib, no input, no World). Wraps a
// dialogue string to a fixed cell width and paginates the wrapped rows
// into fixed-height pages, so the dialog box NEVER overflows or clips no
// matter how long the authored line is (BUGLEDGER B4). Cell accounting
// follows Unicode East Asian Width — exactly the rule .claude/tools/
// dialog_lint.py:cell_width uses (full/wide/ambiguous = 2 cells, the
// rest = 1) — so the renderer's wrap is the single source of truth for
// "fits the box" and the linter can simply assert it produces no row
// wider than the box instead of flagging raw authored length.
namespace nccu::dialog {

// Visual width of `s` in full-width cells. ASCII / narrow = 1, CJK
// ideographs / full-width / ambiguous = 2, combining marks = 0. Mirrors
// dialog_lint.py's cell_width over the codepoint set this game uses.
[[nodiscard]] int CellWidth(const std::string& s);

// Greedy wrap of one logical line to `maxCells` visual cells per row.
// Breaks at ASCII spaces where the run has them (word wrap); otherwise
// breaks on UTF-8 codepoint boundaries (CJK has no spaces) — never
// inside a multibyte sequence and never producing a row wider than
// `maxCells` (a single glyph wider than the budget gets its own row).
// A literal '\n' forces a hard break. Returns at least one row (which
// may be empty for an empty input). Pure & directly unit-testable.
[[nodiscard]] std::vector<std::string> WrapToCells(const std::string& s,
                                                   int maxCells);

// Paginate `rows` into pages of at most `rowsPerPage` rows each. Always
// returns at least one page (possibly empty) so callers can index [0].
[[nodiscard]] std::vector<std::vector<std::string>>
Paginate(const std::vector<std::string>& rows, int rowsPerPage);

// Convenience: wrap then paginate in one call.
[[nodiscard]] std::vector<std::vector<std::string>>
LayoutPages(const std::string& s, int maxCells, int rowsPerPage);

// Dialog box geometry, in one place so View, DialogState pagination and
// the regression tests agree. Values chosen for the existing panel
// Rect{20,320,760,110} drawn at font size 16.
//
// kBoxCells = 80: a wrapped row should FILL the box to near its right
// edge before wrapping, not stop a third of the way across (the V-wrap
// bug — the old 28 wrapped at ~224px in a ~720px-wide text area). The
// text runs from kBoxTextX (36px); the "▼ more" cue draws at
// kBoxX+kBoxW-24 == 756px. Sizing from the rendered advance — a
// full-width glyph at font size 16 advances ~size + size/10 spacing
// ≈ 17.6px, i.e. ~8.8px per cell (the conservative end of the project's
// ~8px/cell model used by EndingView::CenteredX) — a full 80-cell CJK
// row ends at 36 + 80*8.8 ≈ 740px, ~16px clear of the ▼ at 756 and
// inside the 768px inner-right edge. (Pixel-unverifiable here: the font
// atlas is absent on a fresh clone, so the value is sized from the cell
// model with a safety margin rather than measured on screen.) Keep in
// sync with .claude/tools/dialog_lint.py MAX_CELLS and CLAUDE.md §6.
inline constexpr int   kBoxCells       = 80;
inline constexpr int   kBoxRowsPerPage = 3;
inline constexpr float kBoxX           = 20.0f;
inline constexpr float kBoxY           = 320.0f;
inline constexpr float kBoxW           = 760.0f;
inline constexpr float kBoxH           = 110.0f;
inline constexpr float kBoxTextX       = 36.0f;
inline constexpr float kBoxTextY       = 336.0f;
inline constexpr float kBoxLineH       = 22.0f;
inline constexpr int   kBoxFontSize    = 16;

} // namespace nccu::dialog
#endif // DIALOG_LAYOUT_H_
