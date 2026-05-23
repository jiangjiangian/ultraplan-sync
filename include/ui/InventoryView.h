#ifndef INVENTORY_VIEW_H_
#define INVENTORY_VIEW_H_
#include "quest/ItemCatalog.h"   // nccu::InventoryRow DTO
#include <vector>

namespace nccu {
namespace gfx { class IRenderer; }

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
void DrawInventory(nccu::gfx::IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH);

} // namespace nccu

#endif // INVENTORY_VIEW_H_
