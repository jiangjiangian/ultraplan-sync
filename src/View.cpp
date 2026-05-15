#include "View.h"
#include "World.h"
#include "Player.h"
#include "GameObjectQueries.h"
#include "WorldConfig.h"
#include "gfx/Renderer.h"
#include "gfx/CameraScope.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
#include <cstdio>
#include <string>

namespace nccu {

View::View(int windowWidth, int windowHeight)
    : worldmap_(nccu::gfx::Texture::Load("resources/assets/maps/worldmap.png")),
      screenCenter_{windowWidth / 2.0f, windowHeight / 2.0f},
      worldSize_{::world::kSize, ::world::kSize},
      viewportSize_{static_cast<float>(windowWidth),
                    static_cast<float>(windowHeight)} {}

void View::Draw(const World& world) {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;

    if (const Player* p = world.GetPlayer()) {
        camera_.Follow(p->GetPosition(), screenCenter_)
               .ClampToWorld(worldSize_, viewportSize_);
    }

    Renderer{}.Clear(Colors::RayWhite);
    {
        CameraScope cam{camera_};
        Renderer{}.Texture(worldmap_, Vec2{0.0f, 0.0f});
        ForEachActive(world.Objects(),
                      [this](const GameObject& o) { o.Render(renderer_); });
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
}

} // namespace nccu
