#ifndef GFX_UMBRELLA_GLYPH_H_
#define GFX_UMBRELLA_GLYPH_H_
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

/**
 * @file UmbrellaGlyph.h
 * @brief 所有雨傘外觀繪製方式的唯一真實來源——純矩形向量字符（rect-only glyph）。
 *
 * 同一個 helper 同時用於「世界中」（TransparentUmbrella::Render）、「地面拾取物」
 * （QuestFlagPickup::Render——Ch1 苦主的透明傘），以及「結局畫面」（EndingView 依
 * 結局選定），使同一把傘在任何出現處看起來都一致。無 raylib、無 GL：每個像素都經
 * 注入的 IRenderer::DrawRect 繪製，與其餘 Item 繪製路徑相同（Item 不得呼叫
 * DrawText／DrawTexture——架構紅線）。resources/ 可能為空，故以形狀繪製的傘在有無
 * 資產時都畫得一樣。
 *
 * 四把劇情傘 + ProfessorTrap 變體：
 * - TrueBlue        真傘   — 藍色，寬而圓潤的傘面（乾淨／正確）。
 * - FragileBroken   破傘   — 只剩手柄 + 裸露彎折的傘骨（傘面已失）：即「易碎／壞掉」感。
 * - CursedPurple    詛咒傘 — 暗紫，下垂沉重的傘面 + 黑色手柄。
 * - UglyGreen       醜傘   — 螢光綠，凹凸刺眼的傘面（絕不會認錯）。
 * - ProfessorTrap   陷阱傘 — 警示紅，稜角帶尖刺的傘面（陷阱）。非四把劇情傘之一，刻意
 *                            給予獨特的危險紅外觀，使其永不與真傘（藍）或醜傘（綠）混淆。
 */
namespace nccu::game::gfx {

/// @brief 雨傘外觀列舉；驅動 UmbrellaLookColor 與 DrawUmbrellaGlyph 的繪製分支。
enum class UmbrellaLook {
    TrueBlue,       ///< 真傘 — 藍
    FragileBroken,  ///< 破傘 — 只剩手柄／傘骨
    CursedPurple,   ///< 詛咒傘 — 暗紫
    UglyGreen,      ///< 醜傘 — 螢光綠
    ProfessorTrap   ///< 陷阱傘 — 危險紅（非四把劇情傘之一）
};

/**
 * @brief 取每種外觀的標誌色——世界中的傘、拾取物與結局卡共用的同一個值。
 * @param look 雨傘外觀。
 * @return 對應的標誌色。
 *
 * 公開此函式，讓呼叫端（例如 HUD 色塊）不必自行重推就能與 glyph 配色一致。
 */
[[nodiscard]] constexpr nccu::engine::math::Color UmbrellaLookColor(UmbrellaLook look) noexcept {
    switch (look) {
        case UmbrellaLook::TrueBlue:      return nccu::engine::math::Color{ 40, 120, 235, 255};
        case UmbrellaLook::FragileBroken: return nccu::engine::math::Color{170, 170, 175, 255};
        case UmbrellaLook::CursedPurple:  return nccu::engine::math::Color{ 90,  40, 120, 255};
        case UmbrellaLook::UglyGreen:     return nccu::engine::math::Color{120, 255,  40, 255};
        case UmbrellaLook::ProfessorTrap: return nccu::engine::math::Color{235,  70,  40, 255};
    }
    return nccu::engine::math::Color{255, 255, 255, 255};
}

/**
 * @brief 繪製 `look` 的雨傘 glyph，縮放填滿 `bounds`。
 * @param r      注入的渲染器。
 * @param look   雨傘外觀。
 * @param bounds 目標矩形（glyph 填滿之）。
 * @param alpha  整體不透明度（0..255）；預設 255 全不透明。
 *
 * 所有幾何皆以方框比例表示，故同一輪廓在 20x20 世界佔地、16x16 地面拾取物與大張
 * 結局卡上都讀得出來。純呈現、僅依傳入列舉，無模擬／狀態（MVC 乾淨）。
 *
 * `alpha` 等比縮放「每一個」矩形的不透明度，讓淡入的表面（結局卡以自身 alpha 淡入）
 * 能以對應強度繪製 glyph。世界中／拾取物呼叫端從不淡入，預設全不透明，故其行為位元
 * 不變。
 */
inline void DrawUmbrellaGlyph(nccu::engine::render::IRenderer& r, UmbrellaLook look, nccu::engine::math::Rect bounds,
                              unsigned char alpha = 255) {
    namespace C = nccu::engine::math::Colors;
    const float x = bounds.x;
    const float y = bounds.y;
    const float w = bounds.width;
    const float h = bounds.height;
    // 先以 glyph alpha 等比縮放顏色的 alpha（255 ⇒ 不變）
    auto fade = [alpha](nccu::engine::math::Color col) {
        return alpha == 255 ? col
            : col.WithAlpha(static_cast<unsigned char>(
                  static_cast<int>(col.a) * static_cast<int>(alpha) / 255));
    };
    // rc(fx,fy,fw,fh)：以方框比例定位／定尺寸並經 alpha 縮放的矩形
    auto rc = [&](float fx, float fy, float fw, float fh, nccu::engine::math::Color col) {
        r.DrawRect(nccu::engine::math::Rect{x + fx * w, y + fy * h, fw * w, fh * h}, fade(col));
    };
    const nccu::engine::math::Color t = UmbrellaLookColor(look);

    switch (look) {
        case UmbrellaLook::TrueBlue: {
            // 寬、滿、圓潤的三層傘面（「完整／正確」感）覆於淺色手柄上，透明傘看來完好
            rc(0.30f, 0.10f, 0.40f, 0.10f, t);
            rc(0.15f, 0.20f, 0.70f, 0.15f, t);
            rc(0.00f, 0.35f, 1.00f, 0.15f, t);
            rc(0.45f, 0.50f, 0.10f, 0.40f, C::DarkGray);   // 傘桿
            rc(0.35f, 0.88f, 0.30f, 0.10f, C::DarkGray);   // 鉤狀傘腳
            break;
        }
        case UmbrellaLook::FragileBroken: {
            // 只剩手柄與幾根裸露彎折的傘骨——傘面已沒了，讀作毫無疑問的「壞掉／剩手柄」。
            // 上方傘骨樞紐、三根外張的斷骨，再加彎折的傘桿 + 傘鉤
            rc(0.43f, 0.16f, 0.14f, 0.10f, t);             // 傘骨樞紐（頂）
            rc(0.14f, 0.20f, 0.26f, 0.05f, t);             // 左骨（折斷、下垂）
            rc(0.10f, 0.25f, 0.06f, 0.10f, t);             // 左骨末端下垂
            rc(0.60f, 0.20f, 0.24f, 0.05f, t);             // 右骨（折斷）
            rc(0.84f, 0.25f, 0.06f, 0.08f, t);             // 右骨末端
            rc(0.46f, 0.26f, 0.08f, 0.52f, C::DarkGray);   // 傘桿
            rc(0.40f, 0.78f, 0.22f, 0.08f, C::DarkGray);   // 彎折的鉤狀傘腳
            break;
        }
        case UmbrellaLook::CursedPurple: {
            // 下垂沉重的傘面、兩側傘骨向下垂，覆於純黑手柄上——壓抑的「這不對勁」感
            rc(0.35f, 0.14f, 0.30f, 0.10f, t);
            rc(0.15f, 0.24f, 0.70f, 0.14f, t);
            rc(0.05f, 0.38f, 0.90f, 0.10f, t);
            rc(0.05f, 0.48f, 0.15f, 0.20f, t);             // 左骨向下垂
            rc(0.80f, 0.48f, 0.15f, 0.20f, t);             // 右骨向下垂
            rc(0.45f, 0.48f, 0.10f, 0.46f, C::Black);      // 黑色傘桿
            break;
        }
        case UmbrellaLook::UglyGreen: {
            // 凹凸、刺眼、不對稱的傘面，配尖叫般的螢光綠——「醜得全校最好認」（醜到沒人會誤拿）
            rc(0.25f, 0.12f, 0.55f, 0.12f, t);             // 偏心的頂部團塊
            rc(0.10f, 0.24f, 0.85f, 0.16f, t);             // 鼓起的中段（偏右）
            rc(0.05f, 0.40f, 0.80f, 0.12f, t);             // 歪斜的傘緣
            rc(0.85f, 0.36f, 0.12f, 0.10f, t);             // 一個多餘的醜凸起
            rc(0.42f, 0.52f, 0.10f, 0.40f, C::DarkGray);   // 傘桿
            rc(0.32f, 0.90f, 0.30f, 0.08f, C::DarkGray);   // 鉤狀傘腳
            break;
        }
        case UmbrellaLook::ProfessorTrap: {
            // 稜角的階梯狀金字塔傘面，配尖頂 + 鋸齒傘緣倒刺：令人警覺、武器化的「這是陷阱」輪廓
            rc(0.43f, 0.04f, 0.10f, 0.14f, t);             // 尖刺頂端
            rc(0.33f, 0.18f, 0.34f, 0.10f, t);
            rc(0.18f, 0.28f, 0.64f, 0.10f, t);
            rc(0.04f, 0.38f, 0.92f, 0.10f, t);
            rc(0.08f, 0.48f, 0.10f, 0.10f, t);             // 傘緣倒刺
            rc(0.45f, 0.48f, 0.10f, 0.10f, t);
            rc(0.82f, 0.48f, 0.10f, 0.10f, t);
            rc(0.45f, 0.58f, 0.10f, 0.36f, C::DarkGray);   // 傘桿
            break;
        }
    }
}

} // namespace nccu::game::gfx

#endif // GFX_UMBRELLA_GLYPH_H_
