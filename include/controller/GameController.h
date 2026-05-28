#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include "controller/InputHandler.h"
#include "controller/SceneRouter.h"
#include "controller/SimSystem.h"
#include "state/SemesterState.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <memory>
#include <string>
#include <vector>

class Player;  // global-namespace model object
class Vendor;  // shop NPC; pending-purchase target across the dialog frame

namespace nccu {

class World;
struct DialogChoice;

// The input + simulation layer. One Update() advances the world by a
// frame: ticks every object, resolves player collision, dispatches the
// E-key interact, detects building entry, sweeps the dead. Owns the
// EventBus wiring — installed in the ctor, torn down in the dtor so no
// singleton-bound lambda outlives the World refs it captured. Mutates
// the World; never renders, never reads input devices for the View.
//
// Cycle 10.P0a (awsome_cpp.md §6): the input edge timing has moved into
// InputHandler and the FSM transition observer has moved into
// SceneRouter. The Controller stays the orchestrator that wires them to
// the World — exactly the §6 "Controller stays the orchestrator"
// shape, with one concrete responsibility per helper class.
class GameController {
public:
    explicit GameController(World& world);
    ~GameController();

    GameController(const GameController&)            = delete;
    GameController& operator=(const GameController&) = delete;

    void Update();

private:
    // Per-screen INPUT sub-handlers (each reads gfx::Input, so they stay
    // in the controller layer — NOT systems). Each returns true when it
    // consumed the frame and the world must stay frozen (the orchestrator
    // then returns before the simulation pipeline runs), false to fall
    // through to the next screen / the sim. Decomposed out of the former
    // ~793-line Update() god-method (awsome_cpp.md §7 SRP) one-to-one with
    // the original spatially-isolated blocks; input semantics unchanged.
    bool HandleEndingMenu();   // A-T3: the ending screen's bottom menu.
    bool HandlePauseMenu();    // M pause menu + the 說明 help overlay.
    bool HandleDialog();       // dialog advance + vendor-purchase routing.
    bool HandleInventory();    // Tab bag: ↑/↓ select, ←/→ page, E/Enter use.
    // E-interact dispatch: the per-frame talk/pickup probe. Reads input
    // (the E edge + the reach box), so it stays in the controller; it
    // delegates the quest side-effects to the registered QuestHook table
    // (RunInteractHooks) instead of inlining ~14 TryXxx calls.
    void DispatchInteract();

    World&                                               world_;
    std::vector<nccu::gfx::Rect>                         frameColliders_;
    nccu::gfx::Vec2                                       worldSize_;
    nccu::gfx::Vec2                                       playerSize_;
    // Cycle 10.P0a: roster-respawn cursor + interlude-exit latch live
    // on the SceneRouter now. lastRosterState_ and
    // interludeExitZoneLatched_ used to be inline members here.
    SceneRouter                                          sceneRouter_;
    // Cycle 10.P0a: dialog hold-E auto-advance timer + cooldown live on
    // the InputHandler now. eHoldMs_ and eAutoAdvanceCooldown_ used to
    // be inline members here.
    InputHandler                                         input_;
    // I5: the Vendor whose buy menu is currently open. A shop interaction
    // opens a choice dialog this frame and the purchase is confirmed in a
    // LATER frame's dialog branch, so the target must survive across the
    // freeze. A non-owning observer of a World-owned object; cleared the
    // moment the menu closes (confirm / no-op) and whenever a non-vendor
    // dialog opens, so it can never dangle past the roster sweep (a
    // vendor dialog freezes the sim, so the swept-roster path cannot run
    // while it is set). nullptr = no shop menu open.
    Vendor*                                              pendingVendor_ = nullptr;

    // The ordered model-advance pipeline (awsome_cpp.md §6/§7). Run, in
    // this exact order, every non-frozen frame BEFORE the E-interact /
    // building-entry / gate-poll logic: Survival(rain) -> Movement(object
    // tick) -> Collision(player AABB+terrain) -> Spawn(lap + deferred
    // spawns). MovementSystem hands the pre-tick player position to
    // CollisionSystem through the SimContext. SweepSystem is the TERMINAL
    // stage (the end-of-frame deferred deletion), run AFTER the interact /
    // gate logic — exactly where the inline sweep used to sit — so the
    // full per-frame order is byte-preserved. Pure model: no system reads
    // input or renders.
    std::vector<std::unique_ptr<ISystem>>                advanceSystems_;
    SweepSystem                                          sweep_;
};

} // namespace nccu

#endif // GAME_CONTROLLER_H_
