#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include "InputHandler.h"
#include "SceneRouter.h"
#include "SemesterState.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include <string>
#include <vector>

class Player;  // global-namespace model object
class Vendor;  // shop NPC; pending-purchase target across the dialog frame

namespace nccu {

class World;
struct DialogChoice;

// Applies a confirmed dialog choice's side effects to the player. Free
// function so it is unit-testable without the controller's input loop.
void ApplyDialogChoice(Player& player, const DialogChoice& choice);

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
};

} // namespace nccu

#endif // GAME_CONTROLLER_H_
