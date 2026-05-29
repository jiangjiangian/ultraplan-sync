#ifndef MESSAGE_VIEW_H_
#define MESSAGE_VIEW_H_
#include "engine/events/HudSlot.h"
#include "game/world/HudTiming.h"   // kHudTtl + kHudFade (game-side)
#include <string>

namespace nccu::engine::render { class IRenderer; }

namespace nccu {

/**
 * @file MessageView.h
 * @brief 短暫 HUD 提示框（toast）的渲染：底部／頂部分槽，含淡出與 CJK 折行。
 */

// kHudTtl + kHudFade 定義於 game/world/HudTiming.h（上方已 include），使 World
// ——它在 HudExpired() 中依 kHudTtl 老化 HUD 槽、在 DismissHud() 中對齊它——不必
// 拉進此 ui 渲染標頭即可讀取。

/**
 * @brief 繪製短暫、底部錨定的 toast，呈現最新一則 EventType::ShowMessage
 *        （任務提示／喚醒提示／章節清關旁白／漣漪反應／攤販文字），由 EventBus
 *        訂閱者鏡射進 World。
 * @param reducedMotion 開啟減少動畫時把尾段淡出收斂成硬切，使對閃光敏感的玩家
 *                      看到的是一個穩定、到 TTL 時消失的 toast（預設 false 即既
 *                      有行為）。
 * @param slot 選擇 toast 的垂直區帶。Bottom（預設）位於螢幕底部上方約 28px；Top
 *             以固定間距浮在 Bottom 之上，使兩者同幀並存時皆清晰可讀。
 *
 * 為其輸入的純函式，可 spy 測試（與 DrawDialog／DrawEndingCard 同），不保留狀
 * 態。`message` 為空或 `age` >= kHudTtl 時什麼都不畫；否則畫一層半透明背景加上
 * （CJK 折行的）文字，整條橫幅在最後 kHudFade 秒內淡至透明。slot 為純視覺位移
 * ——兩槽的淡出／TTL／折行邏輯完全相同。
 */
void DrawHudMessage(nccu::engine::render::IRenderer& r, const std::string& message,
                    float age, float screenW, float screenH,
                    bool reducedMotion = false,
                    HudSlot slot = HudSlot::Bottom);

} // namespace nccu
#endif // MESSAGE_VIEW_H_
