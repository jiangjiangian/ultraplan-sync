#include "doctest/doctest.h"
#include "engine/render/Font.h"

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

/**
 * @file test_font_ui_literal_scan.cpp
 * @brief 永久性的「無 ?」字型閘：直接掃描原始碼，找出 src/ 與 include/ 下每個
 *        字串字面、以及 docs/content/*.md 的每個碼位，並對照渲染器建構的字型圖集
 *        驗證完整性 —— 確保任何散落字串字面中的中文字都不會在執行時變成「?」豆腐。
 */
//
// 歷史上這類缺陷像打地鼠：某個只用在零散字串字面（如 ShowMessage 內容、商人問候、
// 撿道具台詞）的中文字會在執行時變成「?」，而既有測試卻全綠，因為先前的字形掃描
// 測試只列舉了一份人工整理的、由程式組出的 UI 介面清單。
//
// 本測試直接掃描原始碼以徹底消除這個破口：
//   1. src/ + include/ 下每個 .cpp/.h 的每個 "…" 字串字面。
//   2. 每個 docs/content/*.md 的每個碼位。
// 並對照渲染器建構的字型圖集驗證完整性。
//
// 兩項斷言使「?」不可能再被悄悄出貨：
//   (A) 掃描到的每個中文碼位（原始碼字面或內容）都在「有效圖集」中
//       ＝ ASCII 32..126 ∪ UiLiteralChars() ∪ docs/content。故任何地方的新字形若
//       無法被圖集涵蓋就會讓建置失敗 —— 渲染器無法畫出圖集所缺的字形。
//   (B) 每個「僅出現在原始碼字面」的碼位（不在任何 .md，故執行時的內容載入路徑
//       無法涵蓋它 —— 在全新 clone 中 docs/content 甚至可能完全不存在）都必須在
//       UiLiteralChars() 之中。這條正是抓出「敬」（預告字）的條款：它不在任何 .md，
//       故必須手動烘進永遠存在的字面集合，否則閘會變紅。
//
// 無頭安全：純標準函式庫加 raylib 碼位解碼器（無 GL），原始碼／內容根目錄來自
// 建置系統的定義（CTest 以建置目錄為工作目錄執行）。

#ifndef TEST_SOURCE_DIR
#error "TEST_SOURCE_DIR must be defined by the build system"
#endif
#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

namespace {

namespace fs = std::filesystem;

// ---- UTF-8 → 碼位（自足實作；與 DialogLayout.cpp 一致）-----------------
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
            if (!ok) cp = b0;            // 格式不正確 → 把前導位元組當作 ASCII
        }
        if (cp > 0) out.insert(static_cast<int>(cp));
        i += n;
    }
}

// 我們實際在意要涵蓋的碼位。ASCII（<0x80）一定在圖集中（CollectCodepoints 加入
// 32..126）；中文／符號範圍以下屬雜訊。門檻取 0x2010，使 UI 字面用到的中文引號／
// 箭頭／▼／✓／─／− 都被納入。
bool Interesting(int cp) noexcept { return cp > 0x2010; }

// ---- C++ 原始碼清洗器：去掉註解，保留字串字面的位元組 -------------------
// 對單一編譯單元的小型狀態機：跳過 // 與 /* */ 註解及字元字面，並對每個 "…" 字串
// 字面附加「解碼後」的位元組（解析 \xHH、\n、\t、\\、\" 與 \uXXXX），以還原字面
// 實際會渲染的字形。相鄰串接的字面（"a" "b"）自然會被各自納入。不處理原始字串
// （R"(...)"）—— 本專案未使用（撰寫時以 grep 確認；若日後加入，此清洗器會把 R
// 當識別字、後續 "(...)" 當一般字面，屬保守作法 —— 只會多收、絕不漏字）。
std::string ExtractLiteralBytes(const std::string& src) {
    std::string out;
    enum class St { Code, Slash, LineComment, BlockComment, BlockStar,
                    Str, StrEsc, Chr, ChrEsc };
    St st = St::Code;
    auto appendEscape = [&out](const std::string& s, std::size_t& i) {
        // i 指向反斜線「之後」的字元。解析本程式碼庫常用的跳脫；未知跳脫原樣通過。
        const char c = s[i];
        switch (c) {
            case 'n': out.push_back('\n'); ++i; break;
            case 't': out.push_back('\t'); ++i; break;
            case 'r': out.push_back('\r'); ++i; break;
            case '"': out.push_back('"');  ++i; break;
            case '\\': out.push_back('\\'); ++i; break;
            case '\'': out.push_back('\''); ++i; break;
            case '0': out.push_back('\0'); ++i; break;
            case 'x': {                       // \xHH（1-2 個十六進位數字）
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
                int val = 0, digits = 0;       // 解析 4 個十六進位數字
                while (i < s.size() && digits < 4 &&
                       std::isxdigit(static_cast<unsigned char>(s[i]))) {
                    const char d = s[i];
                    val = val * 16 + (d <= '9' ? d - '0'
                                    : (std::tolower(d) - 'a' + 10));
                    ++i; ++digits;
                }
                // 把 BMP 碼位編成 UTF-8。
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
            default: out.push_back(c); ++i; break;   // 未知跳脫：原樣保留該字元
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
                else st = St::Code;            // 落單的 '/'，重新處理 c
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
                appendEscape(src, i);          // 會把 i 推進到跳脫之後
                st = St::Str;
                break;
            case St::Chr:
                if (c == '\\') { st = St::ChrEsc; ++i; }
                else if (c == '\'') { st = St::Code; ++i; }
                else ++i;                       // 忽略字元字面的內容
                break;
            case St::ChrEsc:
                ++i;                            // 跳過被跳脫的字元
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

// src/ + include/ 下每個 "…" 字面中的碼位。
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

// 不論能否讀到 docs/content，執行時都一定會烘入的確定性字面集合。
std::set<int> UiLiteralCodepoints() {
    std::set<int> cps;
    DecodeUtf8Into(nccu::engine::render::detail::UiLiteralChars(), cps);
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
// (A) 掃描到的每個中文碼位都能被有效圖集涵蓋。
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

    // 確認根目錄確實解析到（否則整個閘形同虛設）。
    REQUIRE(fs::exists(fs::path(TEST_SOURCE_DIR) / "src"));
    REQUIRE(fs::exists(fs::path(TEST_SOURCE_DIR) / "include"));
    REQUIRE(content.size() > 100);          // docs/content 確實有列舉

    // 有效圖集＝內容可讀時 CollectCodepoints() 所產生者：
    // ASCII ∪ UiLiteralChars ∪ docs/content（無廣泛的後備）。
    std::set<int> atlas;
    atlas.insert(ascii.begin(), ascii.end());
    atlas.insert(ui.begin(), ui.end());
    atlas.insert(content.begin(), content.end());

    std::string diag;
    const std::set<int> srcLits = ScanSourceLiterals(diag);
    REQUIRE(srcLits.size() > 200);          // 掃描確實找到字面

    // 斷言每個值得注意的原始碼字面字形都在圖集中。
    int uncoveredSrc = 0;
    for (int cp : srcLits) {
        if (!Interesting(cp)) continue;
        if (atlas.find(cp) == atlas.end()) {
            ++uncoveredSrc;
            MESSAGE("UNCOVERED source-literal glyph " << Hex(cp));
        }
    }
    CHECK(uncoveredSrc == 0);

    // 斷言每個值得注意的內容字形都在圖集中（依建構方式本就如此，但此處固定它，
    // 使日後重構圖集建構流程時不致悄悄開始漏掉內容字形）。
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
// (B) 每個僅出現在原始碼字面（不在任何 .md）的字形都已烘進 UiLiteralChars() ——
// 這條使全新 clone 的後備路徑（docs/content 不存在）同樣防豆腐，也是抓出「敬」的
// 那條。
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
        if (content.find(cp) != content.end()) continue;   // .md 已涵蓋
        if (ui.find(cp) == ui.end()) {                       // 必須被烘入
            missing.push_back(cp);
            MESSAGE("source-literal-only glyph NOT in UiLiteralChars(): "
                    << Hex(cp));
        }
    }
    CHECK(missing.empty());
}

// ---------------------------------------------------------------------------
// 把先前缺漏的特定字形以具名方式固定，使粗心地退回 Font.h 區段時能以可讀的失敗
// 被抓到，獨立於上面的廣泛掃描。「敬」是預告字。
// ---------------------------------------------------------------------------
TEST_CASE("UI-B-1: the previously-tofu source-literal glyphs are now baked") {
    const std::set<int> ui = UiLiteralCodepoints();
    // 敬 刺 君 含 扶 毫 央 櫃 牽 羊 加上兩個中文彎引號 “ ”
    const std::string newlyBaked = "敬刺君含扶毫央櫃牽羊"
                                   "\xE2\x80\x9C\xE2\x80\x9D";
    std::set<int> need;
    DecodeUtf8Into(newlyBaked, need);
    for (int cp : need) {
        INFO("expected baked glyph " << Hex(cp));
        CHECK(ui.find(cp) != ui.end());
    }
}
