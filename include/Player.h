#ifndef PLAYER_H_
#define PLAYER_H_
#include "Character.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"
#include <optional>
#include <string>
#include <unordered_map>

class Player final : public Character {
public:
    explicit Player(nccu::gfx::Vec2 position);

    void Update(float deltaTime) override;
    void Render(nccu::gfx::IRenderer& renderer) const override;
    void Interact(Player* initiator) override;

    void HandleInput(float deltaTime);

    // Mutators return *this so callers can chain:
    //   player.AddKarma(10).AddMoney(50).SetHasUmbrella(true);
    Player& AddKarma(int delta);            // clamped to [-100, 100]
    Player& decreaseKarma(int amount);      // thin wrapper, prefer AddKarma
    Player& resetRainMeter() noexcept;
    Player& SetHasUmbrella(bool v) noexcept { hasUmbrella_ = v; return *this; }
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

    // Rain accumulation: 5 units/sec exposure when umbrella-less; clamped
    // [0,100]. When the meter fills, the player is respawned at the 正門
    // gate and a ShowMessage event is emitted via EventBus.
    Player& ApplyRain(float dt);

private:
    void RespawnAtGate();

    float rainMeter_;
    int karma_;
    bool hasUmbrella_;
    int money_;
    std::unordered_map<std::string, bool> flags_;
    std::unordered_map<std::string, int>  consumables_;

    std::optional<nccu::gfx::Texture> sprite_;
    nccu::gfx::Vec2 lastFacing_{0.0f, 1.0f};  // start facing down
    float animTimer_{0.0f};
    int   animStep_{0};                       // 0..3 -> column 1,0,1,2
};

#endif // PLAYER_H_
