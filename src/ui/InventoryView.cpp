#include "ui/InventoryView.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/quest/ItemCatalog.h"   // kItem* category sentinels
#include "game/dialog/DialogLayout.h" // WrapToCells — CJK-aware description wrap

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

// ---- U2-T1: paging window math (pure, header-declared, unit-tested) ------
int InventoryPageCount(int rowCount) noexcept {
    if (rowCount <= 0) return 1;                 // empty bag is "1 / 1"
    return (rowCount + kInventoryRowsPerPage - 1) / kInventoryRowsPerPage;
}

int InventoryPageOf(int cursor, int rowCount) noexcept {
    if (rowCount <= 0) return 0;
    if (cursor < 0) cursor = 0;
    if (cursor >= rowCount) cursor = rowCount - 1;
    return cursor / kInventoryRowsPerPage;
}

namespace {

// T6 / U2-T3: a bag row's CATEGORY, derived purely from its itemId
// (presentation only — no gameplay logic, the DTO already carries the id).
// Drives a left-edge swatch + the row colour so 金幣 / 雨傘 / 任務紙張 /
// 食物道具 read as a different KIND than the usable consumables. U2-T3 adds
// Food for the Ch3 物物交換鏈 carried items (香腸 / 大聲公) so they no longer
// borrow the teal potion-flask swatch (which implied "usable") — they are
// view-only (usable=false), so they MUST NOT read as a usable consumable.
enum class RowKind { Consumable, Money, Umbrella, Paper, Food };

RowKind KindOf(const InventoryRow& row) {
    if (row.itemId == kItemMoney) return RowKind::Money;
    if (row.itemId == kItemForm || row.itemId == kItemNotes)
        return RowKind::Paper;
    // U2-T3: the Ch3 carried trade items are view-only 道具, not consumables.
    if (row.itemId == kItemSausage || row.itemId == kItemLoudspeaker)
        return RowKind::Food;
    // Any umbrella sentinel (or a vendor umbrella id) → Umbrella. Shares
    // ItemCatalog::IsUmbrellaItemId (the SAME predicate BuildInventoryRows
    // uses to exclude umbrellas from the count loop) so the two can't drift.
    if (IsUmbrellaItemId(row.itemId))
        return RowKind::Umbrella;
    return RowKind::Consumable;        // a held, usable consumable
}

// U2-T3: the umbrella look for an umbrella row, so the bag swatch matches
// the in-world / ending glyph (the same shared gfx::DrawUmbrellaGlyph).
// Previously only cursed/ugly were mapped and EVERYTHING else fell to
// TrueBlue — so the 破傘 / 陷阱傘 the player can now hold drew the wrong
// (intact blue) canopy. Map every held-umbrella sentinel to its true look:
//   fragile  → FragileBroken (破傘, only handle/ribs)
//   proftrap → ProfessorTrap (陷阱傘, danger red)
//   cursed   → CursedPurple,  ugly → UglyGreen
//   true / loaner (管理員的傘, a plain loaner) / victim (苦主的透明傘) → TrueBlue
UmbrellaLook UmbrellaLookOf(const InventoryRow& row) {
    if (row.itemId == kItemCursedUmbrella || row.itemId == "CursedUmbrella")
        return UmbrellaLook::CursedPurple;
    if (row.itemId == kItemUglyUmbrella || row.itemId == "UglyUmbrella")
        return UmbrellaLook::UglyGreen;
    if (row.itemId == kItemFragileUmbrella)
        return UmbrellaLook::FragileBroken;     // 破傘
    if (row.itemId == kItemProfTrapUmbrella)
        return UmbrellaLook::ProfessorTrap;     // 陷阱傘
    // True + the 管理員 loaner + the carried 苦主 transparent umbrella all
    // read as the clean blue canopy.
    return UmbrellaLook::TrueBlue;
}

// Row text colour by kind: usable consumables stay gold (you can DO
// something), the view-only categories get their own muted tints so the
// player can tell a 金幣 / 傘 / 紙 / 道具 row from an actionable one at a
// glance.
Color RowColor(RowKind k) {
    switch (k) {
        case RowKind::Money:      return Color{255, 205, 90, 255};   // coin gold
        case RowKind::Umbrella:   return Color{150, 200, 255, 255};  // soft blue
        case RowKind::Paper:      return Color{225, 225, 230, 255};  // paper white
        case RowKind::Food:       return Color{255, 190, 120, 255};  // warm tan
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
        case RowKind::Food: {
            // U2-T3: a distinct food/道具 token — a warm orange parcel with a
            // darker tie band — so the Ch3 香腸 / 大聲公 read as carried
            // 道具, NOT the teal potion-flask of a usable consumable. Rect-
            // only (the architecture rule), inset to read as a wrapped item.
            r.DrawRect(Rect{box.x + box.width * 0.18f, box.y + box.height * 0.22f,
                            box.width * 0.64f, box.height * 0.58f},
                       Color{225, 140, 55, 255});          // parcel body
            r.DrawRect(Rect{box.x + box.width * 0.44f, box.y + box.height * 0.10f,
                            box.width * 0.12f, box.height * 0.80f},
                       Color{150, 85, 30, 255});           // tie band
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

    // U2-T2: a BIGGER box. Was 400x312 with a 96px desc band that the
    // longer post-G4 effect lines overflowed; now 468 wide (more room for
    // a wrapped row + the description) and 372 tall, sized so a fixed
    // kInventoryRowsPerPage rows sit at a comfortable 26px pitch AND the
    // description band below holds up to kDescLines WRAPPED lines fully
    // inside the border.
    const float pw = 468.0f;
    const float ph = 372.0f;
    const float px = screenW * 0.5f - pw * 0.5f;
    const float py = screenH * 0.5f - ph * 0.5f;
    r.DrawRect(Rect{px, py, pw, ph}, Color{30, 30, 40, 235});
    // A gold title underline so the panel reads as a framed window.
    r.DrawRect(Rect{px + 16.0f, py + 44.0f, pw - 32.0f, 2.0f},
               Color{255, 200, 70, 220});

    r.DrawText("物品欄", Vec2{px + 16.0f, py + 12.0f}, 24, Colors::White);

    const float listTop = py + 58.0f;
    if (rows.empty()) {
        r.DrawText("（空）", Vec2{px + 16.0f, listTop}, 18,
                   Color{200, 200, 200, 255});
        return;
    }

    const int rowCount = static_cast<int>(rows.size());
    // Clamp the highlighted index into range so an out-of-bounds cursor
    // (e.g. after a row was used and the list shrank) still selects a real
    // row for the description panel.
    const int sel = std::min(std::max(cursor, 0), rowCount - 1);

    // ---- U2-T1: paging. Show only the cursor's page so the selected row
    // is ALWAYS visible and a big bag never crushes the rows / overflows
    // the box. The page index is DERIVED from the cursor (no retained UI
    // state, nothing serialized — state.jsonl stays byte-identical).
    const int pageCount = InventoryPageCount(rowCount);
    const int page      = InventoryPageOf(sel, rowCount);
    const int first     = page * kInventoryRowsPerPage;
    const int last      = std::min(rowCount, first + kInventoryRowsPerPage);

    // Geometry bands: rows in the upper band at a fixed comfortable pitch,
    // then a page-indicator strip, then the description band at the foot.
    constexpr float kRowH     = 26.0f;
    constexpr float kSwatchSz = 20.0f;
    constexpr int   kDescLines = 3;          // wrapped description rows shown
    constexpr float kDescLineH = 18.0f;

    // One drawn row per VISIBLE bag row: a left-edge category swatch, a
    // "> " caret on the selected row, the 中文 name, and an "xN" suffix when
    // count>0. The selected row gets a highlight bar behind it so the cursor
    // is unmistakable. Index i is the absolute row; slot is its on-page row.
    for (int i = first; i < last; ++i) {
        const InventoryRow& row = rows[static_cast<std::size_t>(i)];
        const bool isSel = (i == sel);
        const RowKind kind = KindOf(row);
        const float rowY = listTop + static_cast<float>(i - first) * kRowH;

        if (isSel)
            r.DrawRect(Rect{px + 8.0f, rowY - 2.0f, pw - 16.0f, kRowH - 2.0f},
                       Color{70, 60, 25, 235});            // selection bar

        // Category swatch (the umbrella row draws its actual look).
        DrawSwatch(r, row, kind,
                   Rect{px + 14.0f, rowY + (kRowH - kSwatchSz) * 0.5f - 1.0f,
                        kSwatchSz, kSwatchSz});

        std::string line = (isSel ? "> " : "  ") + row.name;
        if (row.count > 0) line += " x" + std::to_string(row.count);
        // Selected row reads bright orange; otherwise the per-category tint
        // so 金幣 / 雨傘 / 任務紙張 / 道具 are visually distinct from usable
        // items.
        const Color c = isSel ? Color{255, 170, 40, 255} : RowColor(kind);
        r.DrawText(line, Vec2{px + 14.0f + kSwatchSz + 8.0f, rowY + 2.0f},
                   18, c);
    }

    // U2-T1: 「第 N／M 頁」 page indicator — right-aligned-ish on its own
    // strip just under the row band, plus a ←/→翻頁 hint when >1 page so
    // the navigation is discoverable. Drawn for every bag (single page reads
    // "第 1／1 頁") so the affordance is consistent.
    const float pageY = listTop + kInventoryRowsPerPage * kRowH + 4.0f;
    {
        std::string ind = "第 " + std::to_string(page + 1) + "／" +
                          std::to_string(pageCount) + " 頁";
        if (pageCount > 1) ind += "   ←／→ 翻頁";
        r.DrawText(ind, Vec2{px + 16.0f, pageY}, 14, Color{200, 200, 210, 255});
    }

    // ---- Description band for the selected row — a faint divider, the row
    // name, then the catalog line WRAPPED so it never spills past the box
    // border (U2-T2), plus a usage hint. Sits at the panel foot, below the
    // row band + page strip, so it never overlaps them.
    const InventoryRow& cur = rows[static_cast<std::size_t>(sel)];
    const float descTop = pageY + 24.0f;
    r.DrawRect(Rect{px + 12.0f, descTop - 8.0f, pw - 24.0f, 1.0f},
               Color{120, 120, 140, 200});
    r.DrawText(cur.name, Vec2{px + 16.0f, descTop}, 18, Colors::Gold);

    // U2-T2: wrap the description to the box's inner CELL width (East-Asian-
    // Width aware, the project's single source of truth nccu::dialog::
    // WrapToCells — full-width CJK = 2 cells). The text runs from px+16 to
    // ~px+pw-16 (≈436px inner); at font size 14 a full-width glyph advances
    // ~14 + 14/10 ≈ 15.4px ≈ 7.7px/cell, so ≈436/7.7 ≈ 56 cells fit a row.
    // Use 54 for a safety margin against the box's right border. Show up to
    // kDescLines wrapped rows; the densest catalog line (the 防水噴霧
    // 「使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響業力。」 ≈ 48 cells)
    // fits in one row, so kDescLines=3 leaves ample headroom.
    if (!cur.description.empty()) {
        constexpr int kDescCells = 54;
        const std::vector<std::string> wrapped =
            nccu::dialog::WrapToCells(cur.description, kDescCells);
        const int shown =
            std::min(static_cast<int>(wrapped.size()), kDescLines);
        for (int i = 0; i < shown; ++i)
            r.DrawText(wrapped[static_cast<std::size_t>(i)],
                       Vec2{px + 16.0f,
                            descTop + 24.0f + static_cast<float>(i) * kDescLineH},
                       14, Color{210, 210, 215, 255});
    }
    r.DrawText(cur.usable ? "↑↓ 選擇   E 使用" : "↑↓ 選擇",
               Vec2{px + 16.0f,
                    descTop + 24.0f + static_cast<float>(kDescLines) * kDescLineH},
               14, Color{185, 185, 190, 255});
}

} // namespace nccu
