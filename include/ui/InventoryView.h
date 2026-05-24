#ifndef INVENTORY_VIEW_H_
#define INVENTORY_VIEW_H_
#include "quest/ItemCatalog.h"   // nccu::InventoryRow DTO
#include <vector>

namespace nccu {
namespace gfx { class IRenderer; }

// U2-T1: fixed rows-per-page window for the Tab bag. When the row count
// exceeds this, the panel PAGES instead of squeezing every row into a
// cramped sub-28px band (which also made the description overflow). The
// page shown is always the one containing the cursor (derived, see
// InventoryPageOf), so the selected row is never off-screen; an explicit
// ←/→ page-jump in GameController moves the cursor by ±this. Exported so
// the controller's page-jump and the View's window math share ONE value.
inline constexpr int kInventoryRowsPerPage = 6;

// U2-T1: which page (0-based) the cursor sits on, and how many pages a bag
// of `rowCount` rows has, for `kInventoryRowsPerPage` rows/page. Pure
// integer math, exposed so the paging window is unit-testable without a
// renderer. Always >=1 page (an empty bag is "1 of 1"). The cursor is
// clamped into [0,rowCount) first so an out-of-range cursor still yields a
// valid page (mirrors the View's own clamp).
[[nodiscard]] int InventoryPageCount(int rowCount) noexcept;
[[nodiscard]] int InventoryPageOf(int cursor, int rowCount) noexcept;

// Tab inventory overlay (Item 2): a centred panel listing every bag row —
// 金幣, each held consumable (with count + description), the carried
// umbrella, and the current-cycle quest papers — with a movable cursor and
// a description panel for the selected row. Render-ONLY: it draws the
// supplied InventoryRow DTO + cursor index and nothing else (MVC purity,
// mirroring DrawEndingCard taking an EndingSummary). The DTO is built in
// the quest/controller layer (BuildInventoryRows) from World/Player; this
// function never touches World/Player and holds no gameplay logic. No
// raylib here — all drawing goes through the injected IRenderer, so it
// stays deterministic and headless-spy-testable. An empty `rows` paints a
// single "（空）" line.
//
// U2-T1/T2: when rows exceed kInventoryRowsPerPage the panel shows only the
// cursor's page (selected row always visible) + a 「第 N／M 頁」 indicator;
// the enlarged box + per-row description area WRAP the (post-G4, longer)
// effect text inside the border (nccu::dialog::WrapToCells), never spilling.
void DrawInventory(nccu::gfx::IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH);

} // namespace nccu

#endif // INVENTORY_VIEW_H_
