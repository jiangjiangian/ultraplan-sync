#include "doctest/doctest.h"
#include "ui/QuestGiverIndicator.h"
#include "game/entities/NPC.h"
#include "engine/render/IRenderer.h"

#include <vector>

/**
 * @file test_quest_giver_indicator.cpp
 * @brief 驗證任務發派者頭上的「!」指示器：繪製的面板與字形、顏色契約、
 *        浮於精靈頭頂上方的佈局、隨 hitbox 在世界座標移動，以及
 *        IsQuestGiver 透過 GameObject 虛擬分派的行為與預設值。
 */

namespace {

// 攔截用的 IRenderer：記錄每個繪圖原語，使多型的 Render() 路徑可在無頭環境下
// 測試（不需 GL 環境，也不會有任何 raylib 繪圖呼叫真正送進畫面）。
struct Spy final : nccu::engine::render::IRenderer {
    struct RectCall { nccu::engine::math::Rect r; nccu::engine::math::Color c; };
    struct TextCall { std::string s; nccu::engine::math::Vec2 pos; int size;
                      nccu::engine::math::Color c; };
    std::vector<RectCall> rects;
    std::vector<TextCall> texts;
    int sprites = 0;

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view text, nccu::engine::math::Vec2 pos, int size,
                  nccu::engine::math::Color c) override {
        texts.push_back({std::string{text}, pos, size, c});
    }
};

} // namespace

// 被標記為任務發派者的 NPC 必須在精靈上方畫出可見的「!」標記，讓玩家一眼看出
// 對話入口。繪製層曾完全忽略 IsQuestGiver() —— 該旗標在生成端有作用，卻在 View
// 端不可見。下列測試固定此指示器輔助函式，避免回退。

// 對任務發派者繪製面板與「!」字形。
TEST_CASE("QuestGiverIndicator: draws a panel + glyph for a quest-giver") {
    Spy spy;
    // 位於世界座標 (400, 1860) 的 24x24 hitbox —— 對應苦主的生成座標。
    const nccu::engine::math::Rect hb{400.0f, 1860.0f, 24.0f, 24.0f};
    nccu::DrawQuestGiverIndicator(spy, hb);

    // 此輔助函式恰好畫兩個矩形（陰影 + 金色面板）與一個「!」字形 —— 幾何上足以
    // 讓玩家一眼看見、又小到不會擠到精靈。
    REQUIRE(spy.rects.size() == 2);
    REQUIRE(spy.texts.size() == 1);
    CHECK(spy.sprites == 0);
    CHECK(spy.texts[0].s == "!");
    // 金色面板用 raylib 的金色 #FFC83D，而非純白。
    const auto goldPanel = spy.rects[1];
    CHECK(goldPanel.c.r == 255);
    CHECK(goldPanel.c.g == 200);
    CHECK(goldPanel.c.b == 61);
    CHECK(goldPanel.c.a == 255);
    // 陰影為深色半透明，以免在畫面上挖出一個洞。
    const auto shadow = spy.rects[0];
    CHECK(shadow.c.r == 0);
    CHECK(shadow.c.g == 0);
    CHECK(shadow.c.b == 0);
    CHECK(shadow.c.a < 255);
}

// 指示器的佈局浮在 NPC 精靈頭頂上方。
TEST_CASE("QuestGiverIndicator: layout floats above the NPC sprite top") {
    // NPC::Render 把 32 高的精靈以底部對齊在 24 高的 hitbox 上：
    // spriteTopY = hitBox.y + hitBox.height - 32。指示器必須嚴格位於該 y 之上，
    // 並留有明顯間距（不與頭部重疊）。
    const nccu::engine::math::Rect hb{100.0f, 200.0f, 24.0f, 24.0f};
    const auto L = nccu::LayoutQuestGiverIndicator(hb);

    const float spriteTopY = hb.y + hb.height - 32.0f;  // = 192
    const float iconBottom = L.panel.y + L.panel.height;
    CHECK(iconBottom < spriteTopY);          // 浮於上方
    CHECK(spriteTopY - iconBottom >= 10.0f); // 留有可見的間距

    // 水平方向對齊 hitbox 中心，使這個「!」讀起來就是「指這個 NPC」。
    const float iconCenterX = L.panel.x + L.panel.width * 0.5f;
    const float hbCenterX   = hb.x + hb.width * 0.5f;
    CHECK(iconCenterX == doctest::Approx(hbCenterX));

    // 圖示很小（16 px 見方），故絕不會擠到精靈。
    CHECK(L.panel.width  == doctest::Approx(16.0f));
    CHECK(L.panel.height == doctest::Approx(16.0f));
}

// 指示器佈局在世界座標中跟隨 hitbox。
TEST_CASE("QuestGiverIndicator: layout tracks the hitbox in world space") {
    // View 在 CameraScope 內繪製指示器，故此輔助函式必須產生世界座標 ——
    // 移動 NPC 時圖示必須在兩軸都位移相同的量。
    const auto A = nccu::LayoutQuestGiverIndicator(
        nccu::engine::math::Rect{100.0f, 200.0f, 24.0f, 24.0f});
    const auto B = nccu::LayoutQuestGiverIndicator(
        nccu::engine::math::Rect{160.0f, 260.0f, 24.0f, 24.0f});

    CHECK(B.panel.x - A.panel.x == doctest::Approx(60.0f));
    CHECK(B.panel.y - A.panel.y == doctest::Approx(60.0f));
    // 文字隨面板位移相同的量。
    CHECK(B.textPos.x - A.textPos.x == doctest::Approx(60.0f));
    CHECK(B.textPos.y - A.textPos.y == doctest::Approx(60.0f));
}

// NPC::IsQuestGiver 透過 GameObject 的虛擬分派回應。
TEST_CASE("NPC::IsQuestGiver routes through GameObject virtual dispatch") {
    // View 走訪 world.Objects()（GameObject&），透過虛擬基底詢問 IsQuestGiver()。
    // 生成時標記為 true 的 NPC 必須透過該基底參考回應 true —— 否則任何 NPC 的
    // 「!」標記都會悄悄永遠不出現。
    NPC giver(nccu::engine::math::Vec2{400, 1860},
              std::vector<std::string>{"hi"},
              /*isQuestGiver=*/true,
              "victim");
    NPC bystander(nccu::engine::math::Vec2{500, 1860},
                  std::vector<std::string>{"hi"},
                  /*isQuestGiver=*/false,
                  "bookworm");
    const GameObject& giverBase     = giver;
    const GameObject& bystanderBase = bystander;
    CHECK(giverBase.IsQuestGiver());            // 虛擬分派生效
    CHECK_FALSE(bystanderBase.IsQuestGiver());  // 並尊重旗標
}

// 一般 GameObject 的 IsQuestGiver 預設為 false。
TEST_CASE("Plain GameObject defaults IsQuestGiver to false") {
    // 基底類別預設必須維持 false，使道具、玩家、商人、裝飾物件與環境學生都不會
    // 畫出多餘的「!」。此處用非 NPC 的 GameObject 子類別證明繼承閉合性。
    // GameObject 已不再是肥介面（介面隔離的角色拆分已移除其 Update/Render/Interact
    // 純虛擬函式 —— 現為可選的角色介面），故空子類別不扮演任何角色、無需覆寫。
    struct StubObj final : GameObject {
        StubObj() : GameObject(nccu::engine::math::Vec2{0, 0},
                               nccu::engine::math::Rect{0, 0, 1, 1}) {}
    };
    StubObj o;
    const GameObject& base = o;
    CHECK_FALSE(base.IsQuestGiver());
}
