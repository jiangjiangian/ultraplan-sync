#ifndef ENDING_VIEW_H_
#define ENDING_VIEW_H_
#include "game/state/EndingMenuModel.h"  // IsEndingState + EndingMenuChoice +
                                           // EndingMenuChoiceAt + EndingMenuLabel
                                           // (game-side; this header renders against them)
#include "game/state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
namespace engine::render { class IRenderer; }

/**
 * @file EndingView.h
 * @brief 結局畫面的渲染：結局摘要 DTO、字面字串列舉，以及 DrawEndingCard。
 */

/**
 * @brief 列舉結局畫面可能繪製的每一個字面字串（涵蓋全部三種結局與每個判定條件
 *        分支），皆取自 DrawEndingCard 繪製時所用的「同一份」constexpr 表。
 * @return 所有結局字串。
 *
 * 字型覆蓋率測試會掃描它，使任一條未烘入字型圖集的理由句／條件標籤，在建置的圖
 * 集檢查中自動失敗（不會靜默漂移成缺字方塊）。純資料——不碰 raylib、不碰 GL。
 */
[[nodiscard]] std::vector<std::string> EndingCardStrings();

/**
 * @brief 結局畫面的純渲染 DTO。
 *
 * View.cpp 持有 World／Player，並把這些基本值萃取進此結構；EndingView 隨後在
 * 「不碰 World／Player、不含任何遊戲邏輯」的前提下渲染（MVC 純度——View 層讀取
 * 模型狀態，而不查詢它）。下列布林對應結局判定旗標，讓 EndingView 能顯示「本次
 * 通關實際觸發的具體判定條件」：
 *   karma            最終業力值（每張字卡都顯示）。
 *   hasTrueUmbrella  kFlagHasTrueUmbrella（奪回真傘，結局 A）。
 *   consoledTA       kFlagConsoledTA（終局體諒助教，結局 A）。
 *   tookCursed       kFlagTookCursedUmbrella（詛咒傘路線，結局 B）。
 *   boughtUgly       kFlagBoughtUglyUmbrella（買下醜綠傘，結局 C）。
 *   finaleChoiceMade kFlagTaFinaleChoiceMade（助教終局結算已發生）。
 * 結局 B 的「最後質問助教」(coldFinale) 由 EndingView 以
 * finaleChoiceMade && !consoledTA 推導（與 EndingGate.cpp 一致），故此 DTO 只
 * 攜帶原始旗標，不攜帶判定結果。
 */
struct EndingSummary {
    SemesterState state = SemesterState::Ending_C;
    int  karma            = 0;
    bool hasTrueUmbrella  = false;
    bool consoledTA       = false;
    bool tookCursed       = false;
    bool boughtUgly       = false;
    bool finaleChoiceMade = false;
};

// EndingMenuChoice／EndingMenuChoiceAt／EndingMenuLabel 定義於
// game/state/EndingMenuModel.h（遊戲側）；上方已 include，使僅 include
// ui/EndingView.h 的呼叫端仍能解析這些名稱。此處的渲染器只讀取它們，由
// controller 驅動。

/**
 * @brief 繪製全螢幕結局畫面。
 * @param menuCursor 底部三選項選單的游標（0..2），預設 0（回首頁），供不驅動
 *                   選單的呼叫端／測試使用。
 * @param alpha 淡入進度（0..1）。
 *
 * 黑色背景＋標題＋開場字卡，接著一段戲內「你為何走到這裡」的理由、一張顯示最終
 * 業力與本結局實際觸發判定條件的結算卡，以及一列高亮 `menuCursor` 的底部三選項
 * 選單。自我完備且可由 spy 測試（與 DrawDialog 同）：學期抵達結局時 View 會提前
 * 返回進入此函式，故理由／結算／選單「必須」在這裡渲染（結局 .md 永不被繪製，見
 * View.cpp）。summary 供給畫面所需的每個基本值；EndingView 以 constexpr 表持有
 * 標題、開場字卡、理由文案、條件標籤與選單標籤。
 */
void DrawEndingCard(nccu::engine::render::IRenderer& r, const EndingSummary& summary,
                    std::string_view title, float alpha,
                    float screenW, float screenH, int menuCursor = 0);
} // namespace nccu
#endif // ENDING_VIEW_H_
