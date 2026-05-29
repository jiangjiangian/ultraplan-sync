#ifndef DIALOG_LAYOUT_H_
#define DIALOG_LAYOUT_H_
#include <cstddef>
#include <string>
#include <vector>

/**
 * @file DialogLayout.h
 * @brief 對話框排版的純呈現工具（無 raylib、無輸入、無 World）：把一行對白依固定
 *        格寬斷行，再把斷出的列分頁，使對話框無論台詞多長都絕不溢出或被裁切。
 *
 * 格寬計算遵循 Unicode East Asian Width（全形／寬／模糊 = 2 格，其餘 = 1 格），因此
 * renderer 的斷行即為「是否塞得進框」的唯一真實來源——驗證工具只需斷言它不會產出比框
 * 更寬的列，而不必去檢查作者原始字串的長度。
 */
namespace nccu::dialog {

/**
 * @brief 計算字串 `s` 以全形格為單位的視覺寬度。
 * @param s 受測的 UTF-8 字串。
 * @return 視覺格數：ASCII／窄字 = 1，CJK 漢字／全形／模糊 = 2，組合附加符號 = 0。
 */
[[nodiscard]] int CellWidth(const std::string& s);

/**
 * @brief 把一段邏輯行貪婪斷行為每列至多 `maxCells` 格。
 * @param s        待斷行的單行字串。
 * @param maxCells 每列的視覺格數上限。
 * @return 斷出的列向量；至少一列（空輸入則為單一空列）。
 *
 * 若該段含 ASCII 空白則在空白處斷（word wrap）；否則在 UTF-8 字元邊界斷（CJK 無空白）
 * ——絕不切在多位元組序列中間，也絕不產出比 `maxCells` 寬的列（單一字元若本身超寬則
 * 獨佔一列）。字面 '\n' 強制硬斷。純函式，可直接單元測試。
 */
[[nodiscard]] std::vector<std::string> WrapToCells(const std::string& s,
                                                   int maxCells);

/**
 * @brief 把 `rows` 分頁為每頁至多 `rowsPerPage` 列。
 * @return 頁向量；永遠至少回傳一頁（可能為空），呼叫端才能安全索引 [0]。
 */
[[nodiscard]] std::vector<std::vector<std::string>>
Paginate(const std::vector<std::string>& rows, int rowsPerPage);

/** @brief 便利函式：一次完成斷行再分頁。 */
[[nodiscard]] std::vector<std::vector<std::string>>
LayoutPages(const std::string& s, int maxCells, int rowsPerPage);

// 對話框幾何尺寸集中於此一處，讓 View、DialogState 分頁與回歸測試三方一致；數值依
// 既有面板 Rect{20,320,760,110}、字級 16 選定。
//
// kBoxCells = 80：斷出的列應「填滿」到接近框右緣才換行，而非到三分之一處就停（過去設
// 28 會在 ~720px 寬的文字區只排到 ~224px 就斷）。文字自 kBoxTextX（36px）起排，「▼ 更
// 多」提示畫在 kBoxX+kBoxW-24 == 756px。尺寸由實際繪製步進反推：字級 16 的全形字元前進
// 約 size + size/10 ≈ 17.6px，即每格約 8.8px（取 EndingView::CenteredX 所用 ~8px/格
// 模型的保守端）——滿 80 格的 CJK 列止於 36 + 80*8.8 ≈ 740px，距 756px 的 ▼ 約 16px，
// 且落在 768px 內右緣之內。此值依格寬模型加安全邊際估出（無字型圖集時無法以像素量測），
// 而非螢幕實測。
inline constexpr int   kBoxCells       = 80;
inline constexpr int   kBoxRowsPerPage = 3;
inline constexpr float kBoxX           = 20.0f;
inline constexpr float kBoxY           = 320.0f;
inline constexpr float kBoxW           = 760.0f;
inline constexpr float kBoxH           = 110.0f;
inline constexpr float kBoxTextX       = 36.0f;
inline constexpr float kBoxTextY       = 336.0f;
inline constexpr float kBoxLineH       = 22.0f;
inline constexpr int   kBoxFontSize    = 16;

} // namespace nccu::dialog
#endif // DIALOG_LAYOUT_H_
