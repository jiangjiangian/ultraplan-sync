#ifndef GAME_HELP_H_
#define GAME_HELP_H_
#include <array>
#include <span>
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
// Every line stays within the help panel's ~24 full-width-cell width.
//
// U2-T4: the help text grew past one screenful (a 【道具須知】 economy /
// item-usage tips section was added per owner item 6), so it is now SPLIT
// INTO PAGES (kGameHelpPages below) instead of crammed into one panel at a
// tiny pitch. The flat kGameHelpLines / kGameHelpClosing are KEPT as the
// glyph-coverage + menu-hint source of truth (the 5c scan + test_menu_help
// iterate them), but the renderers (View.cpp / TitleScreen.cpp) draw the
// PAGED view so each page breathes at a comfortable pitch. The two must
// stay consistent: every page line also appears in kGameHelpLines (the
// static_assert below pins page1+page2 == flat-lines + the closing line).

// --- Page 1: 操作 + 目標 -------------------------------------------------
inline constexpr std::array<std::string_view, 13> kGameHelpPage1 = {
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
};

// --- Page 2: 雨傘外觀 + 道具須知 + 三種結局 ------------------------------
inline constexpr std::array<std::string_view, 18> kGameHelpPage2 = {
    // T4b: the four umbrella looks so the player can identify them in the
    // world / bag / ending — the SAME looks gfx::DrawUmbrellaGlyph paints.
    "【雨傘外觀】",
    "真傘：藍色，傘面完整。",
    "破傘：只剩手柄與斷骨。",
    "詛咒傘：暗紫色，傘面下垂。",
    "醜傘：螢光綠，全校最好認。",
    "",
    // U2-T4 (owner item 6): the economy + item-usage須知 the bag itself
    // can't say. 金幣 persists across chapters; every OTHER carried item
    // (消耗品 / 食物 / 任務道具) is chapter-scoped and wiped on market
    // entry (SceneRouter ClearConsumables). Most items shed 雨量 on use
    // (ApplyConsumableEffect), and an held umbrella relieves rain
    // AUTOMATICALLY (no manual use). The `!` over an NPC marks a quest
    // giver / the way forward (QuestGiverIndicator). 業力 steers which of
    // the three endings you reach. Each line ≤24 cells, glyphs baked (#10).
    "【道具須知】",
    "金幣跨章節保留；其餘道具只在該章節有效。",
    "離開市集會清空當章的消耗品。",
    "多數道具可減緩雨量，記得善用。",
    "撐傘會自動減緩雨量，不必手動使用。",
    "頭上冒出「！」的人就是接下來的去向。",
    "善惡業力會默默決定你的結局。",
    "",
    // Item 1e: the endings section is ACTIONABLE (spoiler-light) — it
    // names the LEVERS that decide each path (業力 / 找回真傘 / 助教態度 /
    // 買醜綠傘) instead of pure flavour, so a player knows HOW to steer.
    // No exact thresholds (kept spoiler-light); each ending is one line.
    // The 破財消災 path is the closing line (kGameHelpClosing) so the
    // reader leaves on the comedic-but-actionable note.
    "【三種結局】",
    "真相大白：善待人、找回真傘、體諒助教。",
    "屠龍者終成惡龍：偷傘、業力過低或對助教強硬。",
    "破財消災：到集英樓買下醜綠傘。",
};

// The paged view the renderers iterate (View.cpp in-game overlay +
// TitleScreen.cpp title help). Page-flip is Left/Right (View) — a 「第 N
// ／M 頁」 indicator + the 返回 chip ride on every page. std::span over the
// constexpr page arrays: zero copies, the lines stay string_views into the
// static storage above.
inline constexpr std::array<std::span<const std::string_view>, 2>
    kGameHelpPages = {
        std::span<const std::string_view>{kGameHelpPage1},
        std::span<const std::string_view>{kGameHelpPage2},
};
// Phase 4 layering: kGameHelpPageCount now lives in
// game/state/GameHelpPages.h so the game-layer scenes / controllers
// (TitleScene, PauseScreen) can read it without pulling this ui
// header. The static_assert below pins that the literal count and the
// page array length stay in lock-step — adding a third page means
// bumping BOTH.
} // namespace nccu

#include "game/state/GameHelpPages.h"
namespace nccu {
static_assert(kGameHelpPageCount ==
              static_cast<int>(kGameHelpPages.size()),
              "ui/GameHelp.h kGameHelpPages count must match "
              "game/state/GameHelpPages.h kGameHelpPageCount — adding "
              "a page means bumping BOTH.");

// Back-compat flat list = page1 ++ page2, KEPT as the glyph-scan + menu-
// hint source of truth (test_font_ui_glyph_scan / test_menu_help iterate
// it). Every glyph that renders is therefore still gated here.
inline constexpr std::array<std::string_view,
                            kGameHelpPage1.size() + kGameHelpPage2.size() - 1>
    kGameHelpLines = {
    // page 1
    "【操作】", "WASD／方向鍵：移動", "E：對話／撿取／購買",
    "Tab：開啟物品欄", "M：開啟選單（暫停）", "",
    "【目標】", "你的傘被拿走了。",
    "走訪政大山下，找回屬於你的透明傘。",
    "雨量會在戶外累積，躲進建築或撐傘可回復。",
    "淋太久會被沖回正門。", "按 M 暫停可凍結雨壓力計。", "",
    // page 2 (its last line is the 破財消災 closing, omitted here — it is
    // kGameHelpClosing, scanned separately, so the array size is page1 +
    // page2 - 1).
    "【雨傘外觀】", "真傘：藍色，傘面完整。", "破傘：只剩手柄與斷骨。",
    "詛咒傘：暗紫色，傘面下垂。", "醜傘：螢光綠，全校最好認。", "",
    "【道具須知】",
    "金幣跨章節保留；其餘道具只在該章節有效。",
    "離開市集會清空當章的消耗品。",
    "多數道具可減緩雨量，記得善用。",
    "撐傘會自動減緩雨量，不必手動使用。",
    "頭上冒出「！」的人就是接下來的去向。",
    "善惡業力會默默決定你的結局。", "",
    "【三種結局】",
    "真相大白：善待人、找回真傘、體諒助教。",
    "屠龍者終成惡龍：偷傘、業力過低或對助教強硬。",
};

// The ending list's third path is the closing line so the reader leaves
// on the comedic-but-actionable note (the lever: buy the ugly green
// umbrella at 集英樓). It is the LAST line of page 2; kept as its own
// constant so the 5c glyph-scan still covers it. Same ~24-cell budget.
inline constexpr std::string_view kGameHelpClosing =
    "破財消災：到集英樓買下醜綠傘。";

// Consistency gate: the paged view (page1 + page2) must equal the flat
// kGameHelpLines + the one closing line, so neither can drift uncovered by
// the glyph-scan (which iterates the flat list + closing).
static_assert(kGameHelpPage1.size() + kGameHelpPage2.size() ==
                  kGameHelpLines.size() + 1,
              "kGameHelpPages and kGameHelpLines+closing must stay in sync");
static_assert(kGameHelpPage2.back() == kGameHelpClosing,
              "page 2 must end on the kGameHelpClosing line");

} // namespace nccu

#endif // GAME_HELP_H_
