#ifndef GAME_CONTROLLER_H_
#define GAME_CONTROLLER_H_
#include "SemesterState.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace nccu {

class World;

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
    const std::unordered_map<std::string, SemesterState> enterTrigger_;
    std::vector<nccu::gfx::Rect>                         frameColliders_;
    nccu::gfx::Vec2                                       worldSize_;
    nccu::gfx::Vec2                                       playerSize_;
};

} // namespace nccu

#endif // GAME_CONTROLLER_H_
