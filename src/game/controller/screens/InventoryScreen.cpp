#include "game/controller/screens/InventoryScreen.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/InventoryPaging.h"  // kInventoryRowsPerPage
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include <algorithm>
#include <vector>

namespace nccu {

bool HandleInventory(EventBus& bus, World& world) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    if (Input::IsPressed(Key::Tab))
        world.SetInventoryOpen(!world.InventoryOpen());
    if (world.InventoryOpen()) {
        // Item 2(b): the bag is a hold-and-use list. ↑/↓ move the cursor;
        // E/Enter on a CONSUMABLE row uses it (applies the SAME effect the
        // pickup used to fire, then decrements the count); on a view-only
        // row (金幣 / 雨傘 / 任務紙張) E/Enter is inert — the View already
        // shows that row's description. The rows are rebuilt from the
        // Player each frame the bag is open (BuildInventoryRows), so a row
        // that hits 0 after use disappears next frame and the cursor is
        // re-clamped. Normal movement / interact never run here (the early
        // return below freezes the sim), so opening the bag can't move the
        // player or re-trigger NPCs — only Tab (handled above) re-closes
        // it. Build the rows ONCE; act on the captured snapshot.
        if (Player* invP = world.GetPlayer()) {
            const std::vector<InventoryRow> rows = BuildInventoryRows(*invP);
            const int n = static_cast<int>(rows.size());
            int cur = world.InventoryCursor();
            if (n > 0) {
                if (cur < 0)   cur = 0;
                if (cur >= n)  cur = n - 1;
                if (Input::IsPressed(Key::Up))   cur = (cur - 1 + n) % n;
                if (Input::IsPressed(Key::Down)) cur = (cur + 1) % n;
                // U2-T1: ←/→ jump a whole PAGE (the View pages the bag once
                // rows exceed kInventoryRowsPerPage). The page index is
                // DERIVED from the cursor render-side, so moving the cursor
                // by ±a page is all that is needed — the shown page follows.
                // Up/Down already flip the page when the selection crosses a
                // boundary; ←/→ are the explicit fast path. Clamped (no
                // wrap) so a page-jump can't skip past the ends. No new
                // serialized state — InventoryCursor is not in state.jsonl.
                if (Input::IsPressed(Key::Right))
                    cur = std::min(n - 1, cur + nccu::kInventoryRowsPerPage);
                if (Input::IsPressed(Key::Left))
                    cur = std::max(0, cur - nccu::kInventoryRowsPerPage);
                world.SetInventoryCursor(cur);
                if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
                    const InventoryRow& sel = rows[static_cast<std::size_t>(cur)];
                    if (sel.usable && IsUsableConsumable(sel.itemId) &&
                        invP->ConsumableCount(sel.itemId) > 0) {
                        // Apply the effect, THEN spend one. Order matters
                        // for nothing here (the effect doesn't read the
                        // count), but spending after keeps the "use → it's
                        // gone" reading obvious. ApplyConsumableEffect
                        // publishes the same flavour ShowMessage the pickup
                        // path used to.
                        ApplyConsumableEffect(bus, *invP, sel.itemId);
                        (void)invP->ConsumeOne(sel.itemId);
                    }
                }
            } else {
                world.SetInventoryCursor(0);
            }
        }
        return true;
    }
    return false;   // bag closed — fall through
}

} // namespace nccu
