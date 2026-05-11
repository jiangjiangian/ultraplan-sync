#pragma once
#include "Character.h"
#include <string>
#include <vector>

class NPC : public Character {
public:
    NPC(nccu::gfx::Vec2 position,
        std::vector<std::string> dialogLines,
        bool isQuestGiver = false);

    void Update(float deltaTime) override;
    void Draw() const override;
    void Interact(Player* initiator) override;

    bool   IsQuestGiver()       const { return isQuestGiver_; }
    size_t CurrentLineIndex()   const { return currentLineIndex_; }
    size_t DialogLineCount()    const { return dialogLines_.size(); }
    const std::string& CurrentLineText() const;

    // Replace the dialog lines (used when chapter state changes; the
    // state machine pushes new lines into the NPC). Resets currentLineIndex_.
    void SetDialogLines(std::vector<std::string> lines);

private:
    std::vector<std::string> dialogLines_;
    size_t                   currentLineIndex_;
    bool                     isQuestGiver_;
};
