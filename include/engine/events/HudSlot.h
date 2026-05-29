#ifndef HUD_SLOT_H_
#define HUD_SLOT_H_

namespace nccu {

/**
 * @brief HUD 訊息頻道：把章節／結局提示與一般訊息分到兩條獨立通道。
 *
 * 設兩條互不干擾的 HUD 訊息頻道，讓「章節通關提示」與一般 ShowMessage（抵達
 * 提示／karma 變動／拾取）能在同一幀並存而不互相覆蓋。先前 World 只有單一訊息
 * 槽，章節提示在每次切章時只活 0.02 秒（一幀）就被下一幀發布的抵達提示蓋掉。
 * 單純調整發布順序只能解掉部分競爭；把頻道一分為二才徹底解決抵達提示的覆蓋。
 *
 * - Top：章節／結局等重大進度提示。畫在底部訊息帶上方（約 25 px 間距），讓兩行
 *   皆清晰可讀。
 * - Bottom：其餘一切（拾取訊息、karma 增減、抵達提示、攤販購買文字、離場準備）。
 *   為 Event::slot 欄位的預設值，使既有發布者在 Bottom 頻道上行為不變；只有少數
 *   高優先提示（章節提示／結局關卡）改用 Top。
 *
 * 純資料——不含 raylib、不做渲染。獨立成小標頭，讓 World / Event / MessageView
 * / ChapterToast / EventWiring 可單獨引入，而不必拉進渲染層或 EventBus。
 */
enum class HudSlot {
    Top,      ///< 章節／結局等重大進度提示
    Bottom,   ///< 其餘一般訊息（預設頻道）
};

} // namespace nccu

#endif // HUD_SLOT_H_
