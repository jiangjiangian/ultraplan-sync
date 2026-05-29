#ifndef CONTROLLER_SCREENS_INVENTORY_SCREEN_H_
#define CONTROLLER_SCREENS_INVENTORY_SCREEN_H_

class EventBus;       // Plan P2 step 2: ApplyConsumableEffect takes bus

namespace nccu {

class World;

// Tab inventory overlay (S5b-5 + Item 2). Edge-triggered toggle, then —
// while open — freezes the sim exactly like the dialog box (the
// orchestrator returns before the pipeline / interact / sweep). Checked
// AFTER the dialog handler so a conversation has priority and Tab can't
// pop the panel mid-dialog.
//
// Item 2(b): the bag is a hold-and-use list. ↑/↓ move the cursor; E/Enter
// on a CONSUMABLE row uses it (applies the SAME effect the pickup used to
// fire, then decrements the count); on a view-only row (金幣 / 雨傘 / 任務
// 紙張) E/Enter is inert — the View already shows that row's description.
// ←/→ jump a whole page (kInventoryRowsPerPage, clamped — no wrap).
//
// Returns true while the bag is open (the world stays frozen), false
// when the bag is closed (the controller falls through).
[[nodiscard]] bool HandleInventory(EventBus& bus, World& world);

} // namespace nccu

#endif // CONTROLLER_SCREENS_INVENTORY_SCREEN_H_
