#include "doctest/doctest.h"
#include "ui/ChapterCard.h"
#include "game/state/SemesterState.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"
#include <string>
#include <vector>

using nccu::ChapterCardKind;
using nccu::ChapterCardState;
using nccu::ChapterCardForTransition;
using nccu::ChapterCardHeadline;
using nccu::ChapterCardSubtitle;
using S = nccu::SemesterState;

/**
 * @file test_chapter_card.cpp
 * @brief 驗證章節轉場大卡（「傘不見了 / 找到傘了」書擋）：依轉場分類卡片種類、
 *        標題與副標文案、淡入→停留→淡出→自動消失的計時狀態機，以及繪製內容
 *        （傘的視覺提示）與不溢出面板的換行。
 */

namespace {

// 攔截用的 IRenderer：擷取文字與矩形顏色，使大卡的繪圖呼叫可在無 GL 環境下斷言。
// 此外也記錄每段文字的繪製 X 與字級，用以斷言面板內換行（無任何卡片列溢出框）。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    std::vector<nccu::engine::math::Color> rectColors;
    std::vector<std::string> texts;
    std::vector<float>       textX;
    std::vector<int>         textSize;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color c) override {
        ++rects; rectColors.push_back(c);
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int sz,
                  nccu::engine::math::Color) override {
        texts.emplace_back(t);
        textX.push_back(p.x);
        textSize.push_back(sz);
    }
};
bool Has(const Spy& s, std::string_view needle) {
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) return true;
    return false;
}
bool HasRectRGB(const Spy& s, nccu::engine::math::Color want) {
    for (const auto& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

} // namespace

// ---- 轉場分類（書擋規則）-----------------------------------------------

// 章節「開始」會觸發 Lost 卡。
TEST_CASE("ChapterCardForTransition: a chapter START fires the Lost card") {
    // 遊戲開始（哨兵 -> 第一章）以及每次從幕間離開進入章節。
    CHECK(ChapterCardForTransition(S::Ending_C, S::Chapter1_AddDrop) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter2_Midterms) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter3_SportsDay) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter4_Finals) == ChapterCardKind::Lost);
}

// 章節「清關」會觸發 Found 卡。
TEST_CASE("ChapterCardForTransition: a chapter CLEAR fires the Found card") {
    // 第一／二／三章 -> 市集（章節在收尾旁白結束後剛清關）。第四章不會到這裡
    // （它會走向結局）。
    CHECK(ChapterCardForTransition(S::Chapter1_AddDrop, S::Interlude_Market) == ChapterCardKind::Found);
    CHECK(ChapterCardForTransition(S::Chapter2_Midterms, S::Interlude_Market) == ChapterCardKind::Found);
    CHECK(ChapterCardForTransition(S::Chapter3_SportsDay, S::Interlude_Market) == ChapterCardKind::Found);
}

// 走向結局時不觸發任何卡（結局畫面由 EndingView 負責）。
TEST_CASE("ChapterCardForTransition: endings fire NO card (EndingView owns them)") {
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_A) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_B) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_D) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_C) == ChapterCardKind::None);
}

// 同狀態的「轉場」不觸發任何卡。
TEST_CASE("ChapterCardForTransition: a same-state 'transition' fires nothing") {
    CHECK(ChapterCardForTransition(S::Chapter2_Midterms, S::Chapter2_Midterms) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Interlude_Market) == ChapterCardKind::None);
}

// ---- 文案 ---------------------------------------------------------------

// 標題：第一章用開場變體，第二到四章用反覆出現的變體。
TEST_CASE("Headline: Ch1 uses the inciting variant, Ch2-4 the recurring one") {
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter1_AddDrop) == "傘，不見了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter2_Midterms) == "傘又掉了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter4_Finals) == "傘又掉了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Found, S::Interlude_Market) == "找到傘了");
    CHECK(ChapterCardHeadline(ChapterCardKind::None, S::Chapter1_AddDrop) == "");
}

// 副標：Lost 卡會標出開始的章節名稱。
TEST_CASE("Subtitle: a Lost card names the starting chapter") {
    CHECK(ChapterCardSubtitle(ChapterCardKind::Lost, S::Chapter2_Midterms) == "第二章 期中考");
    CHECK(ChapterCardSubtitle(ChapterCardKind::Found, S::Interlude_Market) == "這一章，過去了");
}

// ---- 確定性的計時狀態機 -------------------------------------------------

// 計時狀態機：淡入 -> 停留 -> 淡出 -> 自動消失。
TEST_CASE("ChapterCardState: fade-in -> hold -> fade-out -> auto-clear") {
    ChapterCardState c;
    CHECK_FALSE(c.Active());
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第二章 期中考");
    CHECK(c.Active());
    CHECK(c.Kind() == ChapterCardKind::Lost);
    CHECK(c.Headline() == "傘又掉了");
    CHECK(c.Subtitle() == "第二章 期中考");

    // t=0 時 alpha 約為 0（剛武裝），在 kFade 期間漸增。
    CHECK(c.Alpha() == doctest::Approx(0.0f));
    c.Step(ChapterCardState::kFade * 0.5f);
    CHECK(c.Alpha() == doctest::Approx(0.5f));        // 淡入中段
    c.Step(ChapterCardState::kFade * 0.5f);
    CHECK(c.Alpha() == doctest::Approx(1.0f));        // 進入完全停留

    // 停留：過程中仍為 1.0。
    c.Step(0.5f);
    CHECK(c.Alpha() == doctest::Approx(1.0f));

    // 推進到淡出視窗內，確認 alpha 降到 1 以下。
    while (c.Active() && c.Elapsed() < ChapterCardState::kTotal - ChapterCardState::kFade * 0.5f)
        c.Step(0.01f);
    CHECK(c.Active());
    CHECK(c.Alpha() < 1.0f);
    CHECK(c.Alpha() > 0.0f);

    // 超過 kTotal 後自動消失。
    c.Step(ChapterCardState::kFade);
    CHECK_FALSE(c.Active());
    CHECK(c.Alpha() == doctest::Approx(0.0f));
}

// Dismiss 會立即清除（玩家按鍵略過）。
TEST_CASE("ChapterCardState: Dismiss clears immediately (key-press skip)") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Found, "找到傘了", "這一章，過去了");
    c.Step(0.5f);
    CHECK(c.Active());
    c.Dismiss();
    CHECK_FALSE(c.Active());
    CHECK(c.Kind() == ChapterCardKind::None);
}

// 減少動畫時立即全不透明，無漸變。
TEST_CASE("ChapterCardState: reducedMotion is opaque immediately, no ramp") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第三章 運動會");
    CHECK(c.Alpha(/*reducedMotion=*/true) == doctest::Approx(1.0f));  // t=0 即不透明
    c.Step(0.5f);
    CHECK(c.Alpha(true) == doctest::Approx(1.0f));
}

// 對未啟用的卡呼叫 Step 是安全的無操作。
TEST_CASE("ChapterCardState: Step on an inactive card is a safe no-op") {
    ChapterCardState c;
    c.Step(1.0f);
    CHECK_FALSE(c.Active());
    CHECK(c.Alpha() == doctest::Approx(0.0f));
}

// ---- DrawChapterCard（攔截器）-----------------------------------------

// 未啟用的卡不繪製任何東西。
TEST_CASE("DrawChapterCard: inactive card draws nothing") {
    ChapterCardState c;
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(r.rects == 0);
    CHECK(r.texts.empty());
}

// Lost 卡會畫出標題、副標與破傘的視覺提示。
TEST_CASE("DrawChapterCard: a Lost card draws headline + subtitle + 破傘 cue") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第二章 期中考");
    c.Step(0.5f);                                   // 進入完全不透明的停留
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(r.rects >= 3);                            // 背景 + 面板 + 分隔線
    CHECK(Has(r, "傘又掉了"));
    CHECK(Has(r, "第二章 期中考"));
    using nccu::game::gfx::UmbrellaLook;
    // Lost 卡提示的是失去的傘（破傘骨架），而非完整的傘。
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

// Found 卡會畫出「找到傘了」與真傘（藍）的視覺提示。
TEST_CASE("DrawChapterCard: a Found card draws 找到傘了 + the 真傘 (blue) cue") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Found, "找到傘了", "這一章，過去了");
    c.Step(0.5f);
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(Has(r, "找到傘了"));
    using nccu::game::gfx::UmbrellaLook;
    // Found 卡提示的是尋回的真傘（藍），而非破傘骨架。
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
}

// 任何章節卡的標題／副標列都不會溢出面板。卡片現會把標題與副標在側邊留白內
// 換行（nccu::dialog::WrapToCells），故每段繪製列的右緣都留在畫面內，即使在
// 較窄寬度也是。以共用的字寬模型（每個東亞字寬約 size/2 px）斷言。
TEST_CASE("UI-B-3: every chapter-card row stays within the screen width") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第一章 加退選");
    c.Step(0.5f);
    for (const float screenW : {800.0f, 480.0f, 360.0f}) {
        Spy r;
        nccu::DrawChapterCard(r, c, screenW, 450.0f);
        REQUIRE(r.texts.size() == r.textX.size());
        REQUIRE(r.texts.size() == r.textSize.size());
        for (std::size_t i = 0; i < r.texts.size(); ++i) {
            const float perCell = static_cast<float>(r.textSize[i]) * 0.5f;
            const float rowPx =
                static_cast<float>(nccu::dialog::CellWidth(r.texts[i])) * perCell;
            INFO("screenW=" << screenW << " row='" << r.texts[i] << "'");
            CHECK(r.textX[i] >= -0.5f);
            CHECK(r.textX[i] + rowPx <= screenW + 2.0f);
        }
    }
}
