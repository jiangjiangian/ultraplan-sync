#include "ui/MessageView.h"
#include "ui/ReducedMotion.h"
#include "game/dialog/DialogLayout.h"   // WrapToCells / CellWidth——本專案的 EAW
                                   // 感知換行與量測（CJK 字寬計為 2 格）
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {

using namespace nccu::engine::render;
using namespace nccu::engine::math;

namespace {

// 橫幅文字大小，以及其周圍的版面內距。
constexpr int   kFontSize = 18;
constexpr float kPadX     = 18.0f;
constexpr float kPadY     = 12.0f;
constexpr float kLineH    = 24.0f;   // > kFontSize：留一點行距
constexpr float kMarginB  = 28.0f;   // 與螢幕底部的間隙

// kFontSize 下每個 EAW 字格的像素寬。本專案與字型無關的文字模型
//（EndingView::CenteredX、ChapterCard、DialogLayout.h）將全形字形推進約
//`sz` px = 2 格，故每格約 `sz/2` px。與 dialog::CellWidth 共用此值，使提示橫幅的
// 量測與對話框／結局卡片完全一致——單一事實來源，而非各自私估。
constexpr float kPxPerCell = static_cast<float>(kFontSize) * 0.5f;

// 經字格模型算出 `s` 在 kFontSize 下的像素寬。
float TextWidthPx(const std::string& s) {
    return static_cast<float>(nccu::dialog::CellWidth(s)) * kPxPerCell;
}

// 以「字格」為預算（而非私下的像素估計）來換行此提示，使用本專案 EAW 感知的
// nccu::dialog::WrapToCells，使過長的 ShowMessage 不會溢出面板，且字面的 '\n'
// 仍會強制斷行（WrapToCells 對 '\n' 的處理與舊版及 DialogView 完全相同）。
// maxWidth（px）經共用的 px/格 模型換算為字格數。
std::vector<std::string> WrapToBox(const std::string& s, float maxWidth) {
    const int maxCells =
        std::max(1, static_cast<int>(maxWidth / kPxPerCell));
    return nccu::dialog::WrapToCells(s, maxCells);
}

}  // namespace

void DrawHudMessage(IRenderer& r, const std::string& message,
                    float age, float screenW, float screenH,
                    bool reducedMotion, HudSlot slot) {
    if (message.empty() || age >= kHudTtl) return;  // 無內容可顯示

    // 存活時間 → 透明度。先維持不透明，再於最後 kHudFade 秒內由 1→0 漸隱——把倒數
    //（remaining = lifetime - elapsed，<= 0 時消失）的計時慣用法改寫成淡出。
    // 當 reducedMotion 為真時，HudToastFadeT 會把漸隱塌縮成硬切（維持不透明直到
    // TTL 邊界，再由上方 DrawHudMessage 的提前返回於一幀內隱藏提示）。
    const float remaining = kHudTtl - age;
    const float t = HudToastFadeT(remaining, kHudFade, reducedMotion);
    const auto  a = static_cast<unsigned char>(t * 255.0f);

    const float maxTextW = screenW * 0.72f;
    const std::vector<std::string> lines = WrapToBox(message, maxTextW);

    // 橫幅方框寬度取最寬的換行行，水平置中。
    // 垂直錨點取決於插槽：
    //   Bottom -> 螢幕底部 - kMarginB - boxH。
    //   Top    -> bottomBaseline - kSlotGap - boxH（位於底部插槽上方的固定視覺帶，
    //             以單行底部提示的高度為準，使 Top 帶即使只有 Top 在顯示也穩定）。
    // 此間隙採版面常數，而非讀取當前 Bottom 高度：View 先呼叫 Top 再呼叫 Bottom，
    // 兩者皆為各自狀態的純函式，若彼此耦合會被迫多跑一趟量測。兩帶間約 25-30 px
    // 可使兩行都清楚不重疊；GUI 人工驗證已確認。
    float widest = 0.0f;
    for (const std::string& ln : lines)
        widest = std::max(widest, TextWidthPx(ln));

    const float boxW = widest + kPadX * 2.0f;
    const float boxH = static_cast<float>(lines.size()) * kLineH +
                       kPadY * 2.0f;
    const float boxX = (screenW - boxW) * 0.5f;
    // 底部插槽基準線。頂部插槽則浮在其上方，距離為固定的底部單行高度再加 12 px 間隙。
    const float bottomBaseline = screenH - kMarginB;
    constexpr float kSlotGap   = 12.0f;
    const float kSingleLineH   = kLineH + kPadY * 2.0f;  // 1-line tall
    const float boxY =
        slot == HudSlot::Top
            ? bottomBaseline - kSingleLineH - kSlotGap - boxH
            : bottomBaseline - boxH;

    // 背板採用與文字相同的 alpha，使整個提示一起淡出（與 DrawEndingCard 自成一體的
    // 淡出一致）。
    r.DrawRect(Rect{boxX, boxY, boxW, boxH}, Color{20, 20, 20, a});
    r.DrawRect(Rect{boxX, boxY, boxW, 2.0f},
               Color{245, 245, 245, a});

    // 將每一換行行在方框內「置中」。方框緊貼最寬的一行，故多行提示（例如
    //「DLC開發中」上、「敬請期待」下）會呈現為兩行平衡置中，而非參差的左緣。置中位移
    // 逐行由共用字格模型算出，因此能跟隨每行的真實寬度。
    float y = boxY + kPadY;
    const float innerW = boxW - kPadX * 2.0f;
    for (const std::string& ln : lines) {
        const float lineW = TextWidthPx(ln);
        const float cx = boxX + kPadX + std::max(0.0f, (innerW - lineW) * 0.5f);
        r.DrawText(ln, Vec2{cx, y}, kFontSize, Color{245, 245, 245, a});
        y += kLineH;
    }
}

} // namespace nccu
