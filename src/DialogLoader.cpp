#include "DialogLoader.h"

#include <fstream>
#include <string>

namespace nccu::dialog {

namespace {

// UTF-8 byte sequences we deal with explicitly.
constexpr const char* kFullWidthColon = "\xEF\xBC\x9A";   // U+FF1A "："
constexpr const char* kLeftCjkQuote   = "\xE2\x80\x9C";   // U+201C "“"
constexpr const char* kRightCjkQuote  = "\xE2\x80\x9D";   // U+201D "”"

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
    return Trim(rest.substr(colon + 3 /* bytes in U+FF1A */));
}

// "### (a) something" -> 'a'. Returns 0 if the line is not a substate header.
// Strict: requires "### (" then a single lowercase ASCII letter then ")".
char ParseSubState(const std::string& line) {
    static const std::string kPrefix = "### (";
    if (line.size() < kPrefix.size() + 2 ||
        line.compare(0, kPrefix.size(), kPrefix) != 0) {
        return 0;
    }
    const char c = line[kPrefix.size()];
    if (c < 'a' || c > 'd') return 0;
    if (line[kPrefix.size() + 1] != ')') return 0;
    return c;
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
// quotes, empty body, etc.) returns std::nullopt-via-empty: caller checks
// the returned bool.
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

}  // namespace

LoadedChapter LoadChapter(const std::string& path) {
    LoadedChapter chapter;
    std::ifstream in(path);
    if (!in.is_open()) return chapter;

    std::string currentNpc;
    char currentSubState = 0;

    std::string line;
    while (std::getline(in, line)) {
        RStripCr(line);

        // Any "## " heading terminates the previous NPC section.
        if (StartsWith(line, "## ")) {
            const std::string name = ParseNpcName(line);
            if (!name.empty()) {
                currentNpc = name;
                currentSubState = 0;
                // Ensure the NPC entry exists even if it has no substates yet.
                chapter.npcs[currentNpc];
            } else {
                currentNpc.clear();
                currentSubState = 0;
            }
            continue;
        }

        // Only meaningful while we are inside an NPC section.
        if (currentNpc.empty()) continue;

        if (StartsWith(line, "### ")) {
            const char ss = ParseSubState(line);
            if (ss != 0) {
                currentSubState = ss;
                // Ensure the substate entry exists even if it has no lines.
                chapter.npcs[currentNpc][currentSubState];
            } else {
                currentSubState = 0;
            }
            continue;
        }

        // Skip blockquotes and empty lines outright.
        if (line.empty()) continue;
        if (line[0] == '>') continue;

        if (currentSubState == 0) continue;

        std::string text;
        if (ParseDialogLine(line, text)) {
            chapter.npcs[currentNpc][currentSubState].push_back(std::move(text));
        }
    }
    return chapter;
}

}  // namespace nccu::dialog
