#include "game/controller/InteractDispatch.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/vendor/Vendor.h"
#include "game/controller/VendorMenu.h"
#include "game/controller/GameObjectQueries.h"
#include "game/quest/NpcSpawns.h"     // G-2: IsChapter1FlavorNpc routing
#include "game/quest/QuestHookTable.h"
#include "engine/core/GameObject.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/math/Rect.h"
#include <string_view>

namespace nccu {
using namespace nccu::engine::input;  // Phase 4 §B: input types moved out of nccu::gfx

void DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    using nccu::engine::math::Rect;
    using nccu::queries::ForEachActiveExcept;
    Player* player = world.GetPlayer();
    if (Input::IsPressed(Key::E) && player) {
        // I3 fix: the movement collider for a BlocksMovement() NPC is a
        // player-sized box at the NPC origin, and Rect::Intersects is
        // strict, so physics::ResolveMove halts the player EXACTLY flush
        // against a static NPC (touching, never strictly overlapping). A
        // 24x24 E-probe at the player origin therefore never collides —
        // dialog could never open for a walked-up player (harness OR
        // human). Give the E-probe an explicit interaction reach: inflate
        // it by kInteractReach on every side so a flush-blocked player
        // still overlaps the NPC's hitbox. The MOVEMENT collider is left
        // exactly as-is (frameColliders_ above, unchanged) — the player
        // still cannot walk through an NPC; only the talk reach grows.
        // The margin (8 px, a third of the 24 px box) is far smaller than
        // the world spacing between NPCs/items, so it can only reach an
        // object the player is already standing flush against.
        // Cycle 9.E (audit M2 / D7 / SC 2.5.8): a "larger targets"
        // accessibility profile (World::LargeTargets(),
        // UMBRELLA_LARGE_TARGETS=1) widens the reach to 16 px on each
        // side — effective talk box 56x56 instead of 40x40 — so a
        // tremor-affected player can still trigger NPC dialog without
        // pixel-precise alignment. The MOVEMENT collider above is
        // unchanged (the player still cannot walk through an NPC); ONLY
        // the E-probe reach grows. Default off ⇒ byte-equivalent to
        // pre-9.E behaviour and to the prior `kInteractReach = 8.0f`.
        const float kInteractReach = world.LargeTargets() ? 16.0f : 8.0f;
        const Rect pHit{player->GetPosition().x - kInteractReach,
                        player->GetPosition().y - kInteractReach,
                        24.0f + 2.0f * kInteractReach,
                        24.0f + 2.0f * kInteractReach};
        ForEachActiveExcept(world.Objects(), player,
            [&bus, &world, player, pHit, &pendingVendor](GameObject& o) {
                if (!o.CheckCollision(pHit)) return;
                // I5: a Vendor's NpcId() is empty, so without this it
                // would fall to o.Interact() (NPC line-cycling) and
                // Vendor::TryBuy would have no runtime caller — Ending C
                // / the Ch2 EnergyDrink were unreachable. Route a shop
                // interaction to a buy-choice dialog instead; the purchase
                // itself (money / inventory / EventBus events / soft-cap /
                // setsFlag) stays entirely inside Vendor::TryBuy, invoked
                // on confirm in the dialog branch above. One menu per E
                // tap: skip if a dialog already opened this pass.
                if (o.IsVendor()) {
                    if (world.Dialog().Active()) return;
                    auto* vendor = static_cast<Vendor*>(&o);
                    OpenVendorMenu(world.Dialog(), *vendor);
                    pendingVendor = vendor;
                    return;
                }
                if (const std::string_view id = o.NpcId(); !id.empty()) {
                    // G-2: a Ch1 stationary FLAVOR NPC (搶課同學 / 撐傘路人 /
                    // 揹書包學生) cycles its own chapter1.md line pool one line
                    // per talk via NPC::Interact() — a deterministic,
                    // reproducible pick (no RNG on the state path). Route it
                    // HERE, BEFORE any spine hook, and return: a flavor NPC
                    // must never reach TryReturnVictimUmbrella / OpenNpcDialog
                    // etc., so it provably sets NO quest flag and cannot
                    // perturb the hard-gated 苦主→學長→苦主 spine. (The dialog
                    // it shows is a one-line ShowMessage toast, same channel
                    // as an ambient-pedestrian Interact, not the dialog box.)
                    if (nccu::IsChapter1FlavorNpc(id)) {
                        if (auto* it = o.AsInteractable()) it->Interact(player);
                        return;
                    }
                    // Run the registered Ch1-Ch4 quest hooks, IN ORDER,
                    // BEFORE the dialog opener — exactly the inline TryXxx
                    // sequence this replaces (TryReturnVictimUmbrella ->
                    // TryRescueBookworm -> TryMeetLibrarian ->
                    // TryLendLibrarianUmbrella -> TryReturnLibrarianUmbrella
                    // -> TryApplyCh2Ripple -> TryAdvanceCh3Trade ->
                    // TryApplyCh3Ripple -> TryApplyCh4Ripple). Each hook
                    // self-gates on (state, npcId) and is a cheap no-op
                    // outside its chapter, so running the whole table per
                    // interact is correct and order-stable. The 4th arg is
                    // the Interlude return-target (only the librarian-return
                    // hook scopes itself with it). Adding a chapter/NPC is
                    // now a RegisterHook line, not an edit here (OCP).
                    RunInteractHooks(bus, *player, id,
                                     world.Semester().Current(),
                                     world.Semester().InterludeReturnTo());
                    OpenNpcDialog(world.Dialog(), *player, id,
                                  world.Semester().Current());     // talk
                } else if (auto* it = o.AsInteractable()) {
                    it->Interact(player);                            // pick up / Vendor
                }
            });
    }
}

} // namespace nccu
