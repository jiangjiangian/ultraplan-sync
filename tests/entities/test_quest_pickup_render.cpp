#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/entities/Player.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/quest/Chapter1Quest.h"

#include <vector>

/**
 * @file test_quest_pickup_render.cpp
 * @brief 驗證 QuestFlagPickup::Render 的型別感知算繪：紙張類任務物件畫白色紙張，
 *        苦主的透明傘畫共用的藍色雨傘圖形；且任務物件只畫矩形，不畫 sprite／文字。
 */

namespace {

// 間諜 IRenderer：記錄每個繪圖基元而不觸碰 GL 環境，讓多型的 Render() 路徑
// 可在無視窗環境下測試。
struct CountingRenderer final : nccu::engine::render::IRenderer {
    struct RectCall { nccu::engine::math::Rect r; nccu::engine::math::Color c; };
    std::vector<RectCall> rects;
    int sprites = 0;
    int texts = 0;

    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override {
        ++texts;
    }
};

bool HasColor(const CountingRenderer& s, nccu::engine::math::Color want) {
    for (const auto& rc : s.rects) if (rc.c == want) return true;
    return false;
}

} // namespace

// 任務物件的算繪是型別感知的：紙張類（申請書／筆記）畫白色紙張，苦主的傘畫
// 共用的藍色雨傘圖形。任務物件只畫矩形，絕不畫 sprite 或文字（架構規則）。
TEST_CASE("QuestFlagPickup::Render：紙張類任務物件畫白色紙張") {
    QuestFlagPickup form(nccu::engine::math::Vec2{120.0f, 80.0f}, nccu::kFlagFoundForm);
    CountingRenderer spy;
    form.Render(spy);

    CHECK(spy.sprites == 0);          // 佔位標記，無 sprite
    CHECK(spy.texts == 0);            // 任務物件不畫文字
    REQUIRE(spy.rects.size() >= 1);
    // 紙張用白色：紙張本體為 White（不是舊的 Yellow）。
    CHECK(HasColor(spy, nccu::engine::math::Colors::White));
    CHECK_FALSE(HasColor(spy, nccu::engine::math::Colors::Yellow));
}

// 苦主的透明傘畫成藍色雨傘圖形。
TEST_CASE("QuestFlagPickup::Render：苦主的透明傘畫成藍色雨傘圖形") {
    // Ch1 苦主的透明傘會設定 Flag_HasVictimUmbrella，必須算繪成真傘（藍色），
    // 與世界中的雨傘／結局卡片所用的是同一個共用圖形，而非黃色方塊。
    QuestFlagPickup umb(nccu::engine::math::Vec2{200.0f, 300.0f},
                        nccu::kFlagHasVictimUmbrella,
                        "撿到一把眼熟的透明傘");
    CountingRenderer spy;
    umb.Render(spy);

    CHECK(spy.sprites == 0);
    CHECK(spy.texts == 0);
    REQUIRE(spy.rects.size() >= 3);   // 真正的雨傘輪廓
    // 它的標誌色是真傘藍（共用外觀色），絕不會是 Yellow。
    CHECK(HasColor(spy,
        nccu::game::gfx::UmbrellaLookColor(nccu::game::gfx::UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasColor(spy, nccu::engine::math::Colors::Yellow));
}
