#ifndef GAME_QUEST_INVENTORY_PAGING_H_
#define GAME_QUEST_INVENTORY_PAGING_H_

namespace nccu {

// Blueprint Phase 4 — pure game-domain paging constant for the Tab
// bag. Moved out of ui/InventoryView.h so the game-layer controller
// (game/controller/screens/InventoryScreen.cpp) can read it without
// pulling the ui/ render header — closes a game→ui back-edge while
// keeping the value the single source of truth ui/InventoryView still
// consumes (its View.cpp paging math + the page-jump action both
// reach for the same kInventoryRowsPerPage).
//
// U2-T1: fixed rows-per-page window for the Tab bag. When the row
// count exceeds this, the panel PAGES instead of squeezing every row
// into a cramped sub-28px band (which also made the description
// overflow). The page shown is always the one containing the cursor
// (derived in ui/InventoryView via InventoryPageOf), so the selected
// row is never off-screen; an explicit ←/→ page-jump in the inventory
// scene moves the cursor by ±this.
inline constexpr int kInventoryRowsPerPage = 6;

} // namespace nccu

#endif // GAME_QUEST_INVENTORY_PAGING_H_
