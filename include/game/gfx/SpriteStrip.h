#ifndef GFX_SPRITE_STRIP_H_
#define GFX_SPRITE_STRIP_H_
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "game/state/SemesterState.h"
#include <cmath>

/**
 * @file SpriteStrip.h
 * @brief 純（無 raylib、無 GL、無模擬）的 sprite strip 動畫運算工具。
 *
 * 「strip」是一張水平 PNG，單列等寬排 N 個影格（第 i 格佔 x 軸 texel
 * [i*frameW, (i+1)*frameW)、滿高）。整張以「一個」texture 載入，再以
 * DrawTexturePro 切出每影格的來源矩形做動畫——這是 raylib 的慣用作法。本標頭
 * 只負責運算：某時刻該顯示哪一格、以及該格的來源矩形。實際貼圖在 View 經
 * IRenderer::DrawSprite 完成，使 Item／Model 程式碼永不碰 raylib（架構紅線），
 * 且缺 texture 時自然不畫任何東西。
 *
 * 此處皆為 constexpr／純函式，故影格邏輯可在無 GL context 下做無頭單元測試。
 */

namespace nccu::game::gfx {

/**
 * @brief 在經過時間 `t`（秒）時，`n` 影格、每秒前進 `fps` 格之 strip 的
 *        乒乓（三角波）影格索引。
 * @param t   經過時間（秒）；可為負，仍安全。
 * @param n   strip 影格數。
 * @param fps 每秒前進影格數。
 * @return [0, n) 內的影格索引。
 *
 * 序列為「先正後逆」並循環：0,1,2,...,n-1,n-2,...,1, 0,1,...（週期 2*(n-1)
 * tick）。例如 n=4 得 0,1,2,3,2,1,...——當圖隨影格放大時呈現「呼吸／放大縮小」感。
 * 純正向迴圈（0..n-1,0..）會在末格直接「跳」回首格；乒乓則緩緩折返，使放大型
 * strip 看起來是平滑的進出脈動。
 *
 * 退化輸入皆完整且安全：n<=1（或 fps<=0、或 t 非有限）回傳影格 0——單影格
 * 「strip」即靜態 sprite，而停擺／無效時鐘會停在影格 0 而非越界索引。負的 `t`
 * 亦有處理（取模時化為非負代表值），使異常時鐘永不產生負索引。
 */
[[nodiscard]] inline int FrameAt(double t, int n, double fps) noexcept {
    if (n <= 1 || fps <= 0.0 || !std::isfinite(t)) return 0;
    // 自 t=0 起的整數 tick 數。用 floor()（非截斷）讓負輸入在 t==0 處仍單調遞增
    const double ticksF = std::floor(t * fps);
    // 2*(n-1) 為乒乓週期 tick 數（去程 n-1、回程 n-1）
    const long period = 2L * (static_cast<long>(n) - 1L);
    // 摺回 [0, period) 而不對負值觸發 UB：C++ % 可能為負，為負時補一個週期
    long k = static_cast<long>(ticksF);
    long m = k % period;
    if (m < 0) m += period;
    // 三角折返：前半 [0, n-1] 遞增；後半鏡射遞減。m==n-1（頂點）回傳 n-1；
    // m==period 本會再回到 0，但 m 永遠到不了 period（恆 < period）
    return (m < n) ? static_cast<int>(m)
                   : static_cast<int>(period - m);
}

/**
 * @brief 取已載入 texture（`texW` x `texH` 像素）中、`frameCount` 影格 strip 的
 *        第 `index` 格來源子矩形（以 texel 為單位）。
 * @param index      影格索引；假定已落在 [0, frameCount)（由 FrameAt 保證）。
 * @param frameCount strip 影格數；<=0 時退化為整張 texture。
 * @param texW       texture 寬（像素）。
 * @param texH       texture 高（像素）。
 * @return 該影格的來源矩形。
 *
 * 影格在單列由左至右排列，故每格寬 texW/frameCount、滿高。
 */
[[nodiscard]] inline nccu::engine::math::Rect StripSourceRect(int index, int frameCount,
                                          int texW, int texH) noexcept {
    if (frameCount <= 0) {
        return nccu::engine::math::Rect{0.0f, 0.0f, static_cast<float>(texW),
                    static_cast<float>(texH)};
    }
    const float frameW = static_cast<float>(texW) /
                         static_cast<float>(frameCount);
    return nccu::engine::math::Rect{static_cast<float>(index) * frameW, 0.0f,
                frameW, static_cast<float>(texH)};
}

/**
 * @brief 一個已擺放的動畫裝飾——純資料（無 texture handle、無 raylib）。
 *
 * View 依索引把每個 def 與它載入的 texture 配對，並由 render clock 驅動動畫；
 * World 永遠看不到這些裝飾，故模擬時間軸／序列化輸出位元不變。影格「數」隨 def
 * 攜帶（最簡單而穩健的約定——不解析檔名、也無可能遺失的旁置檔）；影格「尺寸」於
 * 繪製時由載入 texture 的像素大小（texW/frameCount × texH）推得，故 strip 可
 * 任意解析度重新匯出而不需改動程式碼。
 */
struct DecorationDef {
    SemesterState chapter;     ///< 僅在 FSM 處於此狀態時繪製
    nccu::engine::math::Vec2          center;      ///< sprite 繪製所環繞的世界座標中心
    const char*   stripPath;   ///< PNG 路徑；缺檔則不畫
    int           frameCount;  ///< 水平 strip 的影格數（>=1）
    float         drawScale;   ///< 單一影格較長邊的螢幕尺寸（像素）
    double        fps;         ///< 乒乓前進速率
};

/**
 * @brief 給定載入 texture 的像素大小與 def 的 drawScale，算出環繞 `center`
 *        的裝飾螢幕目標矩形。
 * @param d    裝飾定義。
 * @param texW texture 寬（像素）。
 * @param texH texture 高（像素）。
 * @return 螢幕空間的目標矩形。
 *
 * 保留影格長寬比（frameW:texH），把「較長邊」縮放到 drawScale 像素；矩形以
 * `center` 為中心，使乒乓縮放圍繞錨點對稱脈動（雕像／貓在呼吸時不會漂移）。
 * 純函式、無 raylib、可單元測試。
 */
[[nodiscard]] inline nccu::engine::math::Rect DecorationDestRect(const DecorationDef& d,
                                             int texW, int texH) noexcept {
    const int n = d.frameCount > 0 ? d.frameCount : 1;
    const float frameW = static_cast<float>(texW) / static_cast<float>(n);
    const float frameH = static_cast<float>(texH);
    const float longer = frameW > frameH ? frameW : frameH;
    const float scale  = longer > 0.0f ? d.drawScale / longer : 0.0f;
    const float w = frameW * scale;
    const float h = frameH * scale;
    return nccu::engine::math::Rect{d.center.x - w * 0.5f, d.center.y - h * 0.5f, w, h};
}

} // namespace nccu::game::gfx

#endif // GFX_SPRITE_STRIP_H_
