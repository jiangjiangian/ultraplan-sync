#pragma once
#include "Character.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"
#include <optional>
#include <string>

class Player : public Character {
public:
    explicit Player(nccu::gfx::Vec2 position);

    void Update(float deltaTime) override;
    void Draw() const override;
    void Interact(Player* initiator) override;

    void HandleInput(float deltaTime);
    void decreaseKarma(int amount);
    void resetRainMeter();

    // Loads a Pipoya 96x128 sprite sheet (3 walk frames x 4 directions of
    // 32x32 each). Replaces any previously loaded sheet.
    void LoadSprite(const std::string& path);

    int GetKarma() const { return karma_; }
    float GetRainMeter() const { return rainMeter_; }
    bool HasUmbrella() const { return hasUmbrella_; }
    void SetHasUmbrella(bool v) { hasUmbrella_ = v; }

private:
    float rainMeter_;
    int karma_;
    bool hasUmbrella_;

    std::optional<nccu::gfx::Texture> sprite_;
    nccu::gfx::Vec2 lastFacing_{0.0f, 1.0f};  // start facing down
    float animTimer_{0.0f};
    int   animStep_{0};                       // 0..3 -> column 1,0,1,2
};
