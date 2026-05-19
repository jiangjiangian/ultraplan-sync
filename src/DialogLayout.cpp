#include "DialogLayout.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace nccu::dialog {

namespace {

// Length in bytes of the UTF-8 sequence whose lead byte is `b`. 1 for
// ASCII, 3 for the CJK BMP block this game's strings live in. Mirrors
// MessageView.cpp's Utf8Len so the two wrappers agree byte-for-byte.
std::size_t Utf8Len(unsigned char b) noexcept {
    if (b < 0x80) return 1;
    if ((b >> 5) == 0x6) return 2;
    if ((b >> 4) == 0xE) return 3;
    if ((b >> 3) == 0x1E) return 4;
    return 1;  // invalid lead → one byte, never loop forever
}

// Decode the UTF-8 sequence of length `n` starting at &s[i] into a
// codepoint. Assumes i+n <= s.size() (the callers guarantee it).
std::uint32_t DecodeUtf8(const std::string& s, std::size_t i,
                         std::size_t n) noexcept {
    const auto b0 = static_cast<unsigned char>(s[i]);
    if (n == 1) return b0;
    std::uint32_t cp = 0;
    if (n == 2) cp = b0 & 0x1Fu;
    else if (n == 3) cp = b0 & 0x0Fu;
    else cp = b0 & 0x07u;
    for (std::size_t k = 1; k < n; ++k)
        cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3Fu);
    return cp;
}

// Combining mark? (zero visual cells). Python's cell_width skips
// unicodedata.combining(ch); this game's content only uses the
// combining-diacritical block, which is sufficient here.
bool IsCombining(std::uint32_t cp) noexcept {
    return cp >= 0x0300u && cp <= 0x036Fu;
}

// East-Asian-Width "wide": codepoint occupies 2 cells. The set is the
// union of Unicode EAW W / F / A (ambiguous counted as wide — exactly
// dialog_lint.py's `in ("W","F","A")`). Enumerated for the ranges this
// game's Traditional-Chinese content + UI literals actually use; any
// codepoint outside these is treated as 1 cell (narrow), matching the
// linter for the practical character set. Verified against Python
// unicodedata.east_asian_width over docs/content/*.md.
bool IsWide(std::uint32_t cp) noexcept {
    struct Range { std::uint32_t lo, hi; };
    static constexpr std::array<Range, 17> kWide = {{
        {0x00A1u, 0x00A1u},  // ¡            (Ambiguous)
        {0x00A4u, 0x00A4u},  // ¤            (Ambiguous)
        {0x00A7u, 0x00A8u},  // §¨           (Ambiguous)
        {0x00B0u, 0x00B1u},  // °±           (Ambiguous)
        {0x00B7u, 0x00B7u},  // ·            (Ambiguous)
        {0x00D7u, 0x00D7u},  // ×            (Ambiguous)
        {0x00F7u, 0x00F7u},  // ÷            (Ambiguous)
        {0x2010u, 0x2027u},  // ‐ … general punctuation (Ambiguous)
        {0x2030u, 0x205Fu},  // ‰ … incl. — “ ” ‘ ’ … (Ambiguous)
        {0x2190u, 0x2199u},  // ← ↑ → ↓ arrows  (Ambiguous)
        {0x2460u, 0x24FFu},  // enclosed alphanumerics (Ambiguous)
        {0x25A0u, 0x26FFu},  // geometric/misc symbols, ▼ ▶ etc. (A/W)
        {0x2E80u, 0x303Eu},  // CJK radicals … CJK symbols & punctuation
        {0x3041u, 0x33FFu},  // kana, CJK compat (Wide)
        {0x3400u, 0x4DBFu},  // CJK Ext-A (Wide)
        {0x4E00u, 0x9FFFu},  // CJK Unified Ideographs (Wide)
        {0xF900u, 0xFAFFu},  // CJK Compatibility Ideographs (Wide)
    }};
    static constexpr std::array<Range, 3> kWide2 = {{
        {0xFE30u, 0xFE4Fu},  // CJK compatibility forms (Wide)
        {0xFF00u, 0xFF60u},  // fullwidth forms （）！？： etc. (Fullwidth)
        {0xFFE0u, 0xFFE6u},  // fullwidth currency/sign (Fullwidth)
    }};
    for (const Range& r : kWide)
        if (cp >= r.lo && cp <= r.hi) return true;
    for (const Range& r : kWide2)
        if (cp >= r.lo && cp <= r.hi) return true;
    return false;
}

// Cells for one codepoint: 0 combining, 2 wide, else 1.
int CellsOf(std::uint32_t cp) noexcept {
    if (IsCombining(cp)) return 0;
    return IsWide(cp) ? 2 : 1;
}

} // namespace

int CellWidth(const std::string& s) {
    int w = 0;
    for (std::size_t i = 0; i < s.size();) {
        const std::size_t n =
            std::min(Utf8Len(static_cast<unsigned char>(s[i])),
                     s.size() - i);
        w += CellsOf(DecodeUtf8(s, i, n));
        i += n;
    }
    return w;
}

std::vector<std::string> WrapToCells(const std::string& s, int maxCells) {
    if (maxCells < 1) maxCells = 1;
    std::vector<std::string> rows;
    std::string row;
    int rowW = 0;
    // True once a SOFT wrap (width overflow) has just started a fresh
    // row — a stray space at the head of a wrapped row is an artifact
    // and is dropped. An author's intentional leading indent on the
    // original line / after a hard '\n' is preserved (the choice-mark
    // "  "/"> " alignment depends on this).
    bool softWrapped = false;
    // Pending word buffer: ASCII run since the last space, so a word
    // never splits mid-token when spaces are present (word wrap).
    std::string word;
    int wordW = 0;

    auto flushRow = [&](bool soft) {
        // A trailing space left at a soft-wrap boundary is an artifact
        // (the word after it moved to the next row) — drop it so a
        // word-wrapped row has no dangling space. An author's hard
        // '\n'-terminated row keeps its content verbatim.
        if (soft)
            while (!row.empty() && row.back() == ' ') {
                row.pop_back();
                --rowW;
            }
        rows.push_back(row);
        row.clear();
        rowW = 0;
        softWrapped = soft;
    };
    // Place the pending word onto rows, wrapping it if it alone is
    // wider than the budget (the CJK / overlong-token path).
    auto placeWord = [&]() {
        if (word.empty()) return;
        if (rowW > 0 && rowW + wordW > maxCells) flushRow(true);
        for (std::size_t i = 0; i < word.size();) {
            const std::size_t n =
                std::min(Utf8Len(static_cast<unsigned char>(word[i])),
                         word.size() - i);
            const int c = CellsOf(DecodeUtf8(word, i, n));
            if (rowW > 0 && rowW + c > maxCells) flushRow(true);
            row.append(word, i, n);
            rowW += c;
            i += n;
        }
        word.clear();
        wordW = 0;
    };

    for (std::size_t i = 0; i < s.size();) {
        if (s[i] == '\n') {           // hard break (author intent)
            placeWord();
            flushRow(false);
            ++i;
            continue;
        }
        const std::size_t n =
            std::min(Utf8Len(static_cast<unsigned char>(s[i])),
                     s.size() - i);
        const std::uint32_t cp = DecodeUtf8(s, i, n);
        const int c = CellsOf(cp);

        if (n == 1 && s[i] == ' ') {
            // Space terminates the current word; emit the word, then
            // the space itself participates in row fill (collapses at
            // a wrap so a line never starts with a stray space).
            placeWord();
            // Drop a space only at the head of a SOFT-wrapped row
            // (artifact); keep an intentional leading indent otherwise.
            if (rowW == 0 && softWrapped) { i += n; continue; }
            if (rowW + 1 > maxCells) { flushRow(true); i += n; continue; }
            row.push_back(' ');
            rowW += 1;
            i += n;
            continue;
        }
        if (n == 1 && cp != 0 && cp < 0x80u &&
            (cp == '-' || (cp >= '0' && cp <= '9') ||
             (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'))) {
            // Part of an ASCII word: accumulate, don't break inside it
            // unless the word alone exceeds the budget.
            if (wordW + c > maxCells) placeWord();
            word.append(s, i, n);
            wordW += c;
            i += n;
            continue;
        }
        // CJK / punctuation: a break-anywhere unit. Flush any pending
        // ASCII word first, then place this glyph (wrapping if full).
        placeWord();
        if (rowW > 0 && rowW + c > maxCells) flushRow(true);
        row.append(s, i, n);
        rowW += c;
        i += n;
    }
    placeWord();
    rows.push_back(row);            // trailing row (>=1 row guaranteed)
    return rows;
}

std::vector<std::vector<std::string>>
Paginate(const std::vector<std::string>& rows, int rowsPerPage) {
    if (rowsPerPage < 1) rowsPerPage = 1;
    std::vector<std::vector<std::string>> pages;
    for (std::size_t i = 0; i < rows.size(); i += static_cast<std::size_t>(
                                                  rowsPerPage)) {
        const std::size_t end =
            std::min(rows.size(), i + static_cast<std::size_t>(rowsPerPage));
        pages.emplace_back(rows.begin() + static_cast<std::ptrdiff_t>(i),
                           rows.begin() + static_cast<std::ptrdiff_t>(end));
    }
    if (pages.empty()) pages.emplace_back();   // always >=1 page
    return pages;
}

std::vector<std::vector<std::string>>
LayoutPages(const std::string& s, int maxCells, int rowsPerPage) {
    return Paginate(WrapToCells(s, maxCells), rowsPerPage);
}

} // namespace nccu::dialog
