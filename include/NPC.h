#ifndef N_P_C_H_
#define N_P_C_H_
#include "Character.h"
#include "gfx/Texture.h"
#include <optional>
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

    [[nodiscard]] bool BlocksMovement() const noexcept override { return true; }

    // Loads a Pipoya 96x128 sheet. NPCs are stationary, so only the idle
    // column at row 0 (facing down) is ever drawn.
    void LoadSprite(const std::string& path);

    [[nodiscard]] bool   IsQuestGiver()     const noexcept { return isQuestGiver_; }
    [[nodiscard]] size_t CurrentLineIndex() const noexcept { return currentLineIndex_; }
    [[nodiscard]] size_t DialogLineCount()  const noexcept { return dialogLines_.size(); }
    [[nodiscard]] const std::string& CurrentLineText() const;

    // Replace the dialog lines (used when chapter state changes; the
    // state machine pushes new lines into the NPC). Resets currentLineIndex_.
    // Returns *this for chaining: npc.SetDialogLines({...}).LoadSprite(...).
    NPC& SetDialogLines(std::vector<std::string> lines);

private:
    std::vector<std::string> dialogLines_;
    size_t                   currentLineIndex_;
    bool                     isQuestGiver_;

    std::optional<nccu::gfx::Texture> sprite_;
};

#endif // N_P_C_H_
