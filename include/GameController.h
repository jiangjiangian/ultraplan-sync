#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include "SemesterState.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include <string>
#include <vector>

class Player;  // global-namespace model object

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
    // The SemesterState the live NPC roster was last spawned for. When
    // the FSM moves past it (via ANY trigger — EndingGate, EventWiring,
    // future) the next Update() asks World to respawn. Keeps the state
    // machine pure: no World/EventBus dependency, no new EventType.
    nccu::SemesterState                                  lastRosterState_;
};

} // namespace nccu

#endif // GAME_CONTROLLER_H_
