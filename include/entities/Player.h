#ifndef PLAYER_H_
#define PLAYER_H_
#include "entities/Character.h"
#include "engine/math/Color.h"
#include "engine/render/Texture.h"
#include "engine/math/Vec2.h"
#include <optional>
#include <string>
#include <unordered_map>

// B2.1: the umbrella the player is CURRENTLY holding, as pure data on the
// Player — deliberately SEPARATE from the ending-determining flags
// (Flag_HasTrueUmbrella / Flag_TookCursedUmbrella / Flag_BoughtUglyUmbrella).
// Those flags persist for the whole run (EndingGate.cpp reads them to pick
// A/B/C, and Flag_TookCursedUmbrella / Flag_BoughtUglyUmbrella are NEVER
// cleared), so they cannot answer "which umbrella is in the bag right now":
// after Ch4 entry's SetHasUmbrella(false) the old flag-keyed bag still showed
// a cursed / ugly row that the player no longer holds. This enum tracks the
// live held umbrella so the bag reflects the umbrella you ACTUALLY hold (and
// vanishes the instant you lose it), while the ending flags stay untouched.
// None == empty-handed. Loaner is the Ch2 圖書館管理員 umbrella (B2.3): a
// functional shelter that is NOT the true umbrella (no Flag_HasTrueUmbrella).
enum class HeldUmbrella { None, True, Cursed, Ugly, Victim, Fragile,
                          ProfessorTrap, Loaner };

// ISP roles: IUpdatable + IDrawable + IMortal. The old Interact(Player*)
// body was an empty no-op ("Player does not respond to other Players in
// MVP"), so that role is dropped — nothing ever invoked it through the
// container (the E-interact sweep skips the player via ForEachActiveExcept).
// Update (input + animation) and Render (the sprite) are real and kept.
// IMortal is Assignment-#6 combat scaffolding (hp / TakeDamage / IsDead):
// the player has hit-points so the #6 survival mode can damage and kill it;
// hp_ is NOT serialized (the autoplay harness emits only x/y/karma/money/
// rain — see Harness.cpp), so adding it leaves state.jsonl byte-identical.
// Player is final, so WithRoles is keyed on Player itself.
class Player final : public WithRoles<Player, Character>,
                     public IUpdatable, public IDrawable, public IMortal {
public:
    explicit Player(nccu::gfx::Vec2 position);

    void Update(float deltaTime) override;
    void Render(nccu::gfx::IRenderer& renderer) const override;

    void HandleInput(float deltaTime);

    // ── IMortal (Assignment-#6 combat scaffolding) ──────────────────
    // Starting / max hit-points. A round 100 so #6 can tune damage in
    // percentage-like steps; not wired to any current gameplay (no enemy
    // deals damage yet), so the present game is unaffected.
    static constexpr int kMaxHp = 100;
    // Lower hp by `amount` (clamped at 0; a non-positive amount is ignored
    // — healing is a separate concern). noexcept for the combat hot loop.
    void TakeDamage(int amount) noexcept override {
        if (amount <= 0) return;
        hp_ = (amount >= hp_) ? 0 : hp_ - amount;
    }
    [[nodiscard]] bool IsDead() const noexcept override { return hp_ <= 0; }
    [[nodiscard]] int  Hp()     const noexcept override { return hp_; }

    // Mutators return *this so callers can chain:
    //   player.AddKarma(10).AddMoney(50).SetHasUmbrella(true);
    Player& AddKarma(int delta);            // clamped to [-100, 100]
    Player& decreaseKarma(int amount);      // thin wrapper, prefer AddKarma
    Player& resetRainMeter() noexcept;

    // P2 cursed-taint mechanic (replaces the prior one-shot -30 at pickup):
    // each CursedUmbrella pickup increments the taint counter, and every
    // chapter transition (Ch2/Ch3/Ch4 entry — the same hook that resets the
    // held umbrella) bleeds karma -= 5 * cursedTaint_. Two pickups → -10/
    // chapter; three → -15/chapter — so cursed-stacking now reads as a
    // sliding moral cost the player can FEEL across the run instead of a
    // single sticker shock. Never cleared (the "moral stain is permanent"
    // contract); silent under taint=0 so non-cursed runs are byte-identical.
    Player& IncCursedTaint() noexcept { ++cursedTaint_; return *this; }
    Player& ApplyCursedTaintDecay();
    [[nodiscard]] int GetCursedTaint() const noexcept { return cursedTaint_; }

    // SetHasUmbrella(false) is the canonical "the umbrella is gone" call
    // (Ch4 entry's 傘再度失蹤; the per-chapter「傘又掉了」reset). B2.1: losing
    // the umbrella must also empty the held-umbrella slot so the bag's
    // umbrella row disappears — otherwise a stale row lingers (the held kind
    // is the bag's source of truth, not the persistent ending flags). Setting
    // it true here does NOT pick a kind (an explicit SetHeldUmbrella does);
    // this overload keeps the legacy bool contract for callers that only
    // toggle shelter (tests, the rain loop).
    Player& SetHasUmbrella(bool v) noexcept {
        hasUmbrella_ = v;
        if (!v) heldUmbrella_ = HeldUmbrella::None;
        return *this;
    }
    // B2.1/B2.3: set the CURRENTLY held umbrella kind. A non-None kind
    // implies the player is sheltered (hasUmbrella_ = true) so the auto-rain
    // drain (ApplyRainSheltered) kicks in; None clears the shelter too. This
    // is the ONE place the bag's umbrella row is derived from — it never
    // touches the ending flags (callers set those separately when an outcome
    // is actually decided). Idempotent by construction (re-setting the same
    // kind is a no-op assignment), so a re-claim / re-talk never stacks.
    Player& SetHeldUmbrella(HeldUmbrella kind) noexcept {
        heldUmbrella_ = kind;
        hasUmbrella_  = (kind != HeldUmbrella::None);
        return *this;
    }
    [[nodiscard]] HeldUmbrella HeldUmbrellaKind() const noexcept {
        return heldUmbrella_;
    }
    // Money has a soft cap (S5b-4 loop economy): with 3 earn->spend
    // cycles, an uncapped purse would let a thorough explorer trivialise
    // every market. 300 is the SCRIPT_HANDOFF §五.1 999 retuned for the
    // 3-cycle structure. DeductMoney is unaffected; only the ceiling is
    // clamped, never the floor (a negative `amount` still subtracts).
    static constexpr int kMoneySoftCap = 300;
    Player& AddMoney(int amount) noexcept {
        money_ += amount;
        if (money_ > kMoneySoftCap) money_ = kMoneySoftCap;
        return *this;
    }
    Player& SetFlag(const std::string& name)   { flags_[name] = true; return *this; }
    Player& ClearFlag(const std::string& name) { flags_.erase(name);  return *this; }

    // Count-only consumable inventory (S5b-3): the player holds a tally
    // per itemId, not GameObject instances — deliberately minimal ("中庸"
    // per the plan: no duration/quantity GameObjects; the Tab UI in
    // S5b-5 just renders these counts). Vendor::TryBuy adds; a chapter
    // that "uses up" a consumable calls ConsumeOne; S5b-4 clears the map
    // on chapter change (consumables are spent within their chapter).
    Player& AddConsumable(const std::string& itemId) {
        ++consumables_[itemId];
        return *this;
    }
    [[nodiscard]] int ConsumableCount(const std::string& itemId) const {
        auto it = consumables_.find(itemId);
        return it == consumables_.end() ? 0 : it->second;
    }
    // Spends one if present (returns true); a no-op false if none held.
    bool ConsumeOne(const std::string& itemId) {
        auto it = consumables_.find(itemId);
        if (it == consumables_.end() || it->second <= 0) return false;
        if (--it->second == 0) consumables_.erase(it);
        return true;
    }
    // S5b-4 "消耗品當章用完": GameController wipes the inventory when the
    // player re-enters the market, so consumables bought for one chapter
    // can't be hoarded across the market boundary into the next.
    Player& ClearConsumables() noexcept {
        consumables_.clear();
        return *this;
    }
    // Whole-map view for the Tab inventory UI (S5b-5) — read-only so the
    // reactive InventoryView can render every held line without the
    // Player exposing its storage for mutation.
    [[nodiscard]] const std::unordered_map<std::string, int>&
    Consumables() const noexcept { return consumables_; }

    // Loads a Pipoya 96x128 sprite sheet (3 walk frames x 4 directions of
    // 32x32 each). Replaces any previously loaded sheet.
    void LoadSprite(const std::string& path);

    // Draw-time colour modulate for the chosen persona (character-select
    // feature). Pure data: Render() passes it to IRenderer::DrawSprite so
    // five personas sharing a base Pipoya sheet still read as distinct
    // without committing any new sprite binary. White = no recolour
    // (the default, also what the autoplay harness uses). Not gameplay
    // state — purely cosmetic, reset on a fresh World like everything
    // else (Restart builds a new Player).
    Player& SetTint(nccu::gfx::Color t) noexcept { tint_ = t; return *this; }
    [[nodiscard]] nccu::gfx::Color GetTint() const noexcept { return tint_; }

    [[nodiscard]] int   GetKarma()      const noexcept { return karma_; }
    [[nodiscard]] float GetRainMeter()  const noexcept { return rainMeter_; }
    [[nodiscard]] bool  HasUmbrella()   const noexcept { return hasUmbrella_; }
    [[nodiscard]] int   GetMoney()      const noexcept { return money_; }
    [[nodiscard]] bool  HasFlag(const std::string& name) const {
        auto it = flags_.find(name);
        return it != flags_.end() && it->second;
    }

    // Returns false (no side effect) if amount > money_; otherwise deducts.
    [[nodiscard]] bool DeductMoney(int amount) noexcept;

    // Rain exposure (umbrella-less): +5 units/sec, clamped [0,100].
    // lethal=true (default) respawns the player at the 正門 gate
    // (resetRainMeter + ShowMessage) when the meter fills; lethal=false
    // suppresses only that teleport (param kept for unit tests / opt-out).
    // Cycle 4: the survival loop is live — GameController accrues this
    // while outdoors-and-umbrella-less and DRAINS (DrainRain) while
    // sheltered, so a full meter is a manage-your-exposure failure, not a
    // hidden one-shot timer.
    Player& ApplyRain(float dt, bool lethal = true);

    // Rain recovery while FULLY sheltered (inside a building): -10
    // units/sec, clamped [0,100]; never teleports.
    Player& DrainRain(float dt) noexcept;

    // G4: a FIXED rain-meter reduction (units, not a rate), clamped
    // [0,100], never teleports. Applied when a consumable is USED from the
    // bag (防水噴霧 -35 / 暖暖包 -25 / 提神飲料 -15 / 小吃 -15) so food &
    // gear buy back exposure on use, on top of any karma/quest effect. The
    // umbrella's rain relief stays the AUTOMATIC hold-time path
    // (ApplyRainSheltered), unchanged — this is the discrete item path.
    Player& DrainRainBy(float amount) noexcept;

    // REQUIREMENT #5: rain pressure must exist in EVERY chapter, not
    // just Ch1. Before this, holding the Ch1 umbrella made the player
    // permanently rain-immune (ApplyRain self-noops with an umbrella AND
    // the GC treated "has umbrella" as full shelter → DrainRain), so
    // Ch2/Ch3/Ch4 had ~0 rain pressure (Ch3 was literally 0.0 every
    // frame). An umbrella in the heavy 山下 rain should SLOW the soak,
    // not stop it (chapter2.md explicitly: a held umbrella still
    // accrues at a reduced rate). ApplyRainSheltered models exactly
    // that: a partial accrual (kept ≈30 % of the exposed +5 u/s) that is
    // still lethal-armed, so an umbrella now buys TIME, not immunity —
    // the player must still periodically duck into a building to fully
    // recover (DrainRain). GameController calls, per frame:
    //   inside a building     -> DrainRain          (full recovery)
    //   outdoors + umbrella   -> ApplyRainSheltered  (slow accrual)
    //   outdoors, no umbrella -> ApplyRain           (full accrual)
    // ApplyRain / DrainRain semantics are byte-unchanged (the pinned
    // unit contracts hold); this is purely additive.
    Player& ApplyRainSheltered(float dt, bool lethal = true);

private:
    void RespawnAtGate();

    float rainMeter_;
    int karma_;
    bool hasUmbrella_;
    HeldUmbrella heldUmbrella_{HeldUmbrella::None};  // B2.1: bag umbrella row source
    int money_;
    // P2: cursed-pickup count, drives ApplyCursedTaintDecay's per-chapter
    // karma drain. In-class init so the existing Player ctor needs no change;
    // never cleared by SetHasUmbrella/chapter-reset (the moral stain is the
    // run-permanent half of cursed, paired with Flag_TookCursedUmbrella).
    int cursedTaint_{0};
    // Assignment-#6 combat hit-points. NOT serialized by the harness, so it
    // never enters state.jsonl — the present game (no enemy damages the
    // player) leaves it at kMaxHp for the whole run. In-class initialised so
    // the existing Player ctor needs no change.
    int hp_{kMaxHp};
    std::unordered_map<std::string, bool> flags_;
    std::unordered_map<std::string, int>  consumables_;

    std::optional<nccu::gfx::Texture> sprite_;
    nccu::gfx::Color tint_{255, 255, 255, 255};  // persona colour modulate
    nccu::gfx::Vec2 lastFacing_{0.0f, 1.0f};  // start facing down
    float animTimer_{0.0f};
    int   animStep_{0};                       // 0..3 -> column 1,0,1,2
};

#endif // PLAYER_H_
