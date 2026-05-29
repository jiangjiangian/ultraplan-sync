#ifndef DLC_SIGN_H_
#define DLC_SIGN_H_
#include "engine/core/GameObject.h"
#include "engine/math/Vec2.h"
#include <string>

// B2: a re-readable world "easter-egg" sign — a big bold "?" standing at
// the 風雩走廊. On E-interact it publishes a ShowMessage teaser
// (「DLC開發中\n敬請期待」, rendered as two centred toast lines) and is NEVER
// consumed, so the player can
// read it as many times as they like (unlike a QuestFlagPickup / CashPickup,
// which deactivate on pickup). It has no gameplay effect: no flag, no karma,
// no money, no quest hook — purely decorative flavour spawned for the
// open-explore Chapter4_Finals.
//
// It is deliberately NOT an Item subclass: an Item is a pickable thing with
// OnPickup semantics + isPickable_, and the E-interact sweep would expect a
// one-shot collect. The sign is a fixed piece of scenery you can talk to, so
// it lives directly under GameObject with only the two roles it actually
// plays: IDrawable (it draws its own "?" via the injected IRenderer, exactly
// like UmbrellaGlyph / QuestFlagPickup — no View edit, no DrawText, rect-only
// primitives) and IInteractable (its Interact publishes the teaser). Its
// NpcId() stays empty and IsVendor() stays false, so the controller's
// E-interact sweep routes it down the generic `AsInteractable()->Interact()`
// branch — never the NPC-dialog / Vendor-buy paths.
//
// ISP roles: IDrawable + IInteractable (no IUpdatable — the sign never
// ticks). A leaf, so WithRoles is keyed on DlcSign itself.
class DlcSign final : public WithRoles<DlcSign, GameObject>,
                      public IDrawable, public IInteractable {
public:
    explicit DlcSign(nccu::gfx::Vec2 position);

    void Render(nccu::gfx::IRenderer& renderer) const override;
    // Re-readable: publishes the teaser ShowMessage but does NOT set
    // isActive_ = false, so the sign survives the interact and the player
    // can read it again. No null-initiator guard needed beyond the early
    // return (it touches no Player state).
    void Interact(Player* initiator) override;

private:
    std::string message_;
};

#endif // DLC_SIGN_H_
