#ifndef DIALOG_VIEW_H_
#define DIALOG_VIEW_H_
#include "game/dialog/DialogState.h"

namespace nccu::engine::render { class IRenderer; }

namespace nccu {

/**
 * @file DialogView.h
 * @brief 螢幕空間的對話框繪製進入點。
 */

/**
 * @brief 繪製對話框；對話非作用中時不畫任何東西。
 * @param r 注入的渲染器。
 * @param d 要繪製的對話狀態（const 讀取）。
 *
 * 面板 Rect{20,320,760,110}：先填底、再一道細邊框。台詞模式 → 當前行為一段文字；
 * 選單模式 → 每個選項一段文字，選中者前綴 "> "。目前為佔位框，待 resources/assets/ui/
 * 美術到位後換成 sprite。
 */
void DrawDialog(nccu::engine::render::IRenderer& r, const DialogState& d);

} // namespace nccu
#endif // DIALOG_VIEW_H_
