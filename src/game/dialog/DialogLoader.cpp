#include "game/dialog/DialogLoader.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>

/**
 * @file DialogLoader.cpp
 * @brief 章節對白 Markdown 的解析器：把 NPC 標題、(x) 子段標題、選項標籤與
 *        台詞行，連同 karma／旗標註記，解析成 LoadedChapter 模型。
 *
 * 全程逐位元組手寫掃描，刻意不引入正規表示式，以便精準處理全形標點與 CJK 引號
 *（「」“” 與全形冒號、全形括號）等多位元組序列；其匹配規則須與內容產生工具保持一致。
 */

namespace nccu::dialog {

namespace {

// 需顯式處理的 UTF-8 位元組序列（全形標點與 CJK 引號）。
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

// 去除尾端的歸位字元（Windows 的 CRLF 行尾）。
void RStripCr(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

// 修剪前後的 ASCII 空白。
std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) --e;
    return s.substr(b, e - b);
}

// 「## NPC：<name>」→ <name>（已修剪）。非 NPC 標題的行回傳空字串。以全形冒號為分隔。
std::string ParseNpcName(const std::string& line) {
    static const std::string kPrefix = "## NPC";
    if (line.size() < kPrefix.size() ||
        line.compare(0, kPrefix.size(), kPrefix) != 0) {
        return {};
    }
    const std::string rest = line.substr(kPrefix.size());
    const size_t colon = rest.find(kFullWidthColon);
    if (colon == std::string::npos) return {};
    std::string name = Trim(rest.substr(colon + 3 /* U+FF1A 的位元組數 */));

    // 剝除尾端的全形 （…） 註解，使段落鍵為純角色名——例如標題「## NPC：圖書館管理員
    //（新角色）」對應的鍵為「圖書館管理員」。與 CleanLabel 對選項標題「錨定於結尾的
    // （…）」剝除規則一致；各章內容中並無任何 NPC 名合法地以 （…） 結尾，故此處只移除
    // 作者撰寫時的鷹架標註，對純標題（無括註）則不作任何更動。
    const size_t pr = name.rfind(kFullWidthParenR);
    if (pr != std::string::npos && pr + 3 == name.size()) {
        const size_t pl = name.find(kFullWidthParenL);
        if (pl != std::string::npos && pl < pr)
            name = Trim(name.substr(0, pl));
    }
    return name;
}

// 子段字母對映：a→0, b→1, c→2, d→3。
int SubStateValue(char c) { return c - 'a'; }

// 解析「### (x) <heading>」子段標題。成功時回傳 true，並將 `letter` 設為子段字母
//（'a'..'d'）、`heading` 設為 "(x)" 標記之後的原始標題文字（呼叫端再交給 CleanLabel）。
// 嚴格對應 .md 文法 ^###\s*\(([a-d])\)\s*(.*)$。
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
    // 標題文字 = ")" 標記之後的全部內容，剝除前導 ASCII 空白（對應文法中分組後的 \s*）
    heading = line.substr(kPrefix.size() + 2);
    size_t b = 0;
    while (b < heading.size() &&
           (heading[b] == ' ' || heading[b] == '\t')) {
        ++b;
    }
    heading = heading.substr(b);
    return true;
}

// 由子段標題導出選單標籤：
//   - 「…」 引用段為作者明確指定的標籤覆寫，優先採用；
//   - 否則取標題剝除尾端 （…） 全形註解後、修剪過的文字。
std::string CleanLabel(const std::string& raw) {
    const std::string h = Trim(raw);

    // 「…」 段優先。找第一個 U+300C，再找其後第一個 U+300D；內側文字即標籤
    //（非貪婪比對）。
    const size_t lq = h.find(kLeftCornerQuote);
    if (lq != std::string::npos) {
        const size_t inner = lq + 3;  // U+300C 的位元組數
        const size_t rq = h.find(kRightCornerQuote, inner);
        if (rq != std::string::npos && rq > inner) {
            return h.substr(inner, rq - inner);
        }
    }

    // 否則剝除尾端 （…）\s*$ 全形括註。找其 U+FF09 恰好結束字串（忽略尾端 ASCII
    // 空白）的最後一段——對應錨定於結尾的 （.*?）\s*$。
    size_t end = h.size();
    while (end > 0 && (h[end - 1] == ' ' || h[end - 1] == '\t')) --end;
    if (end >= 3 && h.compare(end - 3, 3, kFullWidthParenR) == 0) {
        // 由左往右找仍能在這個結尾 ） 收尾的第一個 （：文法雖非貪婪但錨定於結尾，
        // 故這個結尾 ） 之前最早的 （ 勝出。
        const size_t open = h.find(kFullWidthParenL);
        if (open != std::string::npos && open + 3 <= end - 3) {
            return Trim(h.substr(0, open));
        }
    }
    return h;
}

// s 是否在位元組索引 `start_of_suffix` 起恰好以 suffix 結尾？
bool HasSuffixAt(const std::string& s, size_t start_of_suffix,
                 const char* suffix) {
    const size_t n = std::char_traits<char>::length(suffix);
    if (start_of_suffix + n != s.size()) return false;
    return s.compare(start_of_suffix, n, suffix) == 0;
}

// 解析形如「- "<text>"」的對白項，引號可為 ASCII（0x22）或 CJK（U+201C / U+201D），
// 回傳內側文字。任何不符（無前導破折號、引號缺失／不匹配、內文為空等）回傳 false，
// 由呼叫端檢查回傳的 bool。
bool ParseDialogLine(const std::string& line, std::string& out) {
    // 必須以「- 」開頭（兩個 ASCII 位元組）
    if (line.size() < 4) return false;
    if (line[0] != '-' || line[1] != ' ') return false;

    // 開引號緊接在「- 」之後
    const size_t open_at = 2;

    // 情況 A：ASCII 雙引號 "
    if (line[open_at] == '"') {
        if (line.back() != '"') return false;
        if (line.size() < open_at + 2 + 1) return false; // 需要收尾的 "
        const size_t inner_begin = open_at + 1;
        const size_t inner_end   = line.size() - 1; // 不含
        if (inner_end <= inner_begin) return false;  // 內文為空
        out.assign(line, inner_begin, inner_end - inner_begin);
        return true;
    }

    // 情況 B：CJK 左引號 U+201C（e2 80 9c）
    if (line.size() >= open_at + 3 &&
        line.compare(open_at, 3, kLeftCjkQuote) == 0) {
        const size_t inner_begin = open_at + 3;
        // 必須以 U+201D 結尾
        if (line.size() < inner_begin + 3) return false;
        const size_t inner_end = line.size() - 3;
        if (!HasSuffixAt(line, inner_end, kRightCjkQuote)) return false;
        if (inner_end <= inner_begin) return false;  // 內文為空
        out.assign(line, inner_begin, inner_end - inner_begin);
        return true;
    }

    return false;
}

// 掃描一行 ">" 引用區塊，尋找 `// karma ±N` 註記。比對規則須與內容產生器端的
// karma 樣式一致（即 //\s*karma\s*([+-]\d+) 所表達者）。命中時回傳 true 並寫入
// `value`。
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
                if (d > i + 1) {  // 至少要有一位數字
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

// 掃描一行 ">" 引用區塊，尋找 `Flag_X = true|false` 註記。比對規則須與內容產生器端
// 的旗標樣式一致（即 \b(Flag_[A-Za-z0-9_]+)\s*=\s*(true|false)\b 所表達者）。
bool ScanFlag(const std::string& line, std::string& name, bool& val) {
    const std::string kPfx = "Flag_";
    size_t p = line.find(kPfx);
    while (p != std::string::npos) {
        // "Flag_" 之前的單字邊界 \b：前一個位元組不可為單字字元。
        if (p == 0 || !IsFlagWordByte(line[p - 1])) {
            size_t e = p + kPfx.size();
            while (e < line.size() && IsFlagWordByte(line[e])) ++e;
            if (e > p + kPfx.size()) {  // 前綴後至少要有一個單字字元
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
                    // 字面值之後的單字邊界 \b：下一個位元組不可為單字字元。
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
    SubEntry*   cur = nullptr;  // 目前作用中的子段條目，無則為 null

    std::string line;
    while (std::getline(in, line)) {
        RStripCr(line);

        // 任何 "## " 標題都會結束前一個 NPC 段落。內容檔的 NPC 標題一律使用
        // "## "，故只認 "## " 即可，與既有行為完全一致。
        if (StartsWith(line, "## ")) {
            cur = nullptr;
            const std::string name = ParseNpcName(line);
            if (!name.empty()) {
                currentNpc = name;
                // 即使該 NPC 沒有任何子段，也要先確保其條目存在。
                chapter.npcs[currentNpc];
            } else {
                currentNpc.clear();
            }
            continue;
        }

        // 只有在 NPC 段落內部才有意義。
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

        // 引用區塊註記行：當有子段作用中時，從中掃描 karma／旗標的中介資料。
        // 「取第一個非零 karma」「取第一個非空旗標」這兩道防護與內容產生器端一致
        // ——已寫入過的值不再被後續同類註記覆寫。
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

    // 子段依 subState 遞增排序（a<b<c<d）。內容檔通常已照順序撰寫，但仍主動施行此
    // 模型不變式。採穩定排序，使同一字母重複出現時，仍保留其在文件中的原始先後。
    for (auto& kv : chapter.npcs) {
        std::stable_sort(kv.second.begin(), kv.second.end(),
                         [](const SubEntry& l, const SubEntry& r) {
                             return l.subState < r.subState;
                         });
    }
    return chapter;
}

}  // namespace nccu::dialog
