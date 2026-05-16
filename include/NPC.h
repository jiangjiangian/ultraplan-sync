#ifndef N_P_C_H_
#define N_P_C_H_
#include "Character.h"
#include "CollisionMask.h"
#include "SemesterState.h"
#include "gfx/Texture.h"
#include "gfx/Rect.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class NPC : public Character {
public:
    NPC(nccu::gfx::Vec2 position,
        std::vector<std::string> dialogLines,
        bool isQuestGiver = false,
        std::string_view npcId = {});

    void Update(float deltaTime) override;
    void Render(nccu::gfx::IRenderer& renderer) const override;
    void Interact(Player* initiator) override;

    // Dialog lookup key — GameController builds the per-(npcId,
    // SemesterState) opener from this. "" for Vendor / ambient students
    // (they fall back to Interact()). Overrides GameObject::NpcId so the
    // dispatch stays virtual-not-dynamic_cast.
    [[nodiscard]] std::string_view NpcId() const noexcept override;

    // Stationary archetype NPCs are solid walls the player bumps off;
    // ambient wandering students are decorative and must NOT block the
    // player (a crowd you can't push past is infuriating).
    [[nodiscard]] bool BlocksMovement() const noexcept override { return !wander_; }

    // Turns this NPC into an ambient pedestrian: random-walk at `speed`
    // px/s, re-picking a heading every 1-3 s. `seed` keys the per-NPC
    // PRNG so a crowd does not march in lock-step. Chainable.
    NPC& EnableWander(float speed, unsigned seed) noexcept;

    // Wandering NPCs self-resolve against the world's terrain mask so
    // they don't walk through buildings, the river or painted props. The
    // reference must outlive the NPC (World owns both; fixed after ctor).
    void SetWanderMask(const nccu::CollisionMask& mask) noexcept {
        wanderMask_ = &mask;
    }

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

    // Replace dialog from the runtime chapter content for the given
    // (npcId, state, subState). No match -> dialog cleared. Chainable.
    NPC& LoadDialog(std::string_view npcId, nccu::SemesterState state,
                    int subState = 0);

    [[nodiscard]] const std::vector<std::string>*
        DialogLines() const noexcept override { return &dialogLines_; }

private:
    std::vector<std::string> dialogLines_;
    size_t                   currentLineIndex_;
    bool                     isQuestGiver_;
    std::string              npcId_;

    std::optional<nccu::gfx::Texture> sprite_;

    bool                                       wander_;
    float                                      retargetTimer_;
    nccu::gfx::Vec2                             wanderDir_;
    std::uint32_t                              rng_;
    const nccu::CollisionMask*                 wanderMask_;
};

#endif // N_P_C_H_
