#ifndef CHARACTER_SELECT_H_
#define CHARACTER_SELECT_H_
#include "gfx/Window.h"
#include <string>

namespace nccu {

struct CharacterSelectResult {
    // Resource-relative path of the chosen sprite sheet, e.g.
    // "resources/assets/sprites/school_uniform_3/female_03.png".
    // Empty when `closed` is true.
    std::string spritePath;

    // True if the user closed the window mid-selection — the caller should
    // skip world construction and exit cleanly.
    bool closed{false};
};

// Two-phase pre-gameplay screen: pick gender, then pick figure index.
// Runs its own draw loop on `win` and blocks until the player either
// confirms a figure or closes the window. Loads sprite previews directly
// from resources/assets/sprites/school_uniform_3/.
CharacterSelectResult RunCharacterSelect(gfx::Window& win);

} // namespace nccu

#endif // CHARACTER_SELECT_H_
