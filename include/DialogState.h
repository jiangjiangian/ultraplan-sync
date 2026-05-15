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
    [[nodiscard]] const std::vector<DialogChoice>& Choices() const noexcept {
        return choices_;
    }
    [[nodiscard]] int ChoiceCursor() const noexcept { return choiceCursor_; }

    void MoveChoice(int delta) noexcept;

    // Lines mode: step to next line (return nullptr); past last line with
    // no choices -> Close (nullptr); with choices -> enter choice mode
    // (nullptr). Choice mode: confirm the highlighted choice -> return it
    // and Close. Inactive: nullptr.
    const DialogChoice* Advance();

    void Close() noexcept;

private:
    bool                      active_       = false;
    std::vector<std::string>  lines_;
    std::vector<DialogChoice> choices_;
    std::size_t               cursor_       = 0;
    int                       choiceCursor_ = 0;
};

} // namespace nccu
#endif // DIALOG_STATE_H_
