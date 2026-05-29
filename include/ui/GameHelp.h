#ifndef GAME_HELP_H_
#define GAME_HELP_H_
#include <array>
#include <span>
#include <string_view>

namespace nccu {

// 「遊戲說明」（玩法）文字的單一真實來源，由「標題畫面說明頁」與「遊戲內暫停
// 選單說明疊層」共用，使兩者永不漂移。純資料——不碰 raylib、不碰輸入——由
// View／TitleScreen 繪製這些行。此處每個 CJK 字形皆已烘入字型圖集
// UiLiteralChars()，故面板絕不會出現缺字方塊；glyph-scan 測試會列舉此陣列，並
// 在任一未涵蓋的字形上讓建置失敗。
//
// 排版：在「自然的句／逗號／冒號」邊界斷行（絕不於子句中間斷）。每個操作鍵自成
// 一行（Tab：物品欄 與 M：選單 分開）；每句目標與每個結局各自一行。【雨傘外觀】
// 區段點名四種雨傘外觀，讓玩家能在世界中分辨它們（真傘藍／破傘剩手柄／詛咒傘暗
// 紫／醜傘綠——與 nccu::game::gfx::DrawUmbrellaGlyph 繪製的外觀一致）。每行皆在
// 說明面板約 24 全形格寬之內。
//
// 說明文字已超過一個螢幕高（依需求新增了一段【道具須知】經濟／道具使用提示），
// 故現在「分頁」（下方 kGameHelpPages）而非擠在單一面板裡用過小的行距呈現。扁平
// 的 kGameHelpLines／kGameHelpClosing 仍保留為「字形覆蓋率＋選單提示」的真實來
// 源（glyph-scan 與選單測試會逐項走訪），但渲染端（View.cpp／TitleScreen.cpp）
// 改畫「分頁」視圖，使每頁以舒適行距呈現。兩者必須一致：每行頁面內容也都出現在
// kGameHelpLines 中（下方 static_assert 釘住 page1+page2 == 扁平行數 + 結語行）。

// --- 第 1 頁：操作 + 目標 -------------------------------------------------
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
    // 雨量計是不可調整的計時壓力（戶外每秒 +5，到 100% 強制重生回正門）。無障礙
    // 準則要求玩家須知道如何延長／暫停任何計時；開啟暫停選單（M）會凍結雨量計
    // （GameController::Update 在 MenuOpen 時於 ApplyRain 之前提前返回）——此行
    // 即在揭示這個操作。（離開遊戲是該選單「內」的「離開」項，而非 ESC——ESC 無
    // 作用，見 Window.h SetExitKey。）純內容、不動引擎。選單說明測試的提示檢查
    // 要求「暫停」與「雨壓力」同在「一行」。
    "按 M 暫停可凍結雨壓力計。",
    "",
};

// --- 第 2 頁：雨傘外觀 + 道具須知 + 三種結局 ------------------------------
inline constexpr std::array<std::string_view, 18> kGameHelpPage2 = {
    // 四種雨傘外觀，讓玩家能在世界／背包／結局中辨識——與
    // nccu::game::gfx::DrawUmbrellaGlyph 繪製的外觀一致。
    "【雨傘外觀】",
    "真傘：藍色，傘面完整。",
    "破傘：只剩手柄與斷骨。",
    "詛咒傘：暗紫色，傘面下垂。",
    "醜傘：螢光綠，全校最好認。",
    "",
    // 背包本身說不清的經濟與道具使用須知。金幣跨章節保留；其餘所有攜帶物（消耗
    // 品／食物／任務道具）僅在該章節有效，進入市集時清空（SceneRouter
    // ClearConsumables）。多數道具使用時會降低雨量（ApplyConsumableEffect），
    // 而撐傘會「自動」減緩雨量（不需手動使用）。NPC 頭上的「!」標示任務給予者／
    // 去向（QuestGiverIndicator）。業力會左右你抵達三種結局中的哪一個。每行 ≤24
    // 格、字形已烘入。
    "【道具須知】",
    "金幣跨章節保留；其餘道具只在該章節有效。",
    "離開市集會清空當章的消耗品。",
    "多數道具可減緩雨量，記得善用。",
    "撐傘會自動減緩雨量，不必手動使用。",
    "頭上冒出「！」的人就是接下來的去向。",
    "善惡業力會默默決定你的結局。",
    "",
    // 結局區段是「可操作」的（少爆雷）——它點名決定各路線的關鍵槓桿（業力／找回
    // 真傘／助教態度／買醜綠傘）而非純風味文字，讓玩家知道如何引導走向。不給確切
    // 門檻（少爆雷）；每個結局一行。破財消災路線是結語行（kGameHelpClosing），讓
    // 讀者停在這個既好笑又可操作的收尾。
    "【三種結局】",
    "真相大白：善待人、找回真傘、體諒助教。",
    "屠龍者終成惡龍：偷傘、業力過低或對助教強硬。",
    "破財消災：到集英樓買下醜綠傘。",
};

// 渲染端逐項走訪的分頁視圖（View.cpp 遊戲內疊層 + TitleScreen.cpp 標題說明）。
// 翻頁為左／右（View）——每頁都帶有「第 N／M 頁」指示與「返回」標籤。對 constexpr
// 頁面陣列取 std::span：零複製，各行仍是指向上方靜態儲存的 string_view。
inline constexpr std::array<std::span<const std::string_view>, 2>
    kGameHelpPages = {
        std::span<const std::string_view>{kGameHelpPage1},
        std::span<const std::string_view>{kGameHelpPage2},
};
// kGameHelpPageCount 改放在 game/state/GameHelpPages.h，使遊戲層的場景／控制器
// （TitleScene、PauseScreen）不必拉進此 ui 標頭即可讀取。下方 static_assert 釘
// 住該字面數量與頁面陣列長度同步——新增第三頁就得「兩邊都」調整。
} // namespace nccu

#include "game/state/GameHelpPages.h"
namespace nccu {
static_assert(kGameHelpPageCount ==
              static_cast<int>(kGameHelpPages.size()),
              "ui/GameHelp.h kGameHelpPages count must match "
              "game/state/GameHelpPages.h kGameHelpPageCount — adding "
              "a page means bumping BOTH.");

// 回溯相容的扁平清單 = page1 ++ page2，保留為 glyph-scan ＋選單提示的真實來源
// （相關測試會逐項走訪）。因此每個會渲染的字形仍受此處把關。
inline constexpr std::array<std::string_view,
                            kGameHelpPage1.size() + kGameHelpPage2.size() - 1>
    kGameHelpLines = {
    // 第 1 頁
    "【操作】", "WASD／方向鍵：移動", "E：對話／撿取／購買",
    "Tab：開啟物品欄", "M：開啟選單（暫停）", "",
    "【目標】", "你的傘被拿走了。",
    "走訪政大山下，找回屬於你的透明傘。",
    "雨量會在戶外累積，躲進建築或撐傘可回復。",
    "淋太久會被沖回正門。", "按 M 暫停可凍結雨壓力計。", "",
    // 第 2 頁（其末行為破財消災結語，此處省略——它是 kGameHelpClosing、另行掃描，
    // 故陣列大小為 page1 + page2 - 1）。
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

// 結局清單的第三條路線即結語行，讓讀者停在這個既好笑又可操作的收尾（槓桿：到
// 集英樓買下醜綠傘）。它是第 2 頁的「最後一行」；獨立成自己的常數，使 glyph-scan
// 仍能涵蓋它。同樣約 24 格的字數預算。
inline constexpr std::string_view kGameHelpClosing =
    "破財消災：到集英樓買下醜綠傘。";

// 一致性把關：分頁視圖（page1 + page2）必須等於扁平 kGameHelpLines 加上那一行結
// 語，使兩者皆不會漂移到 glyph-scan（走訪扁平清單＋結語）涵蓋不到的地方。
static_assert(kGameHelpPage1.size() + kGameHelpPage2.size() ==
                  kGameHelpLines.size() + 1,
              "kGameHelpPages and kGameHelpLines+closing must stay in sync");
static_assert(kGameHelpPage2.back() == kGameHelpClosing,
              "page 2 must end on the kGameHelpClosing line");

} // namespace nccu

#endif // GAME_HELP_H_
