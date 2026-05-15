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
    Player& AddMoney(int amount) noexcept   { money_ += amount; return *this; }
    Player& SetFlag(const std::string& name)   { flags_[name] = true; return *this; }
    Player& ClearFlag(const std::string& name) { flags_.erase(name);  return *this; }

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

    std::optional<nccu::gfx::Texture> sprite_;
    nccu::gfx::Vec2 lastFacing_{0.0f, 1.0f};  // start facing down
    float animTimer_{0.0f};
    int   animStep_{0};                       // 0..3 -> column 1,0,1,2
};

#endif // PLAYER_H_
