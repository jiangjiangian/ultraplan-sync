#ifndef TRANSPARENT_UMBRELLA_H_
#define TRANSPARENT_UMBRELLA_H_
#include "entities/Item.h"
#include "gfx/Color.h"

// REQUIREMENT #9: the umbrellas a player chooses between in Ch1 must
// look CLEARLY different, not four near-identical pale-blue glyphs.
// Render() switches on this per-subclass silhouette so each reads at a
// glance even before the colour is parsed (and on displays where the
// tints are subtle). Pure data on the object — MVC stays clean (the
// View reads it in Render(); World/Item carry no raylib).
enum class UmbrellaStyle {
    Domed,    // True — wide rounded canopy: the "clean / correct" read
    Broken,   // Fragile — small canopy with a torn-off rib (cheap/weak)
    Spiked,   // ProfessorTrap — angular stepped canopy (danger / trap)
    Drooping  // Cursed — sagging dark canopy + black handle (wrong)
};

class TransparentUmbrella : public Item {
public:
    TransparentUmbrella(nccu::gfx::Vec2 position, std::string name,
                        nccu::gfx::Color tint,
                        UmbrellaStyle style = UmbrellaStyle::Domed)
        : Item(position, nccu::gfx::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint), style_(style) {}

    void Update(float /*deltaTime*/) override {}
    void Render(nccu::gfx::IRenderer& renderer) const override; // per-style glyph via IRenderer (Template Method)

    [[nodiscard]] UmbrellaStyle Style() const noexcept { return style_; }
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
    UmbrellaStyle    style_{UmbrellaStyle::Domed};
};

#endif // TRANSPARENT_UMBRELLA_H_
