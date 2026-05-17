#ifndef TRANSPARENT_UMBRELLA_H_
#define TRANSPARENT_UMBRELLA_H_
#include "Item.h"
#include "gfx/Color.h"

class TransparentUmbrella : public Item {
public:
    TransparentUmbrella(nccu::gfx::Vec2 position, std::string name, nccu::gfx::Color tint)
        : Item(position, nccu::gfx::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint) {}

    void Update(float /*deltaTime*/) override {}
    void Render(nccu::gfx::IRenderer& renderer) const override; // 3-rect glyph via IRenderer (Template Method)
    // Both pick-up paths route through the same quest gate (defined in
    // the .cpp so it can see Player/EventBus): an umbrella may only be
    // claimed once the player has taken the 苦主's request
    // (Flag_PromisedVictim) — before that it fires a guidance cue
    // instead of a silent no-op. By Ch3/Ch4 the flag is long set, so the
    // TrueUmbrella re-claim there is unaffected.
    void Interact(Player* initiator) override;
    void OnPickup(Player* player) override;

    virtual void beClaimed(Player* player) = 0;

protected:
    nccu::gfx::Color umbrellaTint_;
};

#endif // TRANSPARENT_UMBRELLA_H_
