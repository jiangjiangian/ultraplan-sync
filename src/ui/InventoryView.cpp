#include "ui/InventoryView.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"

#include <algorithm>
#include <string>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

void DrawInventory(IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH) {
    // Dim the (frozen) world, then a centred panel — same overlay idiom
    // as the dialog box / ending card.
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, 140});

    const float pw = 380.0f;
    const float ph = 300.0f;
    const float px = screenW * 0.5f - pw * 0.5f;
    const float py = screenH * 0.5f - ph * 0.5f;
    r.DrawRect(Rect{px, py, pw, ph}, Color{30, 30, 40, 235});

    r.DrawText("物品欄", Vec2{px + 16.0f, py + 14.0f}, 24, Colors::White);

    float y = py + 56.0f;
    if (rows.empty()) {
        r.DrawText("（空）", Vec2{px + 16.0f, y}, 18,
                   Color{200, 200, 200, 255});
        return;
    }

    // Clamp the highlighted index into range so an out-of-bounds cursor
    // (e.g. after a row was used and the list shrank) still selects a real
    // row for the description panel.
    const int sel = rows.empty() ? 0
        : std::min(std::max(cursor, 0),
                   static_cast<int>(rows.size()) - 1);

    // One line per row: a "> " caret on the selected row, the 中文 name,
    // and an "xN" suffix when count>0 (single-instance rows omit it).
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const InventoryRow& row = rows[i];
        const bool isSel = (static_cast<int>(i) == sel);
        std::string line = (isSel ? "> " : "  ") + row.name;
        if (row.count > 0) line += " x" + std::to_string(row.count);
        // Usable rows read gold (you can do something with them); view-
        // only rows white. The selected row is highlighted orange so the
        // keyboard cursor is unambiguous.
        const Color c = isSel ? Color{255, 153, 0, 255}
                       : row.usable ? Colors::Gold
                                    : Colors::White;
        r.DrawText(line, Vec2{px + 16.0f, y}, 18, c);
        y += 26.0f;
    }

    // Description panel for the selected row — a faint divider then the
    // catalog line, plus a usage hint when the row is usable. Sits in the
    // lower band of the panel so it never overlaps the row list above.
    const InventoryRow& cur = rows[static_cast<std::size_t>(sel)];
    const float descY = py + ph - 86.0f;
    r.DrawRect(Rect{px + 12.0f, descY - 8.0f, pw - 24.0f, 1.0f},
               Color{120, 120, 140, 200});
    r.DrawText(cur.name, Vec2{px + 16.0f, descY}, 18, Colors::Gold);
    if (!cur.description.empty())
        r.DrawText(cur.description, Vec2{px + 16.0f, descY + 24.0f}, 14,
                   Color{210, 210, 215, 255});
    r.DrawText(cur.usable ? "↑↓ 選擇   E 使用" : "↑↓ 選擇",
               Vec2{px + 16.0f, descY + 50.0f}, 14,
               Color{170, 170, 175, 255});
}

} // namespace nccu
