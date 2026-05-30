#include "game/controller/screens/InventoryScreen.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/InventoryPaging.h"  // kInventoryRowsPerPage
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include <algorithm>
#include <vector>

/**
 * @file InventoryScreen.cpp
 * @brief 背包畫面的輸入處理：Tab 開關背包，開啟期間凍結模擬，並以方向鍵移動／翻頁、
 *        以 E/Enter 使用消耗品。
 */

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
                // ←／→ 一次跳「整頁」（列數超過 kInventoryRowsPerPage 後 View 會把背包
                // 分頁）。頁碼是在繪製端「由游標導出」的，故只需把游標移動 ±一頁即可——
                // 顯示的頁面會跟著走。↑／↓ 在選取跨越頁界時本就會翻頁；←／→ 則是明確的
                // 快速捷徑。夾限（不繞回），使跳頁不會越過頭尾。不新增任何序列化狀態——
                // InventoryCursor 不在 state.jsonl 中。
                if (Input::IsPressed(Key::Right))
                    cur = std::min(n - 1, cur + nccu::kInventoryRowsPerPage);
                if (Input::IsPressed(Key::Left))
                    cur = std::max(0, cur - nccu::kInventoryRowsPerPage);
                world.SetInventoryCursor(cur);
                if (Input::IsPressed(Key::E) || Input::IsPressed(Key::Enter)) {
                    const InventoryRow& sel = rows[static_cast<std::size_t>(cur)];
                    if (sel.usable && IsUsableConsumable(sel.itemId) &&
                        invP->ConsumableCount(sel.itemId) > 0) {
                        // 先套用效果，「再」扣減一份。此處先後其實不影響任何結果（效果不
                        // 讀取數量），但把扣減放在後面，能讓「使用 → 它就沒了」這個語意一目
                        // 了然。ApplyConsumableEffect 會發布與拾取路徑相同的風味 ShowMessage。
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
    return false;   // 背包關閉——往下穿透
}

} // namespace nccu
