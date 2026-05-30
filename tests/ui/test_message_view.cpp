#include "doctest/doctest.h"
#include "ui/MessageView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

/**
 * @file test_message_view.cpp
 * @brief 驗證 DrawHudMessage：空訊息與超過 TTL 不繪製、無空白字元的長中文
 *        字串自動換行、明確換行強制斷行，以及訊息框置中與不溢出黑框的排版。
 */

namespace {

// 攔截用的 IRenderer：計數矩形並擷取文字，使 DrawHudMessage 的繪圖呼叫可在無
// GL 環境下斷言。此外也擷取每段文字的繪製 X 與最寬的底框矩形，用以斷言置中與
// 框內換行（繪製位置是「置中」唯一可觀察的依據）。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<std::string>   texts;
    std::vector<float>         textX;
    int                        fontSize = 0;
    nccu::engine::math::Rect            backdrop{0, 0, 0, 0};   // 目前見過最寬的矩形

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color) override {
        ++rects;
        if (r.width > backdrop.width) backdrop = r;     // 框是最寬的矩形
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int sz,
                  nccu::engine::math::Color) override {
        texts.emplace_back(t);
        textX.push_back(p.x);
        fontSize = sz;
    }
};

} // namespace

// 空的 HUD 訊息不論存活時間多少都不繪製任何東西。
TEST_CASE("空的 HUD 訊息不論存活時間都不繪製任何東西") {
    Spy s;
    nccu::DrawHudMessage(s, "", 0.0f, 800.0f, 450.0f);
    nccu::DrawHudMessage(s, "", 2.0f, 800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

// 新的 HUD 訊息會繪製底框與文字。
TEST_CASE("新的 HUD 訊息會繪製底框與文字") {
    Spy s;
    nccu::DrawHudMessage(s, "撿到一頁學霸的筆記。", 0.0f, 800.0f, 450.0f);
    CHECK(s.rects >= 1);                       // 底框（+ 強調條）
    REQUIRE(s.texts.size() >= 1);
    CHECK(s.texts[0] == "撿到一頁學霸的筆記。"); // 短 → 單行
}

// 超過 TTL 的 HUD 訊息不繪製。
TEST_CASE("超過 TTL 的 HUD 訊息不繪製") {
    Spy s;
    nccu::DrawHudMessage(s, "已過期的旁白", nccu::kHudTtl + 0.5f,
                         800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

// 恰在 TTL 邊界的 HUD 訊息不繪製（邊界為包含式）。
TEST_CASE("恰在 TTL 邊界的 HUD 訊息不繪製") {
    Spy s;
    nccu::DrawHudMessage(s, "邊界", nccu::kHudTtl, 800.0f, 450.0f);
    CHECK(s.rects == 0);
    CHECK(s.texts.empty());
}

// 無空白字元的長中文字串會依量得寬度自動換行（不靠空白斷行）。
TEST_CASE("無空白的長中文字串會換成多行（不靠空白斷行）") {
    Spy s;
    // 約 40 個中文字搭配狹窄視窗 → 必定斷行。
    const std::string longLine =
        "這是一段很長的章節清關旁白用來測試在沒有空白字元的中文字串"
        "上面是否能夠依照量得的寬度自動換行而不會切斷一個字。";
    nccu::DrawHudMessage(s, longLine, 0.5f, 480.0f, 320.0f);
    CHECK(s.texts.size() >= 2);                 // 已換行
    // 任何換行片段都不得超過原文長度，且把它們接回去必須完全重現原文
    // （不可漏字或切字）。
    std::string joined;
    for (const std::string& ln : s.texts) joined += ln;
    CHECK(joined == longLine);
}

// 訊息中的明確換行字元會強制斷行。
TEST_CASE("訊息中的明確換行字元會強制斷行") {
    Spy s;
    nccu::DrawHudMessage(s, "第一行\n第二行", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0] == "第一行");
    CHECK(s.texts[1] == "第二行");
}

// DLC 預告以兩行各自置中呈現。
// 訊息框緊貼最寬的一行（DLC開發中，6 個 ASCII + 3 個中文字寬），故較短的
// 敬請期待（4 個中文字 = 8 字寬）會比寬行更靠右 → 其 X 嚴格較大。這個右移正是
// 逐行置中可觀察的特徵（靠左對齊的框會把兩行畫在同一 X）。
TEST_CASE("DLC 預告以兩行各自置中呈現") {
    Spy s;
    nccu::DrawHudMessage(s, "DLC開發中\n敬請期待", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 2);
    CHECK(s.texts[0] == "DLC開發中");
    CHECK(s.texts[1] == "敬請期待");
    // 較窄的第二行（8 字寬）位於緊貼較寬第一行（6+6=12 字寬）的框內，故被推到
    // 第一行的右側。
    CHECK(s.textX[1] > s.textX[0]);
    // 兩行的起始都在框的左內緣或其右側（皆在框內）。
    CHECK(s.textX[0] >= s.backdrop.x);
    CHECK(s.textX[1] >= s.backdrop.x);
}

// 單行訊息會被置於它（緊貼的）框內。
// 其繪製 X 等於框的左內緣（框緊貼這一行，故置中偏移約為 0）。重點是：單行提示
// 的觀感維持不變。
TEST_CASE("單行訊息被置於它的框內") {
    Spy s;
    nccu::DrawHudMessage(s, "撿到 50 元", 0.0f, 800.0f, 450.0f);
    REQUIRE(s.texts.size() == 1);
    CHECK(s.textX[0] >= s.backdrop.x);
    CHECK(s.textX[0] <= s.backdrop.x + s.backdrop.width);
}

// 長提示絕不溢出黑框：每段換行列都符合面板的內部字寬上限。
// 使用本專案東亞寬度感知的 CellWidth（與換行所用同一量測），即規格要求的
// 框內容納判定。框緊貼最寬列，故「容納於框內」等同「無列寬於最寬列」，此外也
// 確認沒有任何列超過依 72% 螢幕文字寬度推得的字寬上限。
TEST_CASE("長提示在黑框的字寬內換行") {
    Spy s;
    const float screenW = 800.0f, screenH = 450.0f;
    const std::string longLine =
        "這是一段非常長的旁白用來確認訊息框會把過長的中文字串自動換行而"
        "不會衝出黑色底框的右邊界一個字都不能溢出框外才算通過這個測試。";
    nccu::DrawHudMessage(s, longLine, 0.5f, screenW, screenH);
    REQUIRE(s.texts.size() >= 2);                  // 確實有換行

    // 字型模型：每個東亞字寬約 fontSize/2 px。框的內寬為 backdrop.width 減去
    // 兩側各 18px 的邊距。任何換行列都不得寬於此內寬（否則就溢出框了）。
    REQUIRE(s.fontSize > 0);
    const float perCell  = static_cast<float>(s.fontSize) * 0.5f;
    const float innerW   = s.backdrop.width - 18.0f * 2.0f;
    for (const std::string& row : s.texts) {
        const float rowPx =
            static_cast<float>(nccu::dialog::CellWidth(row)) * perCell;
        INFO("row='" << row << "' rowPx=" << rowPx << " innerW=" << innerW);
        CHECK(rowPx <= innerW + 0.5f);             // 容納於框內
    }
    // 框本身也符合「72% 文字預算 + 邊距」，亦即從未長過螢幕所允許的寬度。
    CHECK(s.backdrop.width <= screenW * 0.72f + 18.0f * 2.0f + 0.5f);

    // 不漏字／不切字：把各列接回去可重現原文。
    std::string joined;
    for (const std::string& ln : s.texts) joined += ln;
    CHECK(joined == longLine);
}
