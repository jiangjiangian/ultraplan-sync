#include "DialogView.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include <string>

namespace nccu {

void DrawDialog(nccu::gfx::IRenderer& r, const DialogState& d) {
    using namespace nccu::gfx;
    if (!d.Active()) return;

    r.DrawRect(Rect{20.0f, 320.0f, 760.0f, 110.0f}, Colors::RayWhite);
    r.DrawRect(Rect{20.0f, 320.0f, 760.0f, 2.0f},   Colors::DarkGray);

    if (d.AtChoice()) {
        float y = 336.0f;
        for (int i = 0; i < static_cast<int>(d.Choices().size()); ++i) {
            const std::string mark = (i == d.ChoiceCursor()) ? "> " : "  ";
            r.DrawText(mark + d.Choices()[static_cast<std::size_t>(i)].label,
                       Vec2{36.0f, y}, 16, Colors::Black);
            y += 22.0f;
        }
    } else {
        r.DrawText(d.CurrentLine(), Vec2{36.0f, 336.0f}, 16, Colors::Black);
    }
}

} // namespace nccu
