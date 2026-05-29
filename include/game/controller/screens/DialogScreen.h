#ifndef CONTROLLER_SCREENS_DIALOG_SCREEN_H_
#define CONTROLLER_SCREENS_DIALOG_SCREEN_H_

class Vendor;
class EventBus;     // Plan P2: bus is forwarded into the gate calls below

namespace nccu {

class World;
class InputHandler;
class SceneRouter;

// Dialog freeze: while a conversation is open the world is paused — the
// orchestrator runs ONLY this handler and skips the object tick /
// movement / collision / building-entry / sweep. IsKeyPressed is edge-
// triggered, so the E that opened the box (handled in DispatchInteract
// last frame) does not auto-advance it this frame. Also routes a
// confirmed vendor stock line to Vendor::TryBuy. Returns true while a
// conversation is open (the world stays frozen), false when no dialog
// is active.
//
// pendingVendor is captured by reference because it MUTATES inside this
// function (cleared on decline / on buy / on a non-vendor dialog
// taking over). input is the InputHandler that owns the hold-E
// auto-advance edge timing (Cycle 10.P0a). sceneRouter is the roster
// settler for the same-frame transition close (Cycle 10.P0b / L8 fix).
[[nodiscard]] bool HandleDialog(EventBus& bus, World& world,
                                Vendor*& pendingVendor,
                                InputHandler& input,
                                SceneRouter& sceneRouter);

} // namespace nccu

#endif // CONTROLLER_SCREENS_DIALOG_SCREEN_H_
