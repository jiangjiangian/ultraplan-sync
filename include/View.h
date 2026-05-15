#ifndef VIEW_H_
#define VIEW_H_
#include "gfx/RaylibRenderer.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <cstddef>
#include <vector>

class GameObject; // global-namespace model object, drawn through IRenderer

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
    // One placed building: which loaded texture, where it lands in world
    // pixels, and the ground line (sprite bottom) used for depth sorting.
    struct BuildingSprite {
        std::size_t     texIndex;
        nccu::gfx::Rect dest;
        float           baseY;
        bool            flipX;
        bool            flipY;
    };
    // A single entry in the per-frame painter's-order list. Exactly one
    // of {obj, building} is meaningful: obj!=nullptr → a GameObject,
    // otherwise `building` indexes buildings_.
    struct DrawRef {
        float               y;
        const ::GameObject* obj;
        std::size_t         building;
    };

    nccu::gfx::RaylibRenderer        renderer_;
    nccu::gfx::Camera2D              camera_;
    nccu::gfx::Texture               worldmap_;
    nccu::gfx::Vec2                  screenCenter_;
    nccu::gfx::Vec2                  worldSize_;
    nccu::gfx::Vec2                  viewportSize_;
    std::vector<nccu::gfx::Texture>  buildingTextures_;
    std::vector<BuildingSprite>      buildings_;
    std::vector<DrawRef>             drawOrder_;  // per-frame scratch
};

} // namespace nccu

#endif // VIEW_H_
