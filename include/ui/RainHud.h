#ifndef RAIN_HUD_H_
#define RAIN_HUD_H_
#include <string_view>

namespace nccu {

/**
 * @brief 為雨量 HUD 讀數提供「色弱備援」的固定前綴字串。
 * @param rm 目前雨量計數值（0–100）。
 * @return 對應三段壓力等級的 2 字元前綴（見下表）。
 *
 * 顏色斜坡（白→金→紅）對紅綠色弱（deuteran／protan）玩家而言難以分辨，因此
 * 額外回傳一段純文字前綴，讓「風險上升」這個訊號不只靠顏色傳達：
 *   rm < 60       → "  "（平靜）
 *   60 ≤ rm < 85  → " !"（警告）
 *   rm ≥ 85       → "!!"（危急）
 * 純函式：不碰 raylib、不配置記憶體；回傳的 view 指向靜態字面值，整個程式
 * 生命週期內持有皆安全。View 會把它接在 "rain: NN%" 前面，使文字通道永不
 * 單靠顏色承載資訊。
 */
constexpr std::string_view RainTierPrefix(float rm) noexcept {
    if (rm >= 85.0f) return "!!";
    if (rm >= 60.0f) return " !";
    return "  ";
}

} // namespace nccu

#endif // RAIN_HUD_H_
