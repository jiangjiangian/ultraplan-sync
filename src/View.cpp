#include "View.h"
#include "World.h"
#include "Player.h"
#include "GameObject.h"
#include "GameObjectQueries.h"
#include "Buildings.h"
#include "Obstacles.h"
#include "WorldConfig.h"
#include "DialogView.h"
#include "EndingView.h"
#include "InventoryView.h"
#include "gfx/Renderer.h"
#include "gfx/Time.h"
#include "gfx/CameraScope.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace nccu {

// Buildings are drawn as separate sprites over a terrain-only base so a
// character standing above a building's ground line is occluded by it
// (walk-behind). The base map carries the open-ground features (running
// track, plaza), so the two named in kBuildingCollisionSkip get no
// sprite — their pixels already live in worldmap_base.png.
View::View(int windowWidth, int windowHeight)
    : worldmap_(nccu::gfx::Texture::Load("resources/assets/maps/worldmap_base.png")),
      screenCenter_{windowWidth / 2.0f, windowHeight / 2.0f},
      worldSize_{::world::kSize, ::world::kSize},
      viewportSize_{static_cast<float>(windowWidth),
                    static_cast<float>(windowHeight)} {
    const auto& skip = obstacles::kBuildingCollisionSkip;
    buildingTextures_.reserve(buildings::kAll.size());
    buildings_.reserve(buildings::kAll.size());
    for (const auto& b : buildings::kAll) {
        if (std::find(skip.begin(), skip.end(), b.name) != skip.end()) continue;
        std::string path = "resources/assets/buildings_3d_trimmed/";
        path += std::string(b.name);
        path += ".png";
        auto tex = nccu::gfx::Texture::Load(path);
        if (!tex.IsValid()) continue;  // art not generated yet → no sprite
        const std::size_t idx = buildingTextures_.size();
        buildingTextures_.push_back(std::move(tex));
        buildings_.push_back(BuildingSprite{
            idx, b.triggerRect,
            b.triggerRect.y + b.triggerRect.height,
            b.flipX, b.flipY});
    }
}

void View::Draw(const World& world) {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;

    const SemesterState st = world.Semester().Current();
    if (IsEndingState(st)) {
        endingAlpha_ = std::min(1.0f, endingAlpha_ +
                                nccu::gfx::Time::DeltaSeconds());
        DrawEndingCard(renderer_, st, world.Semester().CurrentName(),
                       endingAlpha_, viewportSize_.x, viewportSize_.y);
        return;   // ending replaces the world; player has no agency here
    }

    if (const Player* p = world.GetPlayer()) {
        camera_.Follow(p->GetPosition(), screenCenter_)
               .ClampToWorld(worldSize_, viewportSize_);
    }

    Renderer{}.Clear(Colors::RayWhite);
    {
        CameraScope cam{camera_};
        Renderer{}.Texture(worldmap_, Vec2{0.0f, 0.0f});

        // Painter's order: buildings keyed on their ground line, objects
        // on their feet (top-left + player height). Lower y paints first,
        // so a character above a building's base is covered by it; one
        // standing below it draws on top — classic JRPG walk-behind.
        drawOrder_.clear();
        drawOrder_.reserve(buildings_.size() + world.Objects().size());
        for (std::size_t i = 0; i < buildings_.size(); ++i) {
            drawOrder_.push_back(DrawRef{buildings_[i].baseY, nullptr, i});
        }
        ForEachActive(world.Objects(), [this](const GameObject& o) {
            drawOrder_.push_back(DrawRef{
                o.GetPosition().y + ::world::kPlayerHeight, &o, 0});
        });
        std::sort(drawOrder_.begin(), drawOrder_.end(),
                  [](const DrawRef& a, const DrawRef& b) { return a.y < b.y; });
        for (const DrawRef& d : drawOrder_) {
            if (d.obj) {
                d.obj->Render(renderer_);
            } else {
                const BuildingSprite& bs  = buildings_[d.building];
                const Texture&        tex = buildingTextures_[bs.texIndex];
                const float sw = static_cast<float>(tex.Width());
                const float sh = static_cast<float>(tex.Height());
                // Negative source extents make DrawTexturePro mirror the
                // sprite — carries the Tiled flip into the render.
                renderer_.DrawSprite(
                    tex,
                    Rect{0.0f, 0.0f,
                         bs.flipX ? -sw : sw,
                         bs.flipY ? -sh : sh},
                    bs.dest);
            }
        }
    }

    TextBuilder{"WASD: move    E: pick up"}
        .At(Vec2{10, 10}).Size(16).Color(Colors::DarkGray).Draw();
    if (const Player* p = world.GetPlayer()) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "karma: %d   umbrella: %s",
            p->GetKarma(), p->HasUmbrella() ? "yes" : "no");
        TextBuilder{buf}
            .At(Vec2{10, 30}).Size(16).Color(Colors::DarkGray).Draw();
    }
    if (!world.CurrentBuildingName().empty()) {
        TextBuilder{"Inside: " + world.CurrentBuildingName()}
            .At(Vec2{10, 50}).Size(16).Color(Colors::Black).Draw();
    }
    TextBuilder{std::string{world.Semester().CurrentName()}}
        .At(Vec2{10, 70}).Size(16).Color(Colors::Blue).Draw();

    DrawDialog(renderer_, world.Dialog());

    // Tab inventory overlay, on top of the (frozen) world + dialog.
    // Reactive: a pure function of World::InventoryOpen + the Player's
    // count map, drawn each frame it is open (no retained UI state).
    if (world.InventoryOpen()) {
        if (const Player* ip = world.GetPlayer())
            DrawInventory(renderer_, ip->Consumables(),
                          viewportSize_.x, viewportSize_.y);
    }
}

} // namespace nccu
