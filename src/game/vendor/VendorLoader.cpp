#include "game/vendor/VendorLoader.h"

#include <cstdlib>
#include <fstream>
#include <string>

/**
 * @file VendorLoader.cpp
 * @brief 市集內容檔的逐行解析實作：把 markdown 區段轉成 VendorConfig 清單。
 */

namespace nccu::vendor {

namespace {

// 原始碼檔為 UTF-8，從磁碟讀進的 markdown 也是 UTF-8 位元組，因此對 UTF-8 字串字面
// 做 std::string::find／compare 即為位元組比對——正是所需。（DialogLoader 對少數需要
// 的碼位用 \x 轉義；此處為求清晰直接用字面常數，因為欄位標籤都是完整詞，手算其位元組
// 是值得從設計上避開的出錯點。）
constexpr const char* kFullWidthColon  = "：";   // U+FF1A
constexpr const char* kLeftCjkQuote    = "“";   // U+201C
constexpr const char* kRightCjkQuote   = "”";   // U+201D
constexpr const char* kFullWidthParenL = "（";   // U+FF08

const std::string kStallPrefix = "## 攤位";
const std::string kTagKeeper   = "攤主";
const std::string kTagItem     = "商品";
const std::string kTagMechanic = "機制";

void RStripCr(std::string& s) {
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) --e;
    return s.substr(b, e - b);
}

bool StartsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool HasSuffix(const std::string& s, size_t at, const char* suf) {
    const size_t n = std::char_traits<char>::length(suf);
    if (at + n != s.size()) return false;
    return s.compare(at, n, suf) == 0;
}

// 由 "## 攤位：<name>" 取出 <name>；非攤位標題則回傳空字串。"## 10 攤位 lineup"
// 之類會被安全排除（並非以 "## 攤位" 開頭）。
std::string ParseStallName(const std::string& line) {
    if (!StartsWith(line, kStallPrefix)) return {};
    const std::string rest = line.substr(kStallPrefix.size());
    const size_t colon = rest.find(kFullWidthColon);
    if (colon == std::string::npos) return {};
    return Trim(rest.substr(colon + std::char_traits<char>::length(kFullWidthColon)));
}

// 由 "### <key>…" 取出基礎 key（空白或全形括號（後綴之前的 token）。
// 例如 "### onPurchase（陷阱傘殘骸）" 取出 "onPurchase"。
std::string ParseSubsectionKey(const std::string& line) {
    static const std::string kPrefix = "### ";
    if (!StartsWith(line, kPrefix)) return {};
    const std::string rest = Trim(line.substr(kPrefix.size()));
    const size_t parenLen = std::char_traits<char>::length(kFullWidthParenL);
    size_t end = rest.size();
    for (size_t i = 0; i < rest.size(); ++i) {
        if (rest[i] == ' ' || rest[i] == '\t') { end = i; break; }
        if (i + parenLen <= rest.size() &&
            rest.compare(i, parenLen, kFullWidthParenL) == 0) {
            end = i;
            break;
        }
    }
    return rest.substr(0, end);
}

// 取出 `- "…"` 條列的內層文字（ASCII " 或全形 U+201C/U+201D 引號）。任一處不符即回傳
// false——契約與 DialogLoader::ParseDialogLine 完全相同。
bool ParseBulletLine(const std::string& line, std::string& out) {
    if (line.size() < 4 || line[0] != '-' || line[1] != ' ') return false;
    const size_t open = 2;
    if (line[open] == '"') {
        if (line.back() != '"' || line.size() < open + 3) return false;
        const size_t b = open + 1, e = line.size() - 1;
        if (e <= b) return false;
        out.assign(line, b, e - b);
        return true;
    }
    const size_t lq = std::char_traits<char>::length(kLeftCjkQuote);
    if (line.size() >= open + lq &&
        line.compare(open, lq, kLeftCjkQuote) == 0) {
        const size_t b = open + lq;
        const size_t rq = std::char_traits<char>::length(kRightCjkQuote);
        if (line.size() < b + rq) return false;
        const size_t e = line.size() - rq;
        if (!HasSuffix(line, e, kRightCjkQuote) || e <= b) return false;
        out.assign(line, b, e - b);
        return true;
    }
    return false;
}

// 解析 "> <tag>：<value>" 引言欄位；不是此形狀即回傳 false。用 ASCII 冒號的設計註記
// （如 "> 命名原則:"）永遠不會誤判——分隔符規定為全形冒號。
bool ParseField(const std::string& line, std::string& tag,
                std::string& value) {
    if (!StartsWith(line, "> ")) return false;
    const std::string rest = line.substr(2);
    const size_t colon = rest.find(kFullWidthColon);
    if (colon == std::string::npos) return false;
    tag   = Trim(rest.substr(0, colon));
    value = Trim(rest.substr(colon + std::char_traits<char>::length(kFullWidthColon)));
    return !tag.empty();
}

int ParseSignedInt(const std::string& s) {
    return std::atoi(s.c_str());  // 允許開頭 +/- 號；尾端雜訊一律視為 0
}

}  // namespace

// 單趟逐行掃描的狀態機：cur 累積目前攤位，遇到下一個攤位標題或檔尾才 flush()
// 收尾推入 out。pendingStock 與 successCaptured 屬於當前攤位的狀態，因此每次
// flush() 都要一併重設，否則會洩漏到下一個攤位。
std::vector<VendorConfig> LoadInterludeVendors(const std::string& path) {
    std::vector<VendorConfig> out;
    std::ifstream in(path);
    if (!in) return out;

    VendorConfig cur;
    bool haveCur         = false;
    int  pendingStock    = -1;      // 來自 "> stock：N"，於攤位收尾時才套用
    bool successCaptured = false;   // 只採用第一個成交區塊
    enum class Sub { None, Greeting, Success, Leave } sub = Sub::None;

    auto flush = [&]() {
        if (!haveCur) return;
        if (pendingStock >= 0)
            for (auto& it : cur.stock) it.stockLeft = pendingStock;
        if (!cur.greetingLines.empty()) cur.greeting = cur.greetingLines[0];
        out.push_back(std::move(cur));
        cur = VendorConfig{};
        haveCur = false;
        pendingStock = -1;
        successCaptured = false;
        sub = Sub::None;
    };

    std::string line;
    while (std::getline(in, line)) {
        RStripCr(line);

        if (std::string name = ParseStallName(line); !name.empty()) {
            flush();
            haveCur = true;
            cur.name = std::move(name);
            continue;
        }
        if (!haveCur) continue;  // 跳過第一個攤位之前的章節前言註記

        if (std::string key = ParseSubsectionKey(line); !key.empty()) {
            if (key == "greeting") {
                sub = Sub::Greeting;
            } else if (key == "onPurchase" || key == "onDonate" ||
                       key == "onAccept") {
                sub = successCaptured ? Sub::None : Sub::Success;
            } else if (key == "onLeave") {
                sub = Sub::Leave;
            } else {
                sub = Sub::None;  // onChat（未捐款）等其他變體：解析得到但目前不使用
            }
            continue;
        }

        std::string tag, value;
        if (ParseField(line, tag, value)) {
            if (tag == kTagKeeper) {
                cur.stallKeeper = value;
            } else if (tag == kTagItem) {
                const size_t eq = value.find('=');
                if (eq != std::string::npos) {
                    VendorItem item;
                    item.itemId = Trim(value.substr(0, eq));
                    item.price  = ParseSignedInt(Trim(value.substr(eq + 1)));
                    cur.stock.push_back(std::move(item));
                }
            } else if (tag == kTagMechanic) {
                cur.mechanic = value;
            } else if (tag == "tier") {
                cur.tier = ParseSignedInt(value);
            } else if (tag == "karma") {
                cur.karmaOnInteract = ParseSignedInt(value);
            } else if (tag == "stock") {
                pendingStock = ParseSignedInt(value);
            }
            continue;
        }

        std::string text;
        if (ParseBulletLine(line, text)) {
            switch (sub) {
                case Sub::Greeting: cur.greetingLines.push_back(text); break;
                case Sub::Success:  cur.onPurchase.push_back(text);   break;
                case Sub::Leave:    cur.onLeave.push_back(text);      break;
                case Sub::None:     break;
            }
            if (sub == Sub::Success && !cur.onPurchase.empty())
                successCaptured = true;
        }
    }
    flush();
    return out;
}

}  // namespace nccu::vendor
