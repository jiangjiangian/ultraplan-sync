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
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出，以此引入

bool HandleInventory(EventBus& bus, World& world) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    if (Input::IsPressed(Key::Tab))
        world.SetInventoryOpen(!world.InventoryOpen());
    if (world.InventoryOpen()) {
        // 背包是「持有並使用」清單。↑/↓ 移動游標；在消耗品列按 E/Enter 即使用它
        // （套用與拾取相同的效果，再扣減數量）；在僅供檢視的列（金幣／雨傘／任務
        // 紙張）按 E/Enter 無作用——View 已顯示該列說明。背包開啟期間每幀自玩家
        // 重建各列（BuildInventoryRows），故某列使用後歸 0 即於下一幀消失並重新夾限
        // 游標。一般移動／互動絕不在此執行（下方提早返回會凍結模擬），故開背包不會
        // 移動玩家或重新觸發 NPC——只有 Tab（上方處理）能再關閉它。各列只建構一次；
        // 對擷取下來的快照操作。
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
