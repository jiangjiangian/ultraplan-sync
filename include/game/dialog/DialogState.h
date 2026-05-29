#ifndef DIALOG_STATE_H_
#define DIALOG_STATE_H_
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {

/// @brief 一個對話選項：選單中可選的分支，含其後果與一次性副作用。
struct DialogChoice {
    std::string label;                    ///< 選單標籤
    int         karmaDelta = 0;           ///< 確認時施加的業力增減
    std::string setsFlag;                 ///< 要設定的旗標名；"" 表示無
    bool        flagValue  = false;       ///< setsFlag 要設定的布林值
    std::vector<std::string> nextLines{}; ///< 選取後播放的後果台詞
};

// 「能退出的選項」：給「會改變狀態」的選單用、無副作用的收尾標籤，對應攤販的
// 「先不買，謝謝」婉拒（GameController kVendorDeclineLabel）。任何會提交（確認時改動
// karma／flag）的選單都追加一個帶此標籤、karmaDelta 為 0、setsFlag 為空的 DialogChoice，
// 使選它即「無任何狀態變動」地結束對話，玩家稍後可再回來做決定。確認處（GameController）
// 以標籤辨識它，從而略過該選單針對該 NPC 的選後記帳（例如 Ch4 助教的
// Flag_TaFinaleChoiceMade 自鎖）——此選項自身的空 payload 已使 ApplyDialogChoice 成為
// no-op。追加在「最後」，使既有選項索引維持穩定（與攤販婉拒尾項同契約）。
inline constexpr const char* kDialogExitLabel = "我再想想…";

// 純資料的對話會話。由 World 持有；View 以 const 讀取，GameController 推進它。無 raylib。
// 「先顯示、再前進」：Open() 顯示第 0 行；Advance() 移到下一行，唯有越過最後一行才關閉
// （若有提供選項則進入選單模式）。
class DialogState {
public:
    void Open(std::vector<std::string> lines,
              std::vector<DialogChoice> choices = {});

    [[nodiscard]] bool Active()   const noexcept { return active_; }
    [[nodiscard]] bool AtChoice() const noexcept {
        return active_ && cursor_ >= lines_.size() && !choices_.empty();
    }
    [[nodiscard]] const std::string& CurrentLine() const;

    /// @name 分頁（呈現；純資料、無 raylib）
    /// 當前行依對話框（DialogLayout 格寬）斷行，再切成每頁 kBoxRowsPerPage 列。View
    /// 繪製 CurrentPageRows()；GameController 既有的前進鍵會先翻頁、再進到下一行，故
    /// 不論行多長都不會溢出或被裁切——而是分頁。CurrentLineHasMorePages() 驅動「▼」提示。
    ///@{
    [[nodiscard]] std::vector<std::string> CurrentPageRows() const;
    [[nodiscard]] bool CurrentLineHasMorePages() const;
    /**
     * @brief 前進是否會停留在「同一段對話內」（本行還有頁，或還有後續行）。
     * @return 適用「▼ 更多」提示時為 true；停在最後一行最後一頁且無選項時為 false。
     */
    [[nodiscard]] bool HasMore() const;
    ///@}
    /// @brief 取選項清單。
    [[nodiscard]] const std::vector<DialogChoice>& Choices() const noexcept {
        return choices_;
    }
    /// @brief 取目前高亮的選項索引。
    [[nodiscard]] int ChoiceCursor() const noexcept { return choiceCursor_; }

    /// @brief 在選單模式中移動高亮游標（夾在合法範圍內）。
    void MoveChoice(int delta) noexcept;

    /**
     * @brief 為當前對話標註所屬 NPC（選用）。
     * @param npcId NPC 識別字串。
     *
     * OpenNpcDialog 於 Open() 後標註，使確認的選項能歸因回其 NPC（西裝學長一次性
     * 防護會讀此值）。無／Close() 後為 ""。攤販／測試的 Open() 路徑留空，這是正確的
     * ——它們並非選單開場 NPC。
     */
    void SetNpcContext(std::string npcId) { npcId_ = std::move(npcId); }
    /// @brief 取當前對話歸屬的 NPC 識別字串（無時為 ""）。
    [[nodiscard]] const std::string& NpcId() const noexcept { return npcId_; }

    /**
     * @brief 推進對話一步。
     * @return 在選單模式確認選項時，回傳指向該選項的穩定指標；其餘情況皆回傳 nullptr。
     *
     * 台詞模式：本行還有頁則翻頁、停留本行（回 nullptr）；否則進到下一行（nullptr）；
     * 越過最後一行且無選項 → Close（nullptr）；有選項 → 進入選單模式（nullptr）。
     * 選單模式：確認高亮選項 → 回傳指向它的穩定指標；若它帶 nextLines 則轉回台詞模式
     * 播放之（維持作用中）而非關閉，否則 Close。非作用中：nullptr。
     */
    const DialogChoice* Advance();

    /// @brief 結束對話並重設所有游標與歸屬。
    void Close() noexcept;

private:
    bool                      active_       = false;
    std::vector<std::string>  lines_;
    std::vector<DialogChoice> choices_;
    std::size_t               cursor_       = 0;
    std::size_t               pageCursor_   = 0;  ///< lines_[cursor_] 內的頁索引
    int                       choiceCursor_ = 0;
    DialogChoice              picked_;      ///< 穩定儲存，使 Advance() 的回傳指標在 Close 後仍有效
    std::string               npcId_;       ///< NPC 歸屬；Close() 時清空
};

} // namespace nccu
#endif // DIALOG_STATE_H_
