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
// the panel never tofus to `?`. Lines are kept short (the help panel is
// narrower than the dialog box; ~22 full-width cells max).
inline constexpr std::array<std::string_view, 17> kGameHelpLines = {
    "【操作】",
    "WASD / 方向鍵：移動",
    "E：對話 / 撿取 / 購買",
    "Tab：物品欄  M：選單",
    "",
    "【目標】",
    "你的傘被拿走了。走訪政大山下，",
    "找回屬於你的那把透明傘。",
    "雨量會在戶外累積——躲進建築或",
    "撐傘可回復，淋太久會被沖回正門。",
    // Cycle 9.E (audit H3 / D11 / SC 2.2.1): the rain meter is an
    // unmodifiable timed pressure (5 u/s outdoors → forced respawn at
    // 100 %). SC 2.2.1 Level A requires the player to know HOW to
    // extend / pause any sub-20-hour timing; opening the pause menu (M)
    // freezes the meter (GameController::Update early-returns on
    // MenuOpen, before ApplyRain) — this line surfaces that affordance
    // so a player who needs a break knows the path. (Quitting is the 離開
    // item INSIDE that menu, not ESC — ESC is inert, see Window.h
    // SetExitKey.) Pure content; no engine change, within the help budget.
    "M 暫停會凍結雨壓力計。",
    "",
    // Item 1e: the endings section is ACTIONABLE (spoiler-light) — it
    // names the LEVERS that decide each path (業力 / 找回真傘 / 助教結算
    // 的態度 / 買醜綠傘) instead of pure flavour, so a player knows HOW to
    // steer. No exact thresholds (kept spoiler-light). Every glyph here is
    // baked into gfx::Font.h UiLiteralChars() (the #10 atlas lesson) and
    // each line stays within the help panel's ~22 full-width-cell budget.
    "【三種結局】",
    "真相大白：累積業力善待人，",
    "找回真傘，最後體諒助教。",
    "屠龍者終成惡龍：偷傘、業力",
    "過低，或最後對助教強硬。",
};

// The ending list's third path is the closing line so the reader leaves
// on the comedic-but-actionable note (the lever: buy the ugly green
// umbrella at 集英樓). Same ~22-cell budget; glyphs baked per #10.
inline constexpr std::string_view kGameHelpClosing =
    "破財消災：到集英樓買下醜綠傘。";

} // namespace nccu

#endif // GAME_HELP_H_
