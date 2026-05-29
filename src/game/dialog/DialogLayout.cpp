#include "game/dialog/DialogLayout.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace nccu::dialog {

namespace {

// 以前導位元組 `b` 判斷該 UTF-8 序列的位元組長度。ASCII 為 1，本遊戲字串所在的
// CJK BMP 區為 3。與其他斷行器（MessageView 的 Utf8Len）採同一規則，使兩者逐位元組一致。
std::size_t Utf8Len(unsigned char b) noexcept {
    if (b < 0x80) return 1;
    if ((b >> 5) == 0x6) return 2;
    if ((b >> 4) == 0xE) return 3;
    if ((b >> 3) == 0x1E) return 4;
    return 1;  // 無效前導 → 視為一位元組，避免無窮迴圈
}

// 把自 &s[i] 起、長度 `n` 的 UTF-8 序列解碼為 codepoint。假定 i+n <= s.size()
// （由呼叫端保證）。
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

// 是否為組合附加符號（佔 0 視覺格）。格寬計算略過組合字元；本遊戲內容只用到
// 組合附加變音符號區，於此已足夠。
bool IsCombining(std::uint32_t cp) noexcept {
    return cp >= 0x0300u && cp <= 0x036Fu;
}

// East Asian Width「寬」：該 codepoint 佔 2 格。集合為 Unicode EAW 之 W／F／A 聯集
// （模糊類一律當作寬）。僅列舉本遊戲繁中內容 + UI 字面實際用到的區段；其餘 codepoint
// 一律視為 1 格（窄），對實用字元集而言與斷行驗證工具一致。已對 docs/content/*.md
// 全量核對過 EAW 分類。
bool IsWide(std::uint32_t cp) noexcept {
    struct Range { std::uint32_t lo, hi; };
    static constexpr std::array<Range, 17> kWide = {{
        {0x00A1u, 0x00A1u},  // ¡            （模糊）
        {0x00A4u, 0x00A4u},  // ¤            （模糊）
        {0x00A7u, 0x00A8u},  // §¨           （模糊）
        {0x00B0u, 0x00B1u},  // °±           （模糊）
        {0x00B7u, 0x00B7u},  // ·            （模糊）
        {0x00D7u, 0x00D7u},  // ×            （模糊）
        {0x00F7u, 0x00F7u},  // ÷            （模糊）
        {0x2010u, 0x2027u},  // ‐ … 一般標點 （模糊）
        {0x2030u, 0x205Fu},  // ‰ … 含 — “ ” ‘ ’ …（模糊）
        {0x2190u, 0x2199u},  // ← ↑ → ↓ 箭頭 （模糊）
        {0x2460u, 0x24FFu},  // 圈圍字母數字 （模糊）
        {0x25A0u, 0x26FFu},  // 幾何／雜項符號，▼ ▶ 等（A/W）
        {0x2E80u, 0x303Eu},  // CJK 部首 … CJK 符號與標點
        {0x3041u, 0x33FFu},  // 假名、CJK 相容（寬）
        {0x3400u, 0x4DBFu},  // CJK 擴充 A（寬）
        {0x4E00u, 0x9FFFu},  // CJK 統一表意文字（寬）
        {0xF900u, 0xFAFFu},  // CJK 相容表意文字（寬）
    }};
    static constexpr std::array<Range, 3> kWide2 = {{
        {0xFE30u, 0xFE4Fu},  // CJK 相容形式（寬）
        {0xFF00u, 0xFF60u},  // 全形形式 （）！？： 等（全形）
        {0xFFE0u, 0xFFE6u},  // 全形貨幣／符號（全形）
    }};
    for (const Range& r : kWide)
        if (cp >= r.lo && cp <= r.hi) return true;
    for (const Range& r : kWide2)
        if (cp >= r.lo && cp <= r.hi) return true;
    return false;
}

// 單一 codepoint 的格數：組合符號 0、寬 2、其餘 1。
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
    // 一旦軟斷行（寬度溢出）剛開新列即為 true——被推到列首的零星空白是 artifact，
    // 應丟棄。原行／硬斷 '\n' 之後作者刻意的前導縮排則保留（選項標記 "  "/"> "
    // 的對齊依賴此行為）。
    bool softWrapped = false;
    // 待處理單字緩衝：自上一個空白以來的 ASCII 連續串，使有空白時單字絕不從中切斷
    //（word wrap）。
    std::string word;
    int wordW = 0;

    auto flushRow = [&](bool soft) {
        // 軟斷邊界遺留的尾端空白是 artifact（其後的單字已移到下一列）——丟掉它，使
        // word-wrap 的列不留懸空空白。硬斷 '\n' 結尾的列則原樣保留內容。
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
    // 把待處理單字放進列；若單字自身就超過格寬上限則加以斷行（CJK／超長 token 路徑）。
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
        if (s[i] == '\n') {           // 硬斷（作者意圖）
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
            // 空白終結當前單字；先輸出單字，空白本身再參與列填充（在斷行處塌縮，
            // 使列不會以零星空白開頭）。
            placeWord();
            // 只在「軟斷列」列首丟掉空白（artifact）；否則保留作者刻意的前導縮排。
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
            // ASCII 單字的一部分：累積，除非單字自身超過格寬上限，否則不從中斷開。
            if (wordW + c > maxCells) placeWord();
            word.append(s, i, n);
            wordW += c;
            i += n;
            continue;
        }
        // CJK／標點：可在任意處斷開的單位。先輸出待處理 ASCII 單字，再放此字元（滿則斷行）。
        placeWord();
        if (rowW > 0 && rowW + c > maxCells) flushRow(true);
        row.append(s, i, n);
        rowW += c;
        i += n;
    }
    placeWord();
    rows.push_back(row);            // 收尾列（保證至少一列）
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
    if (pages.empty()) pages.emplace_back();   // 永遠至少一頁
    return pages;
}

std::vector<std::vector<std::string>>
LayoutPages(const std::string& s, int maxCells, int rowsPerPage) {
    return Paginate(WrapToCells(s, maxCells), rowsPerPage);
}

} // namespace nccu::dialog
