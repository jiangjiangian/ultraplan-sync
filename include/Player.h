#pragma once
#include "Character.h"

class Player : public Character {
public:
    Player(Vector2 position);

    void Update(float deltaTime) override;
    void Draw() const override;
    void Interact(Player* initiator) override;

    void HandleInput(float deltaTime);
    void decreaseKarma(int amount);
    void resetRainMeter();

    int GetKarma() const { return karma_; }
    float GetRainMeter() const { return rainMeter_; }
    bool HasUmbrella() const { return hasUmbrella_; }
    void SetHasUmbrella(bool v) { hasUmbrella_ = v; }

private:
    float rainMeter_;
    int karma_;
    bool hasUmbrella_;
};
