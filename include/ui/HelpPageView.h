#ifndef HELP_PAGE_VIEW_H_
#define HELP_PAGE_VIEW_H_
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include <functional>
#include <string_view>

namespace nccu::ui {

/**
 * @file HelpPageView.h
 * @brief 共用的「遊戲說明」分頁渲染器，供遊戲內暫停選單疊層與標題畫面說明頁
 *        共用，去除兩處幾乎相同的繪製重複。
 *
 * 兩處呼叫端都畫出相同的面板＋「遊戲說明」標題＋分頁的 GameHelp 內文（行距 17／
 * 空行 8、金色【…】區段標題）＋「第 N／M 頁」指示＋金邊「返回」標籤。僅少數數
 * 值不同；此 helper 把那些值收為參數，使兩處與先前「逐像素一致」又共用同一份程
 * 式碼。
 *
 * 刻意與 renderer 無關：呼叫端傳入一個 `fillRect` 可呼叫物件，使遊戲內疊層
 * （IRenderer&）與標題畫面（具體 Renderer）都能重用，而不耦合到單一繪圖類別。
 * 文字直接透過 TextBuilder 繪製（與兩處呼叫端原本作法相同）。純呈現——不碰
 * World／Player、不碰輸入（MVC）。
 */

/// @brief 兩處原始呼叫端之間「不同」的參數；其餘（由 w／h 推導的幾何、行距、
///        標題偵測）皆共用。
struct HelpPageStyle {
    float                  w;             ///< 視口／視窗寬
    float                  h;             ///< 視口／視窗高
    int                    page;          ///< 要繪製的頁（0 起算，會夾限）
    nccu::engine::math::Color       panelColor;    ///< 疊層 245α 對標題 200α
    nccu::engine::math::Color       indicatorColor;///< 疊層亮色對標題暗灰
    std::string_view       chipLabel;     ///< 「M / E 返回選單」對「Enter / E 返回」
    float                  chipLabelXOffset; ///< 疊層 -58 對標題 -56
};

/**
 * @brief 透過 `fillRect`（畫矩形）與 TextBuilder（畫文字）繪製面板、標題、當前
 *        說明頁內文、頁碼指示與「返回」標籤。
 *
 * 在此之「前」繪製的任何背景由呼叫端負責（疊層的全螢幕遮罩／標題畫面的 Clear）
 * ——那些各不相同，留在呼叫端。
 */
void DrawHelpPage(const std::function<void(nccu::engine::math::Rect,
                                           nccu::engine::math::Color)>& fillRect,
                  const HelpPageStyle& style);

} // namespace nccu::ui

#endif // HELP_PAGE_VIEW_H_
