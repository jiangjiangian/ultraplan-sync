#include "doctest/doctest.h"
#include "ui/EndingView.h"
#include "game/state/SemesterState.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::EndingSummary;

/**
 * @file test_ending_card_render.cpp
 * @brief 驗證結局結算卡的繪製：依結局顯示業力數字與實際生效的判定條件、
 *        對應結局的傘色塊（A 真傘藍 / B 詛咒傘暗紫 / C 醜傘綠 / D 破傘）、
 *        單一結算標題，以及不溢出畫面文字區的換行。
 */

namespace {

// 攔截用的 IRenderer：計數矩形（並擷取其顏色，使結局傘色塊可斷言）並擷取文字，
// 使結局卡的繪圖呼叫可在無 GL 環境下斷言。此外也記錄每段文字的繪製 X 與字級，
// 用以斷言畫面內換行（無任何卡片列溢出框）。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<nccu::engine::math::Color> rectColors;
    std::vector<std::string> texts;
    std::vector<float>       textX;
    std::vector<int>         textSize;

    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color c) override {
        ++rects;
        rectColors.push_back(c);
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::engine::math::Vec2 p, int sz,
                  nccu::engine::math::Color) override {
        texts.emplace_back(t);
        textX.push_back(p.x);
        textSize.push_back(sz);
    }
};

// 若任一擷取到的文字含有 needle 子字串則為 true。
bool Has(const Spy& s, std::string_view needle) {
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) return true;
    return false;
}

int CountWith(const Spy& s, std::string_view needle) {
    int n = 0;
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) ++n;
    return n;
}

// 若任一擷取到的矩形 RGB == want 則為 true（忽略 alpha，因卡片淡入會縮放它）。
// 傘形圖的代表色是其頂部傘面色塊。
bool HasRectRGB(const Spy& s, nccu::engine::math::Color want) {
    for (const nccu::engine::math::Color& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

} // namespace

// IsEndingState 僅對 Ending_* 狀態為 true。
TEST_CASE("IsEndingState is true only for Ending_* states") {
    CHECK(nccu::IsEndingState(SemesterState::Ending_C));
    CHECK_FALSE(nccu::IsEndingState(SemesterState::Chapter1_AddDrop));
}

// DrawEndingCard 會畫出背景、標題、字卡與結算統計。
TEST_CASE("DrawEndingCard issues a backdrop + title + caption + stats") {
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.boughtUgly = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(r.rects >= 2);              // 背景 + 結算面板
    CHECK(r.texts.size() >= 4);       // 標題 + 字卡 + 緣由 + 統計列
}

// 卡片必須依資料物件呈現業力「數字」以及每個結局對應的劇情緣由／判定條件。

// 結局 A 卡會顯示業力與 A 的三項判定條件。
TEST_CASE("Ending A card shows karma + all three A deciding conditions") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 92;
    g.hasTrueUmbrella = true;
    g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "92"));              // 實際的最終業力數字
    CHECK(Has(r, "業力"));            // 業力讀數標籤
    CHECK(Has(r, "業力 > 80"));       // A 條件 1
    CHECK(Has(r, "還回真傘"));         // A 條件 2
    CHECK(Has(r, "體諒助教"));         // A 條件 3
    CHECK(Has(r, "完美結局"));         // 路線標籤
    // A 的緣由台詞（取自結局 A 的字卡）。
    CHECK(Has(r, "辛苦了"));
}

// 結局 B（冷漠最終選擇）只顯示實際生效的條件。
TEST_CASE("Ending B cold-finale shows ONLY the conditions that fired") {
    // coldFinale = finaleChoiceMade && !consoledTA；業力非 <0；非詛咒傘。
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = 35;
    g.finaleChoiceMade = true;
    g.consoledTA = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "35"));
    CHECK(Has(r, "最後質問助教"));      // 實際生效的那一項
    CHECK_FALSE(Has(r, "拿了詛咒傘"));  // 未生效 -> 不顯示
    CHECK_FALSE(Has(r, "業力低於零"));  // 業力 35 >= 0 -> 不顯示
    CHECK(Has(r, "墮落結局"));
}

// 結局 B（詛咒傘 + 負業力）會同時顯示兩項生效的條件。
TEST_CASE("Ending B cursed+negative-karma shows both fired conditions") {
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = -7;
    g.tookCursed = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "-7"));
    CHECK(Has(r, "拿了詛咒傘"));
    CHECK(Has(r, "業力低於零"));
}

// 結局 C（買了醜傘）顯示購買條件，而非平穩收尾的預設條件。
TEST_CASE("Ending C bought-ugly shows the buy condition, not the default") {
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 50;
    g.boughtUgly = true;
    g.finaleChoiceMade = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "50"));
    CHECK(Has(r, "買了醜傘"));
    CHECK_FALSE(Has(r, "平穩收尾"));   // 生效的是購買，而非預設
    CHECK(Has(r, "務實結局"));
}

// 結局 C（平穩收尾的預設分支）顯示「平穩收尾」條件。
TEST_CASE("Ending C calm-default shows the 平穩收尾 condition") {
    // finaleChoiceMade && !boughtUgly -> 判定的 C 預設分支。
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 60;
    g.finaleChoiceMade = true;
    g.boughtUgly = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "平穩收尾"));
    CHECK_FALSE(Has(r, "買了醜傘"));
}

// 結局卡恰好畫出一個被選定的「結算」標題（作為錨點）。
TEST_CASE("Ending card draws exactly one selected '結算' header (anchor)") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 81;
    g.hasTrueUmbrella = true;
    g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    CHECK(CountWith(r, "結算") == 1);
}

// 卡片依「結局」決定要畫哪把最終的傘，故絕不會與結論不符（修正「體諒卻顯示醜傘」）。
// A → 真傘藍、B → 詛咒傘暗紫、C → 醜傘綠 —— 共用傘形圖的代表色可證明畫的是哪一把。
TEST_CASE("T3: Ending A draws the 真傘 (blue) umbrella swatch") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 90; g.hasTrueUmbrella = true; g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    using nccu::game::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
}

// 結局 B 畫出詛咒傘（暗紫）色塊。
TEST_CASE("T3: Ending B draws the 詛咒傘 (dark purple) umbrella swatch") {
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = -5; g.tookCursed = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    using nccu::game::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

// 結局 C 畫出醜傘（綠）色塊 —— 即使玩家也體諒了助教。
TEST_CASE("T3: Ending C draws the 醜傘 (green) umbrella swatch — even when 體諒") {
    // 「體諒卻顯示醜傘」的不符：色塊由「結局」推導，而非原始旗標。即使是體諒了
    // 助教的 C 結局，也仍顯示綠色醜傘，因為這一輪結算為 C。
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 60; g.boughtUgly = true; g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    using nccu::game::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

// 結局 D 的卡片資料契約：IsEndingState 認得 D；卡片會畫出破傘（FragileBroken）
// 色塊與 D 的字卡／緣由／風雨同行路線標籤，以及 D 的兩項判定條件（體諒 + 業力≤80）。

// IsEndingState 認得 Ending_D。
TEST_CASE("G1: IsEndingState recognises Ending_D") {
    CHECK(nccu::IsEndingState(SemesterState::Ending_D));
}

// 結局 D 卡會畫出破傘（FragileBroken）色塊與 D 專屬文案。
TEST_CASE("G1: Ending D card draws the 破傘 (FragileBroken) swatch + D copy") {
    EndingSummary g;
    g.state = SemesterState::Ending_D;
    g.karma = 65;                       // 體諒且業力落在 [0,80]
    g.consoledTA = true;
    g.finaleChoiceMade = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 D", 1.0f, 800.0f, 450.0f);
    using nccu::game::gfx::UmbrellaLook;
    // D 對應的破傘傘形圖 —— 而非真傘／詛咒傘／醜傘。
    CHECK(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    // D 專屬的卡片文案。
    CHECK(Has(r, "65"));                 // 最終業力數字
    CHECK(Has(r, "風雨同行"));            // 路線標籤
    CHECK(Has(r, "體諒助教"));            // D 條件 1
    CHECK(Has(r, "業力 ≤ 80"));          // D 條件 2（差一點點的條款）
    CHECK(Has(r, "傘破了"));              // D 的一段字卡／緣由片段
}

// 確認實際的 DrawEndingCard 渲染完整 D 卡的方式與 A/B/C 相同（D 資料流經每個
// 共用分支），且不會有任何 A/B/C 專屬的假設把錯誤文案洩漏進 D。卡片必須顯示
// D 的字卡、風雨同行路線、一段 D 緣由台詞，以及恰好一個結算標題（與 A 固定的
// 單一標題錨點相同）—— 證明 D 不是走捷徑落到某個預設分支。
TEST_CASE("U1-T1: Ending D renders its caption + reason + path like A/B/C") {
    EndingSummary g;
    g.state = SemesterState::Ending_D;
    g.karma = 70;
    g.consoledTA = true;
    g.finaleChoiceMade = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 D", 1.0f, 800.0f, 450.0f);
    CHECK(r.rects >= 2);                 // 背景 + 結算面板（與 A/B/C 相同）
    CHECK(CountWith(r, "結算") == 1);    // 恰好一個被選定的標題，非 0 或 2
    CHECK(Has(r, "傘破了"));              // D 字卡片段有渲染
    CHECK(Has(r, "把傘磨破了"));          // 一段 D 緣由台詞（實際渲染，非僅查表）
    CHECK(Has(r, "風雨同行結局"));        // D 路線標籤，而非 A/B/C 的
    // D 不可借用其他結局的路線標籤／字卡。
    CHECK_FALSE(Has(r, "完美結局"));
    CHECK_FALSE(Has(r, "墮落結局"));
    CHECK_FALSE(Has(r, "務實結局"));
}

// 結局卡的任何文字列都不會溢出黑框。卡片文字原本是硬塞進 800×450 而未換行；
// 此處固定修正：字卡與緣由台詞現會在側邊留白內換行（nccu::dialog::WrapToCells），
// 故在較小與較窄寬度下，每段繪製列的像素寬都留在畫面文字區內。以共用的字寬模型
// （每個東亞字寬約 size/2 px、與渲染器換行所用相同的量測）斷言。
TEST_CASE("UI-B-3: every ending-card row stays within the screen text area") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 90;
    g.hasTrueUmbrella = true;
    g.consoledTA = true;
    for (const float screenW : {800.0f, 520.0f, 420.0f}) {
        Spy r;
        nccu::DrawEndingCard(r, g, "結局 A 真相大白", 1.0f, screenW, 450.0f);
        REQUIRE(r.texts.size() == r.textX.size());
        REQUIRE(r.texts.size() == r.textSize.size());
        for (std::size_t i = 0; i < r.texts.size(); ++i) {
            const int sz = r.textSize[i];
            const float perCell = static_cast<float>(sz) * 0.5f;
            const float rowPx =
                static_cast<float>(nccu::dialog::CellWidth(r.texts[i])) * perCell;
            const float rightEdge = r.textX[i] + rowPx;
            INFO("screenW=" << screenW << " row='" << r.texts[i]
                 << "' x=" << r.textX[i] << " right=" << rightEdge);
            // Every row's left edge is on-screen and its right edge does not
            // run off the screen's right side (a small slack for the px model).
            CHECK(r.textX[i] >= -0.5f);
            CHECK(rightEdge <= screenW + 2.0f);
        }
    }
}
