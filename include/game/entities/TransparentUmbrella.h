#ifndef TRANSPARENT_UMBRELLA_H_
#define TRANSPARENT_UMBRELLA_H_
#include "game/entities/Item.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"

// REQUIREMENT #9: the umbrellas a player chooses between in Ch1 must
// look CLEARLY different, not four near-identical pale-blue glyphs.
// Render() switches on this per-subclass silhouette so each reads at a
// glance even before the colour is parsed (and on displays where the
// tints are subtle). Pure data on the object — MVC stays clean (the
// View reads it in Render(); World/Item carry no raylib).
enum class UmbrellaStyle {
    Domed,    // True — wide rounded canopy: the "clean / correct" read
    Broken,   // Fragile — only the handle / bare ribs remain (broken)
    Spiked,   // ProfessorTrap — angular stepped canopy (danger / trap)
    Drooping  // Cursed — sagging dark canopy + black handle (wrong)
};

// Map a subclass's UmbrellaStyle to the shared gfx::UmbrellaLook (the single
// source of truth for the silhouette + signature colour). One mapping used by
// BOTH the in-world Render here AND any other surface that wants the same
// look (kept inline next to the styles so the two never drift).
[[nodiscard]] constexpr nccu::gfx::UmbrellaLook
LookForStyle(UmbrellaStyle style) noexcept {
    switch (style) {
        case UmbrellaStyle::Domed:    return nccu::gfx::UmbrellaLook::TrueBlue;
        case UmbrellaStyle::Broken:   return nccu::gfx::UmbrellaLook::FragileBroken;
        case UmbrellaStyle::Spiked:   return nccu::gfx::UmbrellaLook::ProfessorTrap;
        case UmbrellaStyle::Drooping: return nccu::gfx::UmbrellaLook::CursedPurple;
    }
    return nccu::gfx::UmbrellaLook::TrueBlue;
}

// ISP roles: IDrawable + IInteractable. The old Update body was an empty
// no-op (umbrellas don't tick), so that role is dropped; Render (the
// per-style glyph) and Interact (the quest-gated claim) are real and kept.
// All four leaves (True/Fragile/ProfessorTrap/Cursed) only override
// beClaimed() — they share this exact role set — so WithRoles is keyed on
// this intermediate (Derived = TransparentUmbrella); static_cast to it is
// valid for every leaf.
class TransparentUmbrella : public WithRoles<TransparentUmbrella, Item>,
                            public IDrawable, public IInteractable {
public:
    TransparentUmbrella(nccu::gfx::Vec2 position, std::string name,
                        nccu::gfx::Color tint,
                        UmbrellaStyle style = UmbrellaStyle::Domed)
        : WithRoles(position, nccu::gfx::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint), style_(style) {}

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
