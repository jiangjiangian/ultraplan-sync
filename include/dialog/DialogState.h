#ifndef DIALOG_STATE_H_
#define DIALOG_STATE_H_
#include <cstddef>
#include <string>
#include <vector>

namespace nccu {

struct DialogChoice {
    std::string label;
    int         karmaDelta = 0;
    std::string setsFlag;          // "" = no flag
    bool        flagValue  = false;
    std::vector<std::string> nextLines{}; // consequence lines played on pick
};

// Pure-data conversation. Owned by World; View reads it const, the
// GameController steps it. No raylib. "Show then advance": Open() shows
// line 0; Advance() moves to the next line, and only past the last line
// does it close (or enter choice mode if choices were supplied).
class DialogState {
public:
    void Open(std::vector<std::string> lines,
              std::vector<DialogChoice> choices = {});

    [[nodiscard]] bool Active()   const noexcept { return active_; }
    [[nodiscard]] bool AtChoice() const noexcept {
        return active_ && cursor_ >= lines_.size() && !choices_.empty();
    }
    [[nodiscard]] const std::string& CurrentLine() const;

    // --- Pagination (presentation; pure data, no raylib) ----------------
    // The current line is wrapped to the dialog box (DialogLayout cell
    // width) and split into pages of kBoxRowsPerPage rows. The View draws
    // CurrentPageRows(); GameController's existing advance key steps a
    // page before it steps the line, so a long line never overflows or
    // clips — it paginates. CurrentLineHasMorePages() drives the "▼"
    // affordance.
    [[nodiscard]] std::vector<std::string> CurrentPageRows() const;
    [[nodiscard]] bool CurrentLineHasMorePages() const;
    // True when advancing will move WITHIN the same conversation (another
    // page of this line, or a further line) — i.e. the "▼ more" hint
    // applies. False on the last page of the last line (no choices).
    [[nodiscard]] bool HasMore() const;
    [[nodiscard]] const std::vector<DialogChoice>& Choices() const noexcept {
        return choices_;
    }
    [[nodiscard]] int ChoiceCursor() const noexcept { return choiceCursor_; }

    void MoveChoice(int delta) noexcept;

    // Optional npc attribution for the open conversation. OpenNpcDialog
    // tags it after Open() so a confirmed choice can be attributed back
    // to its NPC (the C.3 suit_senior one-shot guard reads this). ""
    // when none / after Close(). Vendor / test Open() paths leave it
    // empty, which is correct — they are not choice-opener NPCs.
    void SetNpcContext(std::string npcId) { npcId_ = std::move(npcId); }
    [[nodiscard]] const std::string& NpcId() const noexcept { return npcId_; }

    // Lines mode: if the current line has another page, turn the page
    // and stay on the line (return nullptr); otherwise step to the next
    // line (return nullptr); past last line with no choices -> Close
    // (nullptr); with choices -> enter choice mode (nullptr). Choice
    // mode: confirm the highlighted choice -> return a stable pointer to
    // it; if it carries nextLines, transition back to lines mode playing
    // those (stay active) instead of closing, else Close. Inactive:
    // nullptr.
    const DialogChoice* Advance();

    void Close() noexcept;

private:
    bool                      active_       = false;
    std::vector<std::string>  lines_;
    std::vector<DialogChoice> choices_;
    std::size_t               cursor_       = 0;
    std::size_t               pageCursor_   = 0;  // page within lines_[cursor_]
    int                       choiceCursor_ = 0;
    DialogChoice              picked_;      // stable storage so Advance()'s
                                            // return survives Close
    std::string               npcId_;       // attribution; cleared on Close()
};

} // namespace nccu
#endif // DIALOG_STATE_H_
