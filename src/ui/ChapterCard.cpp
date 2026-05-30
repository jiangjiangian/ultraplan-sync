#include "ui/ChapterCard.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/dialog/DialogLayout.h"   // CellWidth——本專案的文字量測輔助函式
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
using namespace nccu::game::gfx;  // game/gfx 輔助函式

using namespace nccu::engine::render;
using namespace nccu::engine::math;

namespace {

// `s` 是否為章節（而非插曲段／結局）？章節「開始」觸發「遺失」字卡；章節 -> 插曲段
// 觸發「尋回」字卡。
bool IsChapter(SemesterState s) noexcept {
    switch (s) {
        case SemesterState::Chapter1_AddDrop:
        case SemesterState::Chapter2_Midterms:
        case SemesterState::Chapter3_SportsDay:
        case SemesterState::Chapter4_Finals:
            return true;
        default:
            return false;
    }
}

// 在沒有渲染器端 MeasureText（IRenderer 無此功能——與 EndingView/MessageView 受同一
// 限制）的情況下做水平置中。CellWidth 是本專案的東亞寬度文字量測（CJK = 2 格、ASCII =
// 1 格）；在字體大小 `sz` 下，每格字形推進約 sz/2 px，故 `s` 的像素寬約為
// CellWidth(s)*sz/2。回傳可使其置中的 x。
float CenteredX(std::string_view s, int sz, float screenW) {
    const float w = static_cast<float>(nccu::dialog::CellWidth(std::string{s})) *
                    (static_cast<float>(sz) * 0.5f);
    const float x = screenW * 0.5f - w * 0.5f;
    return x < 0.0f ? 0.0f : x;
}

// 在字體大小 `sz`、沿用共用的每格約 sz/2 px 模型下，`widthPx` 內可容納的字格數，使卡片
// 文字在面板內換行、而非衝出方框。至少取 1，使窄面板仍能輸出列。
int CellsForWidth(float widthPx, int sz) {
    const float perCell = static_cast<float>(sz) * 0.5f;
    int n = static_cast<int>(widthPx / perCell);
    return n < 1 ? 1 : n;
}

// 以本專案 EAW 感知的 nccu::dialog::WrapToCells 把 `s` 換行到螢幕文字寬度（左右各內縮
// 一段側邊距），再把每列置中畫在 `y`、依 `lineH` 遞進。回傳最後一列「之後」的 y。
float DrawCenteredWrapped(IRenderer& r, const std::string& s, int sz,
                          float screenW, float marginPx, float y,
                          float lineH, Color col) {
    const float textW = std::max(40.0f, screenW - marginPx * 2.0f);
    for (const std::string& row :
         nccu::dialog::WrapToCells(s, CellsForWidth(textW, sz))) {
        r.DrawText(row, Vec2{CenteredX(row, sz, screenW), y}, sz, col);
        y += lineH;
    }
    return y;
}

} // namespace

ChapterCardKind ChapterCardForTransition(SemesterState from,
                                         SemesterState to) noexcept {
    if (from == to) return ChapterCardKind::None;     // 非真正的轉場
    // 某章節正要開始（遊戲開始進入第一章，或插曲段離開進入第二／三／四章）：作為引信的
    //「傘又掉了」節拍。
    if (IsChapter(to)) return ChapterCardKind::Lost;
    // 某章節剛通關（其收尾旁白已關閉，FSM 此刻跳往市集）：「找到傘了」的回報。第四章絕不
    // 到達此處——它轉場到某個 Ending_* 狀態，由 EndingView 掌管。
    if (to == SemesterState::Interlude_Market && IsChapter(from))
        return ChapterCardKind::Found;
    return ChapterCardKind::None;
}

std::string_view ChapterCardHeadline(ChapterCardKind kind,
                                     SemesterState to) noexcept {
    switch (kind) {
        case ChapterCardKind::Lost:
            // 第一章是引發事件——玩家自己的透明傘從綜院傘架消失（章節開場），故第一個變體
            // 較貼切；第二至四章則是反覆出現的「又一次」節拍。
            return to == SemesterState::Chapter1_AddDrop
                       ? "傘，不見了"
                       : "傘又掉了";
        case ChapterCardKind::Found:
            return "找到傘了";
        case ChapterCardKind::None:
        default:
            return "";
    }
}

std::string ChapterCardSubtitle(ChapterCardKind kind, SemesterState to) {
    switch (kind) {
        case ChapterCardKind::Lost: {
            // 標出正要開始的章節，使這個大節拍同時兼作章節標題卡。字形即章節名稱
            //（經建築／說明字集 + ChapterCardStrings 掃描已涵蓋於字形圖集）。
            switch (to) {
                case SemesterState::Chapter1_AddDrop: return "第一章 加退選";
                case SemesterState::Chapter2_Midterms: return "第二章 期中考";
                case SemesterState::Chapter3_SportsDay: return "第三章 運動會";
                case SemesterState::Chapter4_Finals:   return "第四章 期末";
                default: return "";
            }
        }
        case ChapterCardKind::Found:
            // 市集插曲段前的一段簡短尾聲。
            return "這一章，過去了";
        case ChapterCardKind::None:
        default:
            return "";
    }
}

std::vector<std::string> ChapterCardStrings() {
    std::vector<std::string> out;
    const SemesterState states[] = {SemesterState::Chapter1_AddDrop,
                                    SemesterState::Chapter2_Midterms,
                                    SemesterState::Chapter3_SportsDay,
                                    SemesterState::Chapter4_Finals};
    for (SemesterState s : states) {
        out.emplace_back(ChapterCardHeadline(ChapterCardKind::Lost, s));
        out.push_back(ChapterCardSubtitle(ChapterCardKind::Lost, s));
    }
    out.emplace_back(
        ChapterCardHeadline(ChapterCardKind::Found, SemesterState::Interlude_Market));
    out.push_back(
        ChapterCardSubtitle(ChapterCardKind::Found, SemesterState::Interlude_Market));
    return out;
}

float ChapterCardState::Alpha(bool reducedMotion) const noexcept {
    if (kind_ == ChapterCardKind::None) return 0.0f;
    if (reducedMotion) return 1.0f;            // 無亮度漸變，硬切
    if (elapsed_ < kFade)                      // 淡入
        return std::clamp(elapsed_ / kFade, 0.0f, 1.0f);
    const float fadeOutStart = kTotal - kFade;
    if (elapsed_ > fadeOutStart)               // 淡出
        return std::clamp((kTotal - elapsed_) / kFade, 0.0f, 1.0f);
    return 1.0f;                               // 維持
}

void ChapterCardState::Trigger(ChapterCardKind kind, std::string_view headline,
                               std::string subtitle) noexcept {
    kind_     = kind;
    headline_ = std::string{headline};
    subtitle_ = std::move(subtitle);
    elapsed_  = 0.0f;
}

void ChapterCardState::Step(float dt) noexcept {
    if (kind_ == ChapterCardKind::None) return;
    elapsed_ += dt;
    if (elapsed_ >= kTotal) Dismiss();
}

void ChapterCardState::Dismiss() noexcept {
    kind_    = ChapterCardKind::None;
    elapsed_ = 0.0f;
    headline_.clear();
    subtitle_.clear();
}

void DrawChapterCard(IRenderer& r, const ChapterCardState& card,
                     float screenW, float screenH, bool reducedMotion) {
    if (!card.Active()) return;
    const float alpha = card.Alpha(reducedMotion);
    if (alpha <= 0.0f) return;
    const auto a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);

    // 1) 全螢幕變暗，使這個節拍讀作刻意的中斷，有別於角落的提示橫幅。比結局背板更淺
    //    （世界仍隱約可見於下方——這是短暫的節拍，而非整個畫面接管）。
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH},
               Color{0, 0, 0, static_cast<unsigned char>(alpha * 170.0f)});

    // 2) 一個置中的橫幅面板——比提示框更大／更醒目。尺寸為橫跨螢幕中段的固定帶。
    const float panelH = 150.0f;
    const float panelY = screenH * 0.5f - panelH * 0.5f;
    r.DrawRect(Rect{0.0f, panelY, screenW, panelH},
               Color{16, 18, 26, static_cast<unsigned char>(alpha * 225.0f)});
    // 上下各一條金線，使該帶框住標題。
    r.DrawRect(Rect{0.0f, panelY, screenW, 3.0f}, Color{255, 200, 70, a});
    r.DrawRect(Rect{0.0f, panelY + panelH - 3.0f, screenW, 3.0f},
               Color{255, 200, 70, a});

    // 3) 標題左側的雨傘字形提示：遺失 -> 破傘的傘骨（「又弄丟了」）；尋回 -> 真傘的藍色
    //    傘面（「拿回來了」）。與世界中雨傘／拾取物／結局卡片所用的共用字形相同，使圖示
    //    語彙處處一致。隨淡出做 alpha 縮放。
    const UmbrellaLook look = (card.Kind() == ChapterCardKind::Found)
                                  ? UmbrellaLook::TrueBlue
                                  : UmbrellaLook::FragileBroken;
    constexpr float kUmbW = 46.0f, kUmbH = 50.0f;
    DrawUmbrellaGlyph(r, look,
                      Rect{screenW * 0.5f - 150.0f, screenH * 0.5f - kUmbH * 0.5f,
                           kUmbW, kUmbH}, a);

    // 4) 大而醒目的標題（置中），下方是較小的副標。
    constexpr int kHeadSize = 40;
    constexpr int kSubSize  = 18;
    const std::string_view head = card.Headline();
    const std::string&     sub  = card.Subtitle();
    // 尋回 = 暖金（寬慰）；遺失 = 較冷的米白（失去的節拍）。
    const Color headColor = (card.Kind() == ChapterCardKind::Found)
                                ? Color{255, 210, 90, a}
                                : Color{235, 235, 240, a};
    const float headY = screenH * 0.5f - static_cast<float>(kHeadSize) * 0.5f - 12.0f;
    // 把標題 + 副標在側邊距內換行，使未來較長的卡片字串能在帶內自動換行、而非衝出方框。
    // 標題依設計即短（8 格），故此處通常只輸出一列置中文字；換行是預留的安全網。
    constexpr float kSideMargin = 56.0f;
    DrawCenteredWrapped(r, std::string{head}, kHeadSize, screenW, kSideMargin,
                        headY, static_cast<float>(kHeadSize) + 4.0f, headColor);
    if (!sub.empty())
        DrawCenteredWrapped(r, sub, kSubSize, screenW, kSideMargin,
                            headY + 50.0f, 22.0f, Color{200, 200, 210, a});
}

} // namespace nccu
