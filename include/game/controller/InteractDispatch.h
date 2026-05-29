#ifndef CONTROLLER_INTERACT_DISPATCH_H_
#define CONTROLLER_INTERACT_DISPATCH_H_

class Vendor;
class EventBus;        // Plan P2 step 2: bus is threaded to RunInteractHooks

namespace nccu {

class World;

// E-interact dispatch (talk / pick up / open a shop). Reads the E edge
// and the player's reach box, so it stays in the controller layer. For a
// non-flavor NPC it runs the registered QuestHook table (RunInteractHooks)
// — the ~14 inline TryXxx calls are now DATA (quest/QuestHookTable.cpp),
// in the SAME order and with the SAME self-gating semantics — then opens
// the per-(npcId, state) dialog. A flavor NPC short-circuits to its own
// line-cycling Interact; a non-NPC (pickup / Vendor) routes through its
// IInteractable role. Same operations, same order as the inline block.
//
// pendingVendor is a non-owning observer of a World-owned Vendor whose
// buy menu opened this frame; HandleDialog reads it on confirm to route
// to Vendor::TryBuy. Captured by reference so this function can set it
// when an E tap opens a shop. Cleared the moment a non-vendor dialog
// opens, OR on a vendor decline/buy.
void DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor);

} // namespace nccu

#endif // CONTROLLER_INTERACT_DISPATCH_H_
