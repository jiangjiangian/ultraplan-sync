#include "vendor/VendorLoader.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace nccu::vendor {

namespace {

// The source file is UTF-8; markdown read from disk is UTF-8 bytes. So
// std::string::find / compare on UTF-8 string literals is a byte match —
// exactly what we want. (DialogLoader uses \x escapes for the few code
// points it needs; here we use the literals directly for clarity, since
// the field tags are whole words and hand-computing their bytes is the
// one mistake worth designing out.)
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

// "## 攤位：<name>" -> <name>. Empty if not a stall heading. "## 10 攤位
// lineup" is safely rejected (does not start with "## 攤位").
std::string ParseStallName(const std::string& line) {
    if (!StartsWith(line, kStallPrefix)) return {};
    const std::string rest = line.substr(kStallPrefix.size());
    const size_t colon = rest.find(kFullWidthColon);
    if (colon == std::string::npos) return {};
    return Trim(rest.substr(colon + std::char_traits<char>::length(kFullWidthColon)));
}

// "### <key>…" -> base key (token before whitespace or a （ qualifier).
// e.g. "### onPurchase（陷阱傘殘骸）" -> "onPurchase".
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

// Inner text of a `- "…"` bullet (ASCII " or CJK U+201C/U+201D). False on
// any mismatch — identical contract to DialogLoader::ParseDialogLine.
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

// "> <tag>：<value>" blockquote field. False if not that shape. ASCII-colon
// design notes ("> 命名原則:") never match — the separator is full-width.
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
    return std::atoi(s.c_str());  // leading +/- ok, trailing junk -> 0
}

}  // namespace

std::vector<VendorConfig> LoadInterludeVendors(const std::string& path) {
    std::vector<VendorConfig> out;
    std::ifstream in(path);
    if (!in) return out;

    VendorConfig cur;
    bool haveCur         = false;
    int  pendingStock    = -1;      // from "> stock：N", applied at close
    bool successCaptured = false;   // first success block wins
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
        if (!haveCur) continue;  // skip §-intro notes before the first stall

        if (std::string key = ParseSubsectionKey(line); !key.empty()) {
            if (key == "greeting") {
                sub = Sub::Greeting;
            } else if (key == "onPurchase" || key == "onDonate" ||
                       key == "onAccept") {
                sub = successCaptured ? Sub::None : Sub::Success;
            } else if (key == "onLeave") {
                sub = Sub::Leave;
            } else {
                sub = Sub::None;  // onChat（未捐款） & other variants: recorded-not-used
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
