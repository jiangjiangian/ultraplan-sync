#include "DialogLoader.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>

namespace nccu::dialog {

namespace {

// UTF-8 byte sequences we deal with explicitly.
constexpr const char* kFullWidthColon = "\xEF\xBC\x9A";   // U+FF1A "："
constexpr const char* kLeftCjkQuote   = "\xE2\x80\x9C";   // U+201C "“"
constexpr const char* kRightCjkQuote  = "\xE2\x80\x9D";   // U+201D "”"
constexpr const char* kLeftCornerQuote  = "\xE3\x80\x8C"; // U+300C "「"
constexpr const char* kRightCornerQuote = "\xE3\x80\x8D"; // U+300D "」"
constexpr const char* kFullWidthParenL  = "\xEF\xBC\x88"; // U+FF08 "（"
constexpr const char* kFullWidthParenR  = "\xEF\xBC\x89"; // U+FF09 "）"

bool StartsWith(const std::string& s, const char* prefix) {
    const size_t n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

// Strip a trailing carriage return (CRLF line endings on Windows).
void RStripCr(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

// Trim leading and trailing ASCII whitespace.
std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) --e;
    return s.substr(b, e - b);
}

// "## NPC：<name>" -> <name> (trimmed). Returns empty string if the line is
// not an NPC heading. Uses the full-width colon as the separator.
std::string ParseNpcName(const std::string& line) {
    static const std::string kPrefix = "## NPC";
    if (line.size() < kPrefix.size() ||
        line.compare(0, kPrefix.size(), kPrefix) != 0) {
        return {};
    }
    const std::string rest = line.substr(kPrefix.size());
    const size_t colon = rest.find(kFullWidthColon);
    if (colon == std::string::npos) return {};
    std::string name = Trim(rest.substr(colon + 3 /* bytes in U+FF1A */));

    // Strip a trailing full-width （…） annotation so the section key is
    // the bare name — e.g. chapter2.md「## NPC：圖書館管理員（新角色）」
    // keys as "圖書館管理員". This mirrors CleanLabel's anchored-at-end
    // （…）strip for choice headings; verified that no NPC name across
    // chapter1-4 legitimately ends in （…）, so this only removes author
    // scaffolding and Ch1 (bare headings) is a no-op.
    const size_t pr = name.rfind(kFullWidthParenR);
    if (pr != std::string::npos && pr + 3 == name.size()) {
        const size_t pl = name.find(kFullWidthParenL);
        if (pl != std::string::npos && pl < pr)
            name = Trim(name.substr(0, pl));
    }
    return name;
}

// gen_dialog.py SUB = {"a":0,"b":1,"c":2,"d":3}.
int SubStateValue(char c) { return c - 'a'; }

// Parses a "### (x) <heading>" substate header. On success returns true,
// sets `letter` to the substate letter ('a'..'d') and `heading` to the raw
// heading text after the "(x)" marker (caller passes it to CleanLabel).
// Strict, mirroring gen_dialog SUBSEC_RE = ^###\s*\(([a-d])\)\s*(.*)$ as
// used by LoadChapter's prior strict "### (" parser.
bool ParseSubStateHeader(const std::string& line, char& letter,
                         std::string& heading) {
    static const std::string kPrefix = "### (";
    if (line.size() < kPrefix.size() + 2 ||
        line.compare(0, kPrefix.size(), kPrefix) != 0) {
        return false;
    }
    const char c = line[kPrefix.size()];
    if (c < 'a' || c > 'd') return false;
    if (line[kPrefix.size() + 1] != ')') return false;
    letter = c;
    // Heading text = everything after the ")" marker, leading ASCII
    // whitespace stripped (SUBSEC_RE's \s* after the group).
    heading = line.substr(kPrefix.size() + 2);
    size_t b = 0;
    while (b < heading.size() &&
           (heading[b] == ' ' || heading[b] == '\t')) {
        ++b;
    }
    heading = heading.substr(b);
    return true;
}

// Mirrors gen_dialog.clean_label():
//   - a 「…」 quoted span is the author's explicit label override and wins;
//   - otherwise the heading minus any trailing （…） full-width
//     parenthetical, trimmed.
std::string CleanLabel(const std::string& raw) {
    const std::string h = Trim(raw);

    // 「…」 span wins. Find the first U+300C, then the next U+300D after it;
    // the inner text is the label (re.search with a non-greedy .+?).
    const size_t lq = h.find(kLeftCornerQuote);
    if (lq != std::string::npos) {
        const size_t inner = lq + 3;  // bytes in U+300C
        const size_t rq = h.find(kRightCornerQuote, inner);
        if (rq != std::string::npos && rq > inner) {
            return h.substr(inner, rq - inner);
        }
    }

    // else drop a trailing （…）\s*$ full-width parenthetical. Scan for the
    // last U+FF08 whose matching U+FF09 ends the string (ignoring trailing
    // ASCII whitespace) — re.sub(r"（.*?）\s*$", "", h) anchors at end.
    size_t end = h.size();
    while (end > 0 && (h[end - 1] == ' ' || h[end - 1] == '\t')) --end;
    if (end >= 3 && h.compare(end - 3, 3, kFullWidthParenR) == 0) {
        // Greedy-from-left search for the FIRST （ that still closes at this
        // final ）: re's （.*?）\s*$ is non-greedy but anchored at end, so
        // the earliest （ before this final ） wins.
        const size_t open = h.find(kFullWidthParenL);
        if (open != std::string::npos && open + 3 <= end - 3) {
            return Trim(h.substr(0, open));
        }
    }
    return h;
}

// Does s have the given suffix starting at byte index `start_of_suffix`?
bool HasSuffixAt(const std::string& s, size_t start_of_suffix,
                 const char* suffix) {
    const size_t n = std::char_traits<char>::length(suffix);
    if (start_of_suffix + n != s.size()) return false;
    return s.compare(start_of_suffix, n, suffix) == 0;
}

// Parses a bullet dialog line of the form
//   - "<text>"
// where the quotes can be ASCII (0x22) or CJK (U+201C / U+201D). Returns
// the inner text. On any mismatch (no leading dash, missing/mismatched
// quotes, empty body, etc.) returns false; caller checks the returned bool.
bool ParseDialogLine(const std::string& line, std::string& out) {
    // Must start with "- " (two bytes, ASCII).
    if (line.size() < 4) return false;
    if (line[0] != '-' || line[1] != ' ') return false;

    // The opening quote begins immediately after "- ".
    const size_t open_at = 2;

    // Case A: ASCII double quote ".
    if (line[open_at] == '"') {
        if (line.back() != '"') return false;
        if (line.size() < open_at + 2 + 1) return false; // need closing "
        const size_t inner_begin = open_at + 1;
        const size_t inner_end   = line.size() - 1; // exclusive
        if (inner_end <= inner_begin) return false;  // empty body
        out.assign(line, inner_begin, inner_end - inner_begin);
        return true;
    }

    // Case B: CJK left quote U+201C (e2 80 9c).
    if (line.size() >= open_at + 3 &&
        line.compare(open_at, 3, kLeftCjkQuote) == 0) {
        const size_t inner_begin = open_at + 3;
        // Must end with U+201D.
        if (line.size() < inner_begin + 3) return false;
        const size_t inner_end = line.size() - 3;
        if (!HasSuffixAt(line, inner_end, kRightCjkQuote)) return false;
        if (inner_end <= inner_begin) return false;  // empty body
        out.assign(line, inner_begin, inner_end - inner_begin);
        return true;
    }

    return false;
}

// Scans one ">" blockquote line for a `// karma ±N` note. Mirrors
// gen_dialog KARMA_RE = //\s*karma\s*([+-]\d+). Returns true and sets
// `value` on a match.
bool ScanKarma(const std::string& line, int& value) {
    const std::string kMark = "//";
    size_t p = line.find(kMark);
    while (p != std::string::npos) {
        size_t i = p + kMark.size();
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (line.compare(i, 5, "karma") == 0) {
            i += 5;
            while (i < line.size() &&
                   (line[i] == ' ' || line[i] == '\t')) {
                ++i;
            }
            if (i < line.size() && (line[i] == '+' || line[i] == '-')) {
                size_t d = i + 1;
                while (d < line.size() && line[d] >= '0' &&
                       line[d] <= '9') {
                    ++d;
                }
                if (d > i + 1) {  // at least one digit
                    value = std::atoi(line.substr(i, d - i).c_str());
                    return true;
                }
            }
        }
        p = line.find(kMark, p + 1);
    }
    return false;
}

bool IsFlagWordByte(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_';
}

// Scans one ">" blockquote line for a `Flag_X = true|false` note. Mirrors
// gen_dialog FLAG_RE = \b(Flag_[A-Za-z0-9_]+)\s*=\s*(true|false)\b.
bool ScanFlag(const std::string& line, std::string& name, bool& val) {
    const std::string kPfx = "Flag_";
    size_t p = line.find(kPfx);
    while (p != std::string::npos) {
        // \b before "Flag_": preceding byte must not be a word byte.
        if (p == 0 || !IsFlagWordByte(line[p - 1])) {
            size_t e = p + kPfx.size();
            while (e < line.size() && IsFlagWordByte(line[e])) ++e;
            if (e > p + kPfx.size()) {  // at least one trailing word char
                size_t i = e;
                while (i < line.size() &&
                       (line[i] == ' ' || line[i] == '\t')) {
                    ++i;
                }
                if (i < line.size() && line[i] == '=') {
                    ++i;
                    while (i < line.size() &&
                           (line[i] == ' ' || line[i] == '\t')) {
                        ++i;
                    }
                    bool ok = false;
                    bool v = false;
                    size_t after = i;
                    if (line.compare(i, 4, "true") == 0) {
                        ok = true; v = true; after = i + 4;
                    } else if (line.compare(i, 5, "false") == 0) {
                        ok = true; v = false; after = i + 5;
                    }
                    // \b after the literal: next byte not a word byte.
                    if (ok && (after >= line.size() ||
                               !IsFlagWordByte(line[after]))) {
                        name = line.substr(p, e - p);
                        val = v;
                        return true;
                    }
                }
            }
        }
        p = line.find(kPfx, p + 1);
    }
    return false;
}

}  // namespace

LoadedChapter LoadChapter(const std::string& path) {
    LoadedChapter chapter;
    std::ifstream in(path);
    if (!in.is_open()) return chapter;

    std::string currentNpc;
    SubEntry*   cur = nullptr;  // active substate entry, or null

    std::string line;
    while (std::getline(in, line)) {
        RStripCr(line);

        // Any "## " heading terminates the previous NPC section. (gen_dialog
        // also treats a leading "# " the same way; LoadChapter's content
        // files always use "## " for NPC headings, so "## " suffices and
        // matches the prior behaviour exactly.)
        if (StartsWith(line, "## ")) {
            cur = nullptr;
            const std::string name = ParseNpcName(line);
            if (!name.empty()) {
                currentNpc = name;
                // Ensure the NPC entry exists even if it has no substates.
                chapter.npcs[currentNpc];
            } else {
                currentNpc.clear();
            }
            continue;
        }

        // Only meaningful while we are inside an NPC section.
        if (currentNpc.empty()) continue;

        if (StartsWith(line, "### ")) {
            char letter = 0;
            std::string heading;
            if (ParseSubStateHeader(line, letter, heading)) {
                auto& entries = chapter.npcs[currentNpc];
                entries.push_back(SubEntry{
                    SubStateValue(letter),
                    {},
                    CleanLabel(heading),
                    0,
                    {},
                    false,
                });
                cur = &entries.back();
            } else {
                cur = nullptr;
            }
            continue;
        }

        if (cur == nullptr) continue;

        // Blockquote note lines: scanned for karma / flag metadata while a
        // substate is active. "first non-zero" / "first non-empty" guards
        // mirror gen_dialog (`if cur.karma == 0` / `if not cur.flag`).
        if (!line.empty() && line[0] == '>') {
            if (cur->karmaDelta == 0) {
                int k = 0;
                if (ScanKarma(line, k)) cur->karmaDelta = k;
            }
            if (cur->setsFlag.empty()) {
                std::string fn;
                bool fv = false;
                if (ScanFlag(line, fn, fv)) {
                    cur->setsFlag = fn;
                    cur->flagValue = fv;
                }
            }
            continue;
        }

        if (line.empty()) continue;

        std::string text;
        if (ParseDialogLine(line, text)) {
            cur->lines.push_back(std::move(text));
        }
    }

    // Sub-blocks ordered by subState ascending (a<b<c<d). Content files
    // already author them in order, but enforce the model invariant. Stable
    // so duplicate-letter authoring keeps document order within a letter.
    for (auto& kv : chapter.npcs) {
        std::stable_sort(kv.second.begin(), kv.second.end(),
                         [](const SubEntry& l, const SubEntry& r) {
                             return l.subState < r.subState;
                         });
    }
    return chapter;
}

}  // namespace nccu::dialog
