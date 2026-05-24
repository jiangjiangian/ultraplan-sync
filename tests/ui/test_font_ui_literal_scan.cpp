#include "doctest/doctest.h"
#include "gfx/Font.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// UI-B-1 — the PERMANENT no-`?` font gate. The historical bug was
// whack-a-mole: a CJK glyph used ONLY in a scattered string literal (a
// ShowMessage payload like the DLC teaser 「敬請期待」, a vendor greeting,
// an item-pickup line) tofu'd to `?` at runtime while every existing test
// stayed green, because the older glyph-scan tests only enumerated a
// hand-curated list of code-built UI surfaces (EndingCardStrings /
// QuestObjectiveStrings / CatalogStrings / … in test_font_ui_glyph_scan).
//
// THIS test removes the gap entirely by scanning the SOURCE itself:
//   1. Every "…" string literal in every .cpp/.h under src/ + include/.
//   2. Every codepoint in every docs/content/*.md.
// and asserting completeness against the font atlas the renderer builds.
//
// Two assertions make a silent `?` impossible to ship again:
//   (A) Every scanned CJK codepoint (source-literal OR content) is in the
//       EFFECTIVE atlas = ASCII 32..126 ∪ UiLiteralChars() ∪ docs/content.
//       So a NEW glyph anywhere fails the build unless it is reachable by
//       the atlas — the renderer can't draw a glyph the atlas lacks.
//   (B) Every SOURCE-LITERAL-ONLY codepoint (one that appears in NO .md,
//       so the runtime content-load path cannot cover it — and on a fresh
//       clone docs/content may be absent entirely) MUST be in
//       UiLiteralChars() specifically. This is the clause that catches
//       敬 (the DLC teaser): it is in no .md, so it has to be baked by hand
//       into the always-present literal set, or the gate is red.
//
// Headless-safe: pure std + the raylib codepoint decoder (no GL), and the
// source/content roots come from the build-system defines (CTest runs with
// cwd = build dir, exactly like the dialog content-dir tests).

#ifndef TEST_SOURCE_DIR
#error "TEST_SOURCE_DIR must be defined by the build system"
#endif
#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

namespace {

namespace fs = std::filesystem;

// ---- UTF-8 → codepoints (self-contained; mirrors DialogLayout.cpp) -----
std::size_t Utf8Len(unsigned char b) noexcept {
    if (b < 0x80) return 1;
    if ((b >> 5) == 0x6) return 2;
    if ((b >> 4) == 0xE) return 3;
    if ((b >> 3) == 0x1E) return 4;
    return 1;
}
void DecodeUtf8Into(const std::string& s, std::set<int>& out) {
    for (std::size_t i = 0; i < s.size();) {
        const auto b0 = static_cast<unsigned char>(s[i]);
        const std::size_t n = std::min(Utf8Len(b0), s.size() - i);
        std::uint32_t cp;
        if (n == 1) cp = b0;
        else {
            if (n == 2) cp = b0 & 0x1Fu;
            else if (n == 3) cp = b0 & 0x0Fu;
            else cp = b0 & 0x07u;
            bool ok = true;
            for (std::size_t k = 1; k < n; ++k) {
                const auto bk = static_cast<unsigned char>(s[i + k]);
                if ((bk & 0xC0u) != 0x80u) { ok = false; break; }
                cp = (cp << 6) | (bk & 0x3Fu);
            }
            if (!ok) cp = b0;            // malformed → treat lead byte as ASCII
        }
        if (cp > 0) out.insert(static_cast<int>(cp));
        i += n;
    }
}

// A codepoint we actually care about covering. ASCII (<0x80) is always in
// the atlas (CollectCodepoints adds 32..126); below the CJK/symbol range
// is noise. 0x2010 chosen so the CJK quotes / arrows / ▼ / ✓ / ─ / − the
// UI literals use are all included (same threshold the analysis used).
bool Interesting(int cp) noexcept { return cp > 0x2010; }

// ---- C++ source scrubber: drop comments, keep string-literal bytes -----
// A small state machine over one translation unit: skip // and /* */
// comments and char-literals, and for each "…" string literal append the
// DECODED bytes (resolving \xHH, \n, \t, \\, \" and \uXXXX) so the glyphs a
// literal would actually render are recovered. Concatenated adjacent
// literals ("a" "b") fall out naturally (each contributes its bytes). No
// raw-string ( R"(...)" ) handling — the repo uses none (asserted by a
// grep at authoring time; if one is ever added this scrubber treats the R
// as an identifier and the following "(...)" as an ordinary literal, which
// is conservative — it can only OVER-collect, never miss a glyph).
std::string ExtractLiteralBytes(const std::string& src) {
    std::string out;
    enum class St { Code, Slash, LineComment, BlockComment, BlockStar,
                    Str, StrEsc, Chr, ChrEsc };
    St st = St::Code;
    auto appendEscape = [&out](const std::string& s, std::size_t& i) {
        // i points at the char AFTER the backslash. Resolve the common
        // escapes the codebase uses; unknown escapes pass through literally.
        const char c = s[i];
        switch (c) {
            case 'n': out.push_back('\n'); ++i; break;
            case 't': out.push_back('\t'); ++i; break;
            case 'r': out.push_back('\r'); ++i; break;
            case '"': out.push_back('"');  ++i; break;
            case '\\': out.push_back('\\'); ++i; break;
            case '\'': out.push_back('\''); ++i; break;
            case '0': out.push_back('\0'); ++i; break;
            case 'x': {                       // \xHH (1-2 hex digits)
                ++i;
                int val = 0, digits = 0;
                while (i < s.size() && digits < 2 &&
                       std::isxdigit(static_cast<unsigned char>(s[i]))) {
                    const char d = s[i];
                    val = val * 16 + (d <= '9' ? d - '0'
                                    : (std::tolower(d) - 'a' + 10));
                    ++i; ++digits;
                }
                out.push_back(static_cast<char>(val));
                break;
            }
            case 'u': {                       // \uXXXX → UTF-8
                ++i;
                int val = 0, digits = 0;
                while (i < s.size() && digits < 4 &&
                       std::isxdigit(static_cast<unsigned char>(s[i]))) {
                    const char d = s[i];
                    val = val * 16 + (d <= '9' ? d - '0'
                                    : (std::tolower(d) - 'a' + 10));
                    ++i; ++digits;
                }
                // Encode the BMP codepoint as UTF-8.
                if (val < 0x80) out.push_back(static_cast<char>(val));
                else if (val < 0x800) {
                    out.push_back(static_cast<char>(0xC0 | (val >> 6)));
                    out.push_back(static_cast<char>(0x80 | (val & 0x3F)));
                } else {
                    out.push_back(static_cast<char>(0xE0 | (val >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((val >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (val & 0x3F)));
                }
                break;
            }
            default: out.push_back(c); ++i; break;
        }
    };
    for (std::size_t i = 0; i < src.size();) {
        const char c = src[i];
        switch (st) {
            case St::Code:
                if (c == '/') { st = St::Slash; ++i; }
                else if (c == '"') { st = St::Str; ++i; }
                else if (c == '\'') { st = St::Chr; ++i; }
                else ++i;
                break;
            case St::Slash:
                if (c == '/') { st = St::LineComment; ++i; }
                else if (c == '*') { st = St::BlockComment; ++i; }
                else st = St::Code;            // a stray '/', reprocess c
                break;
            case St::LineComment:
                if (c == '\n') st = St::Code;
                ++i;
                break;
            case St::BlockComment:
                if (c == '*') st = St::BlockStar;
                ++i;
                break;
            case St::BlockStar:
                if (c == '/') st = St::Code;
                else if (c != '*') st = St::BlockComment;
                ++i;
                break;
            case St::Str:
                if (c == '\\') { st = St::StrEsc; ++i; }
                else if (c == '"') { st = St::Code; ++i; }
                else { out.push_back(c); ++i; }
                break;
            case St::StrEsc:
                appendEscape(src, i);          // advances i past the escape
                st = St::Str;
                break;
            case St::Chr:
                if (c == '\\') { st = St::ChrEsc; ++i; }
                else if (c == '\'') { st = St::Code; ++i; }
                else ++i;                       // char-literal body ignored
                break;
            case St::ChrEsc:
                ++i;                            // skip the escaped char
                st = St::Chr;
                break;
        }
    }
    return out;
}

std::string ReadFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Codepoints in every "…" literal under src/ + include/. The map value is
// one example file per codepoint, for a legible failure message.
std::set<int> ScanSourceLiterals(std::string& sampleFor_out_diag) {
    (void)sampleFor_out_diag;
    std::set<int> cps;
    const fs::path roots[] = {fs::path(TEST_SOURCE_DIR) / "src",
                              fs::path(TEST_SOURCE_DIR) / "include"};
    for (const fs::path& root : roots) {
        if (!fs::exists(root)) continue;
        for (const auto& e : fs::recursive_directory_iterator(root)) {
            if (!e.is_regular_file()) continue;
            const std::string ext = e.path().extension().string();
            if (ext != ".cpp" && ext != ".h" && ext != ".hpp" &&
                ext != ".cc" && ext != ".cxx")
                continue;
            const std::string bytes = ExtractLiteralBytes(ReadFile(e.path()));
            DecodeUtf8Into(bytes, cps);
        }
    }
    return cps;
}

std::set<int> ScanContent() {
    std::set<int> cps;
    const fs::path dir(TEST_CONTENT_DIR);
    if (!fs::exists(dir)) return cps;
    for (const auto& e : fs::directory_iterator(dir)) {
        if (e.is_regular_file() && e.path().extension() == ".md")
            DecodeUtf8Into(ReadFile(e.path()), cps);
    }
    return cps;
}

// The deterministic, always-present literal set the runtime bakes
// regardless of whether docs/content can be read.
std::set<int> UiLiteralCodepoints() {
    std::set<int> cps;
    DecodeUtf8Into(nccu::gfx::detail::UiLiteralChars(), cps);
    return cps;
}

std::string Hex(int cp) {
    std::ostringstream ss;
    ss << "U+" << std::hex << std::uppercase << std::setw(4)
       << std::setfill('0') << cp;
    return ss.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// (A) Every scanned CJK codepoint is reachable by the effective atlas.
// ---------------------------------------------------------------------------
TEST_CASE("UI-B-1: every src/include literal + docs/content CJK glyph is in "
          "the font atlas") {
    const std::set<int> ascii = [] {
        std::set<int> s;
        for (int c = 32; c <= 126; ++c) s.insert(c);
        return s;
    }();
    const std::set<int> ui      = UiLiteralCodepoints();
    const std::set<int> content = ScanContent();

    // Sanity: the roots really resolved (else the whole gate is vacuous).
    REQUIRE(fs::exists(fs::path(TEST_SOURCE_DIR) / "src"));
    REQUIRE(fs::exists(fs::path(TEST_SOURCE_DIR) / "include"));
    REQUIRE(content.size() > 100);          // docs/content really enumerated

    // Effective atlas = what CollectCodepoints() yields when content IS
    // readable: ASCII ∪ UiLiteralChars ∪ docs/content (no broad fallback).
    std::set<int> atlas;
    atlas.insert(ascii.begin(), ascii.end());
    atlas.insert(ui.begin(), ui.end());
    atlas.insert(content.begin(), content.end());

    std::string diag;
    const std::set<int> srcLits = ScanSourceLiterals(diag);
    REQUIRE(srcLits.size() > 200);          // the scan really found literals

    // Assert every interesting source-literal glyph is in the atlas.
    int uncoveredSrc = 0;
    for (int cp : srcLits) {
        if (!Interesting(cp)) continue;
        if (atlas.find(cp) == atlas.end()) {
            ++uncoveredSrc;
            MESSAGE("UNCOVERED source-literal glyph " << Hex(cp));
        }
    }
    CHECK(uncoveredSrc == 0);

    // Assert every interesting content glyph is in the atlas (it always is
    // by construction, but this pins it so a later refactor of the atlas
    // build can't silently start dropping content glyphs).
    int uncoveredContent = 0;
    for (int cp : content) {
        if (!Interesting(cp)) continue;
        if (atlas.find(cp) == atlas.end()) {
            ++uncoveredContent;
            MESSAGE("UNCOVERED content glyph " << Hex(cp));
        }
    }
    CHECK(uncoveredContent == 0);
}

// ---------------------------------------------------------------------------
// (B) Every source-literal-only glyph (in NO .md) is baked into
// UiLiteralChars() — the clause that makes the fresh-clone fallback path
// (docs/content absent) just as tofu-proof, and the one that catches 敬.
// ---------------------------------------------------------------------------
TEST_CASE("UI-B-1: every source-literal-only glyph (absent from docs/content) "
          "is baked into UiLiteralChars()") {
    const std::set<int> ui      = UiLiteralCodepoints();
    const std::set<int> content = ScanContent();
    std::string diag;
    const std::set<int> srcLits = ScanSourceLiterals(diag);
    REQUIRE(srcLits.size() > 200);
    REQUIRE(content.size() > 100);

    std::vector<int> missing;
    for (int cp : srcLits) {
        if (!Interesting(cp)) continue;
        if (content.find(cp) != content.end()) continue;   // .md covers it
        if (ui.find(cp) == ui.end()) {                       // must be baked
            missing.push_back(cp);
            MESSAGE("source-literal-only glyph NOT in UiLiteralChars(): "
                    << Hex(cp));
        }
    }
    CHECK(missing.empty());
}

// ---------------------------------------------------------------------------
// Regression: the specific glyphs UI-B-1 surfaced as previously-missing
// (the owner's whack-a-mole set). Pinning them by name so a careless revert
// of the Font.h block is caught with a readable failure, independent of the
// broad scan above. 敬 is the DLC-teaser glyph.
// ---------------------------------------------------------------------------
TEST_CASE("UI-B-1: the previously-tofu source-literal glyphs are now baked") {
    const std::set<int> ui = UiLiteralCodepoints();
    // 敬 刺 君 含 扶 毫 央 櫃 牽 羊  + the two CJK curly quotes “ ”
    const std::string newlyBaked = "敬刺君含扶毫央櫃牽羊"
                                   "\xE2\x80\x9C\xE2\x80\x9D";
    std::set<int> need;
    DecodeUtf8Into(newlyBaked, need);
    for (int cp : need) {
        INFO("expected baked glyph " << Hex(cp));
        CHECK(ui.find(cp) != ui.end());
    }
}
