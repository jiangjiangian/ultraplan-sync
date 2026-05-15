#ifndef VIEW_H_
#define VIEW_H_
#include "gfx/RaylibRenderer.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"

namespace nccu {

class World; // read-only model handed to Draw()

// The rendering layer — owns the concrete renderer, the follow camera and
// the worldmap texture. Draw() is the ONLY place game state becomes
// pixels: it positions its own camera from the model, blits the map,
// renders every active object through IRenderer, then the HUD. Reads the
// World const; never mutates it.
class View {
public:
    View(int windowWidth, int windowHeight);

    void Draw(const World& world);

private:
    nccu::gfx::RaylibRenderer renderer_;
    nccu::gfx::Camera2D       camera_;
    nccu::gfx::Texture        worldmap_;
    nccu::gfx::Vec2           screenCenter_;
    nccu::gfx::Vec2           worldSize_;
    nccu::gfx::Vec2           viewportSize_;
};

} // namespace nccu

#endif // VIEW_H_
