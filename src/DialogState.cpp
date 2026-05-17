#include "DialogState.h"
#include <algorithm>

namespace nccu {

void DialogState::Open(std::vector<std::string> lines,
                       std::vector<DialogChoice> choices) {
    if (lines.empty()) { active_ = false; return; }  // no-op on empty
    lines_        = std::move(lines);
    choices_      = std::move(choices);
    cursor_       = 0;
    choiceCursor_ = 0;
    active_       = true;
}

const std::string& DialogState::CurrentLine() const {
    static const std::string kEmpty;
    if (!active_ || cursor_ >= lines_.size()) return kEmpty;
    return lines_[cursor_];
}

void DialogState::MoveChoice(int delta) noexcept {
    if (!AtChoice()) return;
    const int hi = static_cast<int>(choices_.size()) - 1;
    choiceCursor_ = std::clamp(choiceCursor_ + delta, 0, hi);
}

const DialogChoice* DialogState::Advance() {
    if (!active_) return nullptr;
    if (cursor_ < lines_.size()) {
        ++cursor_;
        if (cursor_ < lines_.size()) return nullptr;       // next line
        if (choices_.empty()) { Close(); return nullptr; } // end, no choice
        return nullptr;                                    // enter choice mode
    }
    // choice mode: confirm the highlighted choice. Copy it into picked_
    // so the returned pointer stays valid after we mutate/close (fixes a
    // latent use-after-free) and so a follow-up can play its lines.
    picked_ = choices_[static_cast<std::size_t>(choiceCursor_)];
    if (!picked_.nextLines.empty()) {
        lines_        = picked_.nextLines;   // play the (b)/(c) consequence
        choices_.clear();
        cursor_       = 0;
        choiceCursor_ = 0;
        // active_ stays true; AtChoice() now false; CurrentLine()==nextLines[0]
    } else {
        Close();                             // no follow-up: old behaviour
    }
    return &picked_;
}

void DialogState::Close() noexcept {
    active_       = false;
    lines_.clear();
    choices_.clear();
    cursor_       = 0;
    choiceCursor_ = 0;
    npcId_.clear();
}

} // namespace nccu
