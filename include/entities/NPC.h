#ifndef N_P_C_H_
#define N_P_C_H_
#include "entities/Character.h"
#include "world/CollisionMask.h"
#include "state/SemesterState.h"
#include "engine/render/Texture.h"
#include "engine/math/Rect.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// ISP roles: all three (IUpdatable + IDrawable + IInteractable). Every body
// is real — Update drives wander / crowd-runner motion, Render draws the
// sprite, Interact cycles dialog lines. Vendor (the only NPC subclass)
// shares this exact role set (it only adds TryBuy + overrides IsVendor), so
// WithRoles is keyed on this intermediate (Derived = NPC); static_cast<NPC*>
// is valid for a Vendor too.
class NPC : public WithRoles<NPC, Character>,
            public IUpdatable, public IDrawable, public IInteractable {
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
    [[nodiscard]] bool BlocksMovement() const noexcept override {
        return !wander_ && !circular_;   // wanderers + crowd runners are decorative
    }

    // Turns this NPC into an ambient pedestrian: random-walk at `speed`
    // px/s, re-picking a heading every 1-3 s. `seed` keys the per-NPC
    // PRNG so a crowd does not march in lock-step. Chainable.
    NPC& EnableWander(float speed, unsigned seed) noexcept;

    // Decorative 校慶 crowd runner: circles `center` at `radius`,
    // advancing `angularSpeed` rad/s from `startAngle`. Walk-animated and
    // non-blocking (the player runs the same track). 5 runners at distinct
    // speeds populate the 操場 lap.
    NPC& EnableCircularRun(nccu::gfx::Vec2 center, float radius,
                           float angularSpeed, float startAngle) noexcept;

    // Wandering NPCs self-resolve against the world's terrain mask so
    // they don't walk through buildings, the river or painted props. The
    // reference must outlive the NPC (World owns both; fixed after ctor).
    void SetWanderMask(const nccu::CollisionMask& mask) noexcept {
        wanderMask_ = &mask;
    }

    // Loads a Pipoya 96x128 sheet (3 walk columns x 4 facing rows, the same
    // sheet the Player uses). A stationary archetype NPC only ever draws the
    // idle column at row 0 (facing down); a wandering / 校慶-runner NPC plays
    // the full 4-direction walk cycle from its heading (see Render).
    void LoadSprite(const std::string& path);

    // Render the WHOLE texture (not a 32×32 Pipoya cell) — for a vendor
    // whose art is a single static image, e.g. the 自動販賣機 machine.
    void SetStaticSprite(bool v) noexcept { staticSprite_ = v; }

    [[nodiscard]] bool   IsQuestGiver()     const noexcept override { return isQuestGiver_; }
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

    // Test inspection (U1-T3): the {column, row} of the 96x128 Pipoya sheet
    // Render() would blit THIS frame — the exact integration of the
    // animated-vs-idle decision with the shared gfx::WalkCycle maths.
    // Exposed because Render()'s textured blit is GL-gated (a headless test
    // has no valid Texture, so Render() early-returns the fallback rect and
    // never reaches the cell selection). Pure read of already-simulated
    // state; mirrors SceneRouter's LastRosterState() inspection accessor.
    struct RenderCell { int col; int row; };
    [[nodiscard]] RenderCell CurrentRenderCell() const noexcept;

private:
    std::vector<std::string> dialogLines_;
    size_t                   currentLineIndex_;
    bool                     isQuestGiver_;
    std::string              npcId_;

    std::optional<nccu::gfx::Texture> sprite_;
    bool                              staticSprite_ = false;

    bool                                       wander_;
    float                                      retargetTimer_;
    nccu::gfx::Vec2                             wanderDir_;
    std::uint32_t                              rng_;
    const nccu::CollisionMask*                 wanderMask_;

    // 校慶 crowd runner (EnableCircularRun): a fixed circular track + a
    // walk-frame animation so the runner reads as running, not sliding.
    bool                                       circular_ = false;
    nccu::gfx::Vec2                             circleCenter_{};
    float                                      circleRadius_ = 0.0f;
    float                                      circleAngle_  = 0.0f;
    float                                      circleSpeed_  = 0.0f;  // rad/s
    // Walk-cycle animation state, shared by the 校慶 runner AND the ambient
    // wanderer (U1-T3) so both read as walking, not sliding — exactly the
    // Player's idiom (Player.cpp): animStep_ cycles the 3 walk columns while
    // moving, holds the idle column at rest; facing_ keys the Pipoya row.
    // moving_ records whether the NPC actually displaced this frame (false
    // while paused or wall-blocked) so Render shows the idle pose at rest.
    // NONE of these reach state.jsonl (the harness dumps pos/flags/npcs-by-
    // id, never the anim frame) — pure render selection, MVC clean.
    float                                      animTimer_    = 0.0f;
    int                                        animStep_     = 0;
    bool                                       moving_       = false;
    nccu::gfx::Vec2                             facing_{0.0f, 1.0f};
};

#endif // N_P_C_H_
