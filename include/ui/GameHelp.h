#ifndef GAME_HELP_H_
#define GAME_HELP_H_
#include <array>
#include <string_view>

namespace nccu {

// REQUIREMENT #9: a single source of truth for the "遊戲說明" (how to
// play) text, shared by BOTH the title-screen help page and the in-game
// pause-menu 說明 overlay so the two never drift. Pure data — no raylib,
// no input — the View/TitleScreen draw these lines. Every CJK glyph here
// is baked into gfx::Font.h UiLiteralChars() (the #10 atlas lesson) so
// the panel never tofus to `?`; the 5c glyph-scan test enumerates this
// array and FAILS the build on any uncovered glyph.
//
// T4 layout: lines break at NATURAL sentence / comma / colon boundaries
// (never mid-clause). Each control key is its OWN line (the owner: split
// Tab：物品欄 and M：選單 apart); each 目標 sentence and each ending sits
// on its own logical line. A 【雨傘外觀】 section names the four umbrella
// looks so the player can tell them apart in the world (真傘藍 / 破傘剩
// 手柄 / 詛咒傘暗紫 / 醜傘綠 — the same looks gfx::DrawUmbrellaGlyph draws).
// Kept within the help panel's ~24 full-width-cell width AND the 450 px
// window height (20 text rows + 3 blanks at the View/TitleScreen pitch).
inline constexpr std::array<std::string_view, 22> kGameHelpLines = {
    "【操作】",
    "WASD／方向鍵：移動",
    "E：對話／撿取／購買",
    "Tab：開啟物品欄",
    "M：開啟選單（暫停）",
    "",
    "【目標】",
    "你的傘被拿走了。",
    "走訪政大山下，找回屬於你的透明傘。",
    "雨量會在戶外累積，躲進建築或撐傘可回復。",
    "淋太久會被沖回正門。",
    // Cycle 9.E (audit H3 / D11 / SC 2.2.1): the rain meter is an
    // unmodifiable timed pressure (5 u/s outdoors → forced respawn at
    // 100 %). SC 2.2.1 Level A requires the player to know HOW to
    // extend / pause any sub-20-hour timing; opening the pause menu (M)
    // freezes the meter (GameController::Update early-returns on
    // MenuOpen, before ApplyRain) — this line surfaces that affordance.
    // (Quitting is the 離開 item INSIDE that menu, not ESC — ESC is inert,
    // see Window.h SetExitKey.) Pure content; no engine change. The
    // test_menu_help.cpp hint check requires 暫停 + 雨壓力 on ONE line.
    "按 M 暫停可凍結雨壓力計。",
    "",
    // T4b: the four umbrella looks so the player can identify them in the
    // world / bag / ending — the SAME looks gfx::DrawUmbrellaGlyph paints.
    "【雨傘外觀】",
    "真傘：藍色，傘面完整。",
    "破傘：只剩手柄與斷骨。",
    "詛咒傘：暗紫色，傘面下垂。",
    "醜傘：螢光綠，全校最好認。",
    "",
    // Item 1e: the endings section is ACTIONABLE (spoiler-light) — it
    // names the LEVERS that decide each path (業力 / 找回真傘 / 助教態度 /
    // 買醜綠傘) instead of pure flavour, so a player knows HOW to steer.
    // No exact thresholds (kept spoiler-light); each ending is one line.
    "【三種結局】",
    "真相大白：善待人、找回真傘、體諒助教。",
    "屠龍者終成惡龍：偷傘、業力過低或對助教強硬。",
};

// The ending list's third path is the closing line so the reader leaves
// on the comedic-but-actionable note (the lever: buy the ugly green
// umbrella at 集英樓). Same ~24-cell budget; glyphs baked per #10.
inline constexpr std::string_view kGameHelpClosing =
    "破財消災：到集英樓買下醜綠傘。";

} // namespace nccu

#endif // GAME_HELP_H_
