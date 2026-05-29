#ifndef CHAPTER_CARD_H_
#define CHAPTER_CARD_H_
#include "game/state/SemesterState.h"
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
namespace engine::render { class IRenderer; }

/**
 * @file ChapterCard.h
 * @brief 章節「書檔」大字卡：一段大、置中、短促的全幅注意節拍，與小而短暫的
 *        HUD toast（MessageView）有別。
 *
 * 兩種樣式：
 *   Lost  「傘又掉了」 — 每章「開始」時顯示（第一章用切題的開場變體「傘，
 *                       不見了」），即引子節拍。
 *   Found 「找到傘了」 — 每章「結束」時、在該章收尾旁白與奪回節拍「之後」顯示
 *                       （它借用了延後轉場的空檔：章節要等其收尾對話關閉後才清
 *                       關，故字卡剛好嵌在 FSM 由 Chapter* 跳往 Interlude_Market
 *                       的那一刻）。
 *
 * 第四章以結局畫面（EndingView）收尾，故那裡不會觸發 Found 字卡。
 *
 * 設計：完全由 View 本就每幀讀取的 FSM 在渲染側驅動（View 比對
 * world.Semester().Current() 與它記住的前一狀態）。不新增任何 EventType、不
 * 發布、不變更模型：字卡是純 View 裝飾，與 endingAlpha_／decorationClock_ 同
 * 性質，故自動跑流程的存檔逐位元不變。決定性來自 Time::DeltaSeconds()（自動跑
 * 流程下的固定 1/60 步長），因此腳本化重播每次都會在相同幀渲染字卡。
 */

enum class ChapterCardKind { None, Lost, Found };

/**
 * @brief 判斷 SemesterState 由 `from` 轉往 `to` 會觸發哪一種書檔字卡（若有）。
 * @return 對應的字卡種類。
 *
 * 純分類：
 *   to 為某 Chapter*                     -> Lost  （某章正開始）
 *   from 為 Chapter1/2/3 且 to==Interlude -> Found （某章剛清關）
 *   任何牽涉 Ending_* 狀態的轉場          -> None  （由 EndingView 負責）
 *   to == Interlude 但來自非章節          -> None
 */
[[nodiscard]] ChapterCardKind ChapterCardForTransition(
    SemesterState from, SemesterState to) noexcept;

/**
 * @brief 取得字卡的大標題。
 * @return Lost 時依 `to` 而定（第一章為開場「傘，不見了」，第二～四章為
 *         「傘又掉了」）；Found 恆為「找到傘了」；None 為空字串。
 *
 * 每個字形皆已烘入字型圖集（glyph-scan 測試涵蓋 ChapterCardStrings()），故不
 * 會出現缺字方塊。
 */
[[nodiscard]] std::string_view ChapterCardHeadline(
    ChapterCardKind kind, SemesterState to) noexcept;

/**
 * @brief 標題下方的一行小副標題。
 * @return 由目的狀態組出：Lost 為章節名（讓玩家知道哪一章開始了）；Found 為
 *         一句短結語。
 */
[[nodiscard]] std::string ChapterCardSubtitle(
    ChapterCardKind kind, SemesterState to);

/**
 * @brief 列舉字卡可能繪製的每一個字面字串，供字型 glyph-scan 測試使用。
 * @return 所有標題／副標題字串。
 *
 * 純資料：使任何未烘入圖集的標題／副標題字形在建置的覆蓋率檢查中失敗，而非靜默
 * 變成缺字方塊。
 */
[[nodiscard]] std::vector<std::string> ChapterCardStrings();

/**
 * @brief 單張字卡的渲染側、具決定性的計時狀態機。
 *
 * 持有目前種類、標題／副標題文字，以及一個經過秒數的時鐘。View 持有「一個」此
 * 物件。Trigger() 在章節邊界轉場時武裝它；Step() 每繪製幀推進淡入／持留／淡出；
 * Dismiss() 讓按鍵把它提早切斷。不碰 raylib、不碰 GL——淡入淡出純為浮點運算
 * （與 endingAlpha_ 同），故可單元測試。
 */
class ChapterCardState {
public:
    /// 總顯示時間與對稱的淡入淡出窗（秒）。持留 = kTotal - 2*kFade。約 2.2 秒的
    /// 總時長讀來是個短促節拍，又不致拖住玩家（底下的世界持續模擬）。
    static constexpr float kFade  = 0.3f;
    static constexpr float kTotal = 2.2f;

    [[nodiscard]] bool            Active()   const noexcept { return kind_ != ChapterCardKind::None; }
    [[nodiscard]] ChapterCardKind Kind()     const noexcept { return kind_; }
    [[nodiscard]] std::string_view Headline() const noexcept { return headline_; }
    [[nodiscard]] const std::string& Subtitle() const noexcept { return subtitle_; }
    [[nodiscard]] float           Elapsed()  const noexcept { return elapsed_; }

    /**
     * @brief 取得 [0,1] 的淡入淡出強度。
     * @param reducedMotion 開啟減少動畫時，全程不透明（無亮度漸變），到 kTotal
     *                      硬切。
     * @return 預設在 kFade 內漸升、持留為 1、最後 kFade 內漸降。
     */
    [[nodiscard]] float Alpha(bool reducedMotion = false) const noexcept;

    /**
     * @brief 為一段新的書檔節拍武裝字卡（重置時鐘）。
     *
     * 種類為 None 時清除字卡（用於某轉場不觸發任何字卡的情形）。
     */
    void Trigger(ChapterCardKind kind, std::string_view headline,
                 std::string subtitle) noexcept;

    /**
     * @brief 以 `dt` 推進時鐘；一旦超過 kTotal 即自動清除（->None）。
     *
     * 未啟用時為冪等的空操作。具決定性：自動跑流程下 dt 為固定步長。
     */
    void Step(float dt) noexcept;

    /// @brief 玩家按鍵略過：立即清除。
    void Dismiss() noexcept;

private:
    ChapterCardKind kind_     = ChapterCardKind::None;
    std::string     headline_;
    std::string     subtitle_;
    float           elapsed_  = 0.0f;
};

/**
 * @brief 依字卡狀態繪製大字卡（未啟用時為空操作）。
 * @param reducedMotion 轉交給 Alpha()。
 *
 * 一層變暗的全螢幕背景、置中的粗體標題＋副標題，以及一個大型雨傘字形提示
 * （Lost -> 破傘傘骨，「傘又掉了」；Found -> 真傘藍傘面，「你把它找回來了」），
 * 整張字卡隨 Alpha() 淡入淡出。自我完備且可由 spy 測試（與 DrawEndingCard／
 * DrawHudMessage 同）。
 */
void DrawChapterCard(nccu::engine::render::IRenderer& r, const ChapterCardState& card,
                     float screenW, float screenH,
                     bool reducedMotion = false);

} // namespace nccu
#endif // CHAPTER_CARD_H_
