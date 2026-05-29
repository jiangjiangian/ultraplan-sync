#include "ui/overlay/HelpOverlay.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"

#include <algorithm>

namespace nccu {

void DrawHelpOverlay(nccu::engine::render::IRenderer& r,
                     const World& world,
                     float screenW,
                     float screenH) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 遊戲內說明（玩法說明）疊層——畫在選單「之上」（選單仍在其後方）。為
    // World::HelpOpen() + World::HelpPage() 的純函式；與標題畫面使用同一份共用的
    // GameHelp 文字，故兩者不會走樣。一個近全螢幕的面板，顯示（現已分頁的）說明中的
    // 一「頁」；←/→ 翻頁，M/E/Enter（皆於 GameController 處理）將其關閉並退回選單
    //（ESC 結束程式）。
    if (!(world.MenuOpen() && world.HelpOpen())) return;

    const float W = screenW;
    const float H = screenH;
    // 蓋在暫停選單「之上」的全螢幕遮罩（僅疊層用——標題畫面改為 Clear，故此處留在
    // 呼叫點）。
    r.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 205});
    // 共用的「遊戲說明」頁面主體（去重）：與標題畫面所繪的面板／標題／分頁主體／頁碼
    // 指示／返回小標相同。此疊層採用的值：245 alpha 面板、淺色指示、位於 -58 的
    //「M / E 返回選單」小標。
    nccu::ui::DrawHelpPage(
        [&r](Rect rect, Color c) { r.DrawRect(rect, c); },
        nccu::ui::HelpPageStyle{
            W, H,
            std::max(0, std::min(world.HelpPage(),
                                 nccu::kGameHelpPageCount - 1)),
            Color{18, 20, 28, 245},
            Color{200, 200, 210, 255},
            "M / E 返回選單", -58.0f});
}

}  // namespace nccu
