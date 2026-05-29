#include "game/dialog/DialogState.h"
#include "game/dialog/DialogLayout.h"
#include <algorithm>

namespace nccu {

void DialogState::Open(std::vector<std::string> lines,
                       std::vector<DialogChoice> choices) {
    if (lines.empty()) { active_ = false; return; }  // 空輸入為 no-op
    lines_        = std::move(lines);
    choices_      = std::move(choices);
    cursor_       = 0;
    pageCursor_   = 0;
    choiceCursor_ = 0;
    active_       = true;
}

const std::string& DialogState::CurrentLine() const {
    static const std::string kEmpty;
    if (!active_ || cursor_ >= lines_.size()) return kEmpty;
    return lines_[cursor_];
}

std::vector<std::string> DialogState::CurrentPageRows() const {
    if (!active_ || cursor_ >= lines_.size()) return {};
    const auto pages = dialog::LayoutPages(
        lines_[cursor_], dialog::kBoxCells, dialog::kBoxRowsPerPage);
    const std::size_t pc = std::min(pageCursor_, pages.size() - 1);
    return pages[pc];
}

bool DialogState::CurrentLineHasMorePages() const {
    if (!active_ || cursor_ >= lines_.size()) return false;
    const auto pages = dialog::LayoutPages(
        lines_[cursor_], dialog::kBoxCells, dialog::kBoxRowsPerPage);
    return pageCursor_ + 1 < pages.size();
}

bool DialogState::HasMore() const {
    if (!active_ || cursor_ >= lines_.size()) return false;
    if (CurrentLineHasMorePages()) return true;       // 本行還有頁
    if (cursor_ + 1 < lines_.size()) return true;     // 還有後續行
    return !choices_.empty();                         // 後面是選單
}

void DialogState::MoveChoice(int delta) noexcept {
    if (!AtChoice()) return;
    const int hi = static_cast<int>(choices_.size()) - 1;
    choiceCursor_ = std::clamp(choiceCursor_ + delta, 0, hi);
}

const DialogChoice* DialogState::Advance() {
    if (!active_) return nullptr;
    if (cursor_ < lines_.size()) {
        if (CurrentLineHasMorePages()) {  // 翻頁、停留本行
            ++pageCursor_;
            return nullptr;
        }
        ++cursor_;
        pageCursor_ = 0;
        if (cursor_ < lines_.size()) return nullptr;       // 下一行
        if (choices_.empty()) { Close(); return nullptr; } // 結束、無選項
        return nullptr;                                    // 進入選單模式
    }
    // 選單模式：確認高亮選項。複製到 picked_，使回傳指標在我們改動／關閉後仍有效
    // （修掉一個潛在的 use-after-free），並讓後續台詞能播放
    picked_ = choices_[static_cast<std::size_t>(choiceCursor_)];
    if (!picked_.nextLines.empty()) {
        lines_        = picked_.nextLines;   // 播放分支後果台詞
        choices_.clear();
        cursor_       = 0;
        pageCursor_   = 0;
        choiceCursor_ = 0;
        // active_ 維持 true；AtChoice() 此刻為 false；CurrentLine()==nextLines[0]
    } else {
        Close();                             // 無後續：沿用舊行為
    }
    return &picked_;
}

void DialogState::Close() noexcept {
    active_       = false;
    lines_.clear();
    choices_.clear();
    cursor_       = 0;
    pageCursor_   = 0;
    choiceCursor_ = 0;
    npcId_.clear();
}

} // namespace nccu
