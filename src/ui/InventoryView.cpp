#include "ui/InventoryView.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include "gfx/UmbrellaGlyph.h"
#include "quest/ItemCatalog.h"   // kItem* category sentinels

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

namespace {

// T6: a bag row's CATEGORY, derived purely from its itemId (presentation
// only — no gameplay logic, the DTO already carries the id). Drives a
// left-edge swatch + the row colour so 金幣 / 雨傘 / 任務紙張 read as a
// different KIND of thing than the usable consumables.
enum class RowKind { Consumable, Money, Umbrella, Paper };

RowKind KindOf(const InventoryRow& row) {
    if (row.itemId == kItemMoney) return RowKind::Money;
    if (row.itemId == kItemForm || row.itemId == kItemNotes)
        return RowKind::Paper;
    // Any umbrella sentinel (or a vendor umbrella id) → Umbrella.
    if (row.itemId.find("umbrella") != std::string::npos ||
        row.itemId.find("Umbrella") != std::string::npos)
        return RowKind::Umbrella;
    return RowKind::Consumable;        // a held, usable consumable
}

// The umbrella look for an umbrella row, so the bag swatch matches the
// in-world / ending glyph (the same shared gfx::DrawUmbrellaGlyph).
UmbrellaLook UmbrellaLookOf(const InventoryRow& row) {
    if (row.itemId == kItemCursedUmbrella || row.itemId == "CursedUmbrella")
        return UmbrellaLook::CursedPurple;
    if (row.itemId == kItemUglyUmbrella || row.itemId == "UglyUmbrella")
        return UmbrellaLook::UglyGreen;
    // True umbrella + the carried 苦主 transparent umbrella both read blue.
    return UmbrellaLook::TrueBlue;
}

// Row text colour by kind: usable consumables stay gold (you can DO
// something), the three view-only categories get their own muted tints so
// the player can tell a 金幣 / 傘 / 紙 row from an actionable one at a glance.
Color RowColor(RowKind k) {
    switch (k) {
        case RowKind::Money:      return Color{255, 205, 90, 255};   // coin gold
        case RowKind::Umbrella:   return Color{150, 200, 255, 255};  // soft blue
        case RowKind::Paper:      return Color{225, 225, 230, 255};  // paper white
        case RowKind::Consumable: return Colors::Gold;
    }
    return Colors::White;
}

// Draw the small left-edge category swatch in `box`.
void DrawSwatch(IRenderer& r, const InventoryRow& row, RowKind k, Rect box) {
    namespace C = Colors;
    switch (k) {
        case RowKind::Umbrella:
            DrawUmbrellaGlyph(r, UmbrellaLookOf(row), box);
            break;
        case RowKind::Money: {
            // A round-ish gold coin token (rect-only, inset to read circular).
            r.DrawRect(Rect{box.x + box.width * 0.18f, box.y + box.height * 0.08f,
                            box.width * 0.64f, box.height * 0.84f}, C::Gold);
            r.DrawRect(Rect{box.x + box.width * 0.36f, box.y + box.height * 0.30f,
                            box.width * 0.28f, box.height * 0.40f},
                       Color{200, 150, 0, 255});           // engraved centre
            break;
        }
        case RowKind::Paper: {
            // A white sheet with a folded corner — matches the world pickup.
            r.DrawRect(Rect{box.x + box.width * 0.20f, box.y + box.height * 0.10f,
                            box.width * 0.60f, box.height * 0.80f}, C::White);
            r.DrawRect(Rect{box.x + box.width * 0.58f, box.y + box.height * 0.10f,
                            box.width * 0.22f, box.height * 0.22f}, C::DarkGray);
            break;
        }
        case RowKind::Consumable: {
            // A little potion/flask: a teal body so a usable item reads apart.
            r.DrawRect(Rect{box.x + box.width * 0.32f, box.y + box.height * 0.05f,
                            box.width * 0.36f, box.height * 0.18f},
                       Color{180, 180, 185, 255});         // cap
            r.DrawRect(Rect{box.x + box.width * 0.24f, box.y + box.height * 0.23f,
                            box.width * 0.52f, box.height * 0.70f},
                       Color{60, 200, 180, 255});          // flask body
            break;
        }
    }
}

}  // namespace

void DrawInventory(IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH) {
    // Dim the (frozen) world, then a centred panel — same overlay idiom
    // as the dialog box / ending card.
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, 140});

    const float pw = 400.0f;
    const float ph = 312.0f;
    const float px = screenW * 0.5f - pw * 0.5f;
    const float py = screenH * 0.5f - ph * 0.5f;
    r.DrawRect(Rect{px, py, pw, ph}, Color{30, 30, 40, 235});
    // A gold title underline so the panel reads as a framed window.
    r.DrawRect(Rect{px + 16.0f, py + 44.0f, pw - 32.0f, 2.0f},
               Color{255, 200, 70, 220});

    r.DrawText("物品欄", Vec2{px + 16.0f, py + 12.0f}, 24, Colors::White);

    float y = py + 58.0f;
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

    // The row list sits in the upper band; the description panel below gets
    // a fixed reserved height, so the rows lay out in the remaining space.
    const float descH    = 96.0f;
    const float listTop  = y;
    const float listBot  = py + ph - descH;
    const float rowH     = std::min(
        28.0f, (listBot - listTop) / static_cast<float>(rows.size()));
    const float swatchSz = std::min(20.0f, rowH - 6.0f);

    // One row each: a left-edge category swatch, a "> " caret on the selected
    // row, the 中文 name, and an "xN" suffix when count>0. The selected row
    // gets a highlight bar behind it so the cursor is unmistakable.
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const InventoryRow& row = rows[i];
        const bool isSel = (static_cast<int>(i) == sel);
        const RowKind kind = KindOf(row);
        const float rowY = listTop + static_cast<float>(i) * rowH;

        if (isSel)
            r.DrawRect(Rect{px + 8.0f, rowY - 2.0f, pw - 16.0f, rowH - 2.0f},
                       Color{70, 60, 25, 235});            // selection bar

        // Category swatch (the umbrella row draws its actual look).
        DrawSwatch(r, row, kind,
                   Rect{px + 14.0f, rowY + (rowH - swatchSz) * 0.5f - 1.0f,
                        swatchSz, swatchSz});

        std::string line = (isSel ? "> " : "  ") + row.name;
        if (row.count > 0) line += " x" + std::to_string(row.count);
        // Selected row reads bright orange; otherwise the per-category tint
        // so 金幣 / 雨傘 / 任務紙張 are visually distinct from usable items.
        const Color c = isSel ? Color{255, 170, 40, 255} : RowColor(kind);
        r.DrawText(line, Vec2{px + 14.0f + swatchSz + 8.0f, rowY + 2.0f},
                   18, c);
    }

    // Description panel for the selected row — a faint divider then the
    // catalog line, plus a usage hint when the row is usable. Sits in the
    // lower band of the panel so it never overlaps the row list above.
    const InventoryRow& cur = rows[static_cast<std::size_t>(sel)];
    const float descY = py + ph - 80.0f;
    r.DrawRect(Rect{px + 12.0f, descY - 10.0f, pw - 24.0f, 1.0f},
               Color{120, 120, 140, 200});
    r.DrawText(cur.name, Vec2{px + 16.0f, descY}, 18, Colors::Gold);
    if (!cur.description.empty())
        r.DrawText(cur.description, Vec2{px + 16.0f, descY + 24.0f}, 14,
                   Color{210, 210, 215, 255});
    r.DrawText(cur.usable ? "↑↓ 選擇   E 使用" : "↑↓ 選擇",
               Vec2{px + 16.0f, descY + 50.0f}, 14,
               Color{185, 185, 190, 255});
}

} // namespace nccu
