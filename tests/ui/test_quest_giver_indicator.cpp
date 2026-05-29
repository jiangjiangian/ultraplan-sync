#include "doctest/doctest.h"
#include "ui/QuestGiverIndicator.h"
#include "game/entities/NPC.h"
#include "engine/render/IRenderer.h"

#include <vector>

namespace {

// Spy IRenderer — mirrors the proven CountingRenderer pattern used in
// test_quest_pickup_render.cpp / test_umbrella_render.cpp. Records every
// primitive so the polymorphic Render() path is testable headless (no GL
// context, no raylib draw call escapes into the world).
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

// H4: NPCs flagged as quest-givers must paint a visible "!" overlay above
// their sprite so the player can spot the dialog hook at a glance. The
// rendering layer used to ignore IsQuestGiver() entirely — the flag was
// load-bearing on the spawn side but invisible at the View. These tests
// pin down the indicator helper so the regression cannot creep back.

TEST_CASE("QuestGiverIndicator: draws a panel + glyph for a quest-giver") {
    Spy spy;
    // 24x24 hitbox at world (400, 1860) — mirrors the 苦主 spawn coord.
    const nccu::engine::math::Rect hb{400.0f, 1860.0f, 24.0f, 24.0f};
    nccu::DrawQuestGiverIndicator(spy, hb);

    // The helper paints exactly two rects (drop shadow + gold panel) and
    // one "!" glyph — enough geometry for a player to spot at a glance,
    // small enough to never crowd the sprite.
    REQUIRE(spy.rects.size() == 2);
    REQUIRE(spy.texts.size() == 1);
    CHECK(spy.sprites == 0);
    CHECK(spy.texts[0].s == "!");
    // The gold panel uses Raylib's Gold-ish #FFC83D, not pure white.
    const auto goldPanel = spy.rects[1];
    CHECK(goldPanel.c.r == 255);
    CHECK(goldPanel.c.g == 200);
    CHECK(goldPanel.c.b == 61);
    CHECK(goldPanel.c.a == 255);
    // Drop shadow is dark and translucent so it doesn't punch a hole.
    const auto shadow = spy.rects[0];
    CHECK(shadow.c.r == 0);
    CHECK(shadow.c.g == 0);
    CHECK(shadow.c.b == 0);
    CHECK(shadow.c.a < 255);
}

TEST_CASE("QuestGiverIndicator: layout floats above the NPC sprite top") {
    // NPC::Render bottom-anchors a 32-tall sprite on the 24-tall hitbox:
    // spriteTopY = hitBox.y + hitBox.height - 32. The indicator must sit
    // strictly ABOVE that y, with a clear gap (no overlap with the head).
    const nccu::engine::math::Rect hb{100.0f, 200.0f, 24.0f, 24.0f};
    const auto L = nccu::LayoutQuestGiverIndicator(hb);

    const float spriteTopY = hb.y + hb.height - 32.0f;  // = 192
    const float iconBottom = L.panel.y + L.panel.height;
    CHECK(iconBottom < spriteTopY);          // floats above
    CHECK(spriteTopY - iconBottom >= 10.0f); // visible breathing room

    // Horizontally centred on the hitbox so the bang reads as "this NPC".
    const float iconCenterX = L.panel.x + L.panel.width * 0.5f;
    const float hbCenterX   = hb.x + hb.width * 0.5f;
    CHECK(iconCenterX == doctest::Approx(hbCenterX));

    // Icon is small (16 px square) so it never crowds the sprite.
    CHECK(L.panel.width  == doctest::Approx(16.0f));
    CHECK(L.panel.height == doctest::Approx(16.0f));
}

TEST_CASE("QuestGiverIndicator: layout tracks the hitbox in world space") {
    // The View draws the indicator INSIDE the CameraScope, so the helper
    // must produce world coordinates — moving the NPC must move the icon
    // by the same delta on both axes.
    const auto A = nccu::LayoutQuestGiverIndicator(
        nccu::engine::math::Rect{100.0f, 200.0f, 24.0f, 24.0f});
    const auto B = nccu::LayoutQuestGiverIndicator(
        nccu::engine::math::Rect{160.0f, 260.0f, 24.0f, 24.0f});

    CHECK(B.panel.x - A.panel.x == doctest::Approx(60.0f));
    CHECK(B.panel.y - A.panel.y == doctest::Approx(60.0f));
    // Text follows the panel by the same delta.
    CHECK(B.textPos.x - A.textPos.x == doctest::Approx(60.0f));
    CHECK(B.textPos.y - A.textPos.y == doctest::Approx(60.0f));
}

TEST_CASE("NPC::IsQuestGiver routes through GameObject virtual dispatch") {
    // The View loops world.Objects() (GameObject&) and asks
    // IsQuestGiver() through the virtual base. A NPC flagged true at
    // spawn must answer true through that base reference — otherwise the
    // "!" overlay would silently never fire for any NPC (H4 root cause).
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
    CHECK(giverBase.IsQuestGiver());            // virtual dispatch ON
    CHECK_FALSE(bystanderBase.IsQuestGiver());  // and respects the flag
}

TEST_CASE("Plain GameObject defaults IsQuestGiver to false") {
    // The base class default must stay false so items, the player, Vendor,
    // decorative objects and ambient students never paint a stray "!".
    // Use a non-NPC GameObject subclass to prove inheritance closure.
    // GameObject is no longer a fat interface (the ISP role split removed
    // its Update/Render/Interact pure-virtuals — those are now opt-in role
    // interfaces), so a bare subclass plays no roles and needs no overrides.
    struct StubObj final : GameObject {
        StubObj() : GameObject(nccu::engine::math::Vec2{0, 0},
                               nccu::engine::math::Rect{0, 0, 1, 1}) {}
    };
    StubObj o;
    const GameObject& base = o;
    CHECK_FALSE(base.IsQuestGiver());
}
