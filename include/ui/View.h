#ifndef VIEW_H_
#define VIEW_H_
#include "gfx/RaylibRenderer.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include "ui/ChapterCard.h"
#include "state/SemesterState.h"
#include <cstddef>
#include <optional>
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
    // A single entry in the per-frame painter's-order list, depth-sorted
    // by `y`. `kind` selects the payload: Object → `obj` is the
    // GameObject; Building → `index` is into buildings_; Decoration →
    // `index` is into decorations_. (Decorations join the SAME sort so an
    // ambient "statue" walk-behinds correctly against NPCs/buildings.)
    enum class DrawKind { Object, Building, Decoration };
    struct DrawRef {
        float               y;
        DrawKind            kind;
        const ::GameObject* obj;     // valid iff kind == Object
        std::size_t         index;   // buildings_/decorations_ index otherwise
    };
    // One LOADED ambient decoration strip: which kDecorations[] def it
    // realises + its loaded texture. Built in the ctor; a def whose PNG
    // is missing is simply skipped (no entry), so the missing-asset path
    // draws nothing. These are View-side cosmetics — never GameObjects,
    // never in World::Objects() — so the harness state.jsonl is unchanged.
    struct DecorationSprite {
        std::size_t        defIndex;   // into nccu::gfx::kDecorations
        nccu::gfx::Texture texture;    // the loaded strip (move-only)
    };

    nccu::gfx::RaylibRenderer        renderer_;
    nccu::gfx::Camera2D              camera_;
    nccu::gfx::Texture               worldmap_;
    nccu::gfx::Vec2                  screenCenter_;
    nccu::gfx::Vec2                  worldSize_;
    nccu::gfx::Vec2                  viewportSize_;
    std::vector<nccu::gfx::Texture>  buildingTextures_;
    std::vector<BuildingSprite>      buildings_;
    std::vector<DecorationSprite>    decorations_;  // ambient strips (cosmetic)
    std::vector<DrawRef>             drawOrder_;  // per-frame scratch
    float                            endingAlpha_ = 0.0f;  // card fade-in
    // Render clock (seconds) for the ping-pong decoration animation —
    // accumulates Time::DeltaSeconds() each frame the world is drawn. NOT
    // simulation state (pure View flourish, mirrors interludeMarkerPhase_
    // / endingAlpha_), so the harness timeline / state.jsonl is unchanged.
    double                           decorationClock_ = 0.0;
    // H3: animation phase (in world px) for the Interlude exit ground
    // marker. Ticked by DeltaSeconds * speed each frame the player is in
    // the Interlude, so the dashed line sweeps west-to-east. Not part of
    // the simulation — pure View visual flourish.
    float                            interludeMarkerPhase_ = 0.0f;
    // U1-T2 chapter bookend big card. The View detects a SemesterState
    // change (vs lastSemester_) each frame and arms chapterCard_ for the
    // 傘又掉了 (chapter start) / 找到傘了 (chapter clear) beat — driven
    // entirely off the FSM the View already reads, so NO new event/publish
    // and state.jsonl stays byte-identical. The card's timer is ticked by
    // Time::DeltaSeconds() (fixed step under the harness) => deterministic.
    // lastSemester_ is std::nullopt until the first Draw so the opening
    // Chapter1 entry fires its inciting card on frame 0.
    std::optional<nccu::SemesterState> lastSemester_;
    ChapterCardState                   chapterCard_;
};

} // namespace nccu

#endif // VIEW_H_
