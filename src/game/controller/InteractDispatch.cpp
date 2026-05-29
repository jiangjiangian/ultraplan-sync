#include "game/controller/InteractDispatch.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/vendor/Vendor.h"
#include "game/controller/VendorMenu.h"
#include "game/controller/GameObjectQueries.h"
#include "game/quest/NpcSpawns.h"     // 提供 IsChapter1FlavorNpc 路由判斷
#include "game/quest/QuestHookTable.h"
#include "engine/core/GameObject.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/math/Rect.h"
#include <string_view>

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出，以此引入

void DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    using nccu::engine::math::Rect;
    using nccu::queries::ForEachActiveExcept;
    Player* player = world.GetPlayer();
    if (Input::IsPressed(Key::E) && player) {
        // BlocksMovement() NPC 的「移動」碰撞體是位於 NPC 原點、玩家大小的盒，且
        // Rect::Intersects 為嚴格相交，故 physics::ResolveMove 會把玩家恰好停在靜態
        // NPC 旁（貼齊，從不嚴格重疊）。因此位於玩家原點的 24x24 E 探測盒永遠不會
        // 相交——走到 NPC 面前的玩家（自動或真人）將永遠無法開啟對話。故給 E 探測盒
        // 一個明確的互動觸及距離：四邊各外擴 kInteractReach，使貼齊受阻的玩家仍能與
        // NPC 碰撞盒重疊。「移動」碰撞體維持原樣（上方 frameColliders_ 不變）——玩家
        // 仍無法穿過 NPC，只有對話觸及範圍變大。此邊距（8 px，盒寬 24 px 的三分之一）
        // 遠小於 NPC／道具間的世界間距，故只可能觸及玩家本就貼齊站著的物件。
        // 「擴大目標」無障礙設定（World::LargeTargets()）把觸及距離放寬到每邊 16 px
        // ——有效對話盒由 40x40 變為 56x56——使手抖的玩家不必像素級對齊也能觸發 NPC
        // 對話。上方「移動」碰撞體不變（玩家仍無法穿過 NPC），只有 E 探測觸及變大。
        // 預設關閉，故與其開啟前的行為（kInteractReach = 8.0f）逐位元等價。
        const float kInteractReach = world.LargeTargets() ? 16.0f : 8.0f;
        const Rect pHit{player->GetPosition().x - kInteractReach,
                        player->GetPosition().y - kInteractReach,
                        24.0f + 2.0f * kInteractReach,
                        24.0f + 2.0f * kInteractReach};
        ForEachActiveExcept(world.Objects(), player,
            [&bus, &world, player, pHit, &pendingVendor](GameObject& o) {
                if (!o.CheckCollision(pHit)) return;
                // Vendor 的 NpcId() 為空，故若無此分支它會落到 o.Interact()
                // （NPC 逐行循環），而 Vendor::TryBuy 將無任何執行期呼叫者——Ending C
                // 與 Ch2 的 EnergyDrink 將無法達成。改把商店互動導向購買選項對話；
                // 購買本身（金錢／背包／EventBus 事件／軟上限／setsFlag）完全留在
                // Vendor::TryBuy 內，於對話分支確認時呼叫。每次 E 一個選單：本輪若已
                // 開啟對話則略過。
                if (o.IsVendor()) {
                    if (world.Dialog().Active()) return;
                    auto* vendor = static_cast<Vendor*>(&o);
                    OpenVendorMenu(world.Dialog(), *vendor);
                    pendingVendor = vendor;
                    return;
                }
                if (const std::string_view id = o.NpcId(); !id.empty()) {
                    // Ch1 的固定閒談型 NPC（搶課同學／撐傘路人／揹書包學生）透過
                    // NPC::Interact() 每次對話循環取出自己 chapter1.md 行池中的一行
                    // ——確定性、可重現的選取（狀態路徑上無亂數）。在「任何主線 hook
                    // 之前」於此處路由並 return：閒談型 NPC 絕不可到達
                    // TryReturnVictimUmbrella／OpenNpcDialog 等，故可證明它不設任何
                    // 任務旗標、也無法擾動硬性守門的「苦主→學長→苦主」主線。（它顯示
                    // 的對話是單行 ShowMessage 提示，與環境路人 Interact 同通道，
                    // 而非對話框。）
                    if (nccu::IsChapter1FlavorNpc(id)) {
                        if (auto* it = o.AsInteractable()) it->Interact(player);
                        return;
                    }
                    // 在對話開啟器「之前」，依序跑過已註冊的 Ch1-Ch4 任務 hook——正是
                    // 此處所取代的內嵌 TryXxx 序列（TryReturnVictimUmbrella ->
                    // TryRescueBookworm -> TryMeetLibrarian ->
                    // TryLendLibrarianUmbrella -> TryReturnLibrarianUmbrella ->
                    // TryApplyCh2Ripple -> TryAdvanceCh3Trade -> TryApplyCh3Ripple
                    // -> TryApplyCh4Ripple）。每個 hook 以 (state, npcId) 自我守門，
                    // 在其章節之外是廉價 no-op，故每次互動跑整張表是正確且順序穩定的。
                    // 第 4 個引數是市集返回目標（只有管理員歸還 hook 用它界定範圍）。
                    // 新增章節／NPC 現在只是一行 RegisterHook，而非在此處修改（OCP）。
                    RunInteractHooks(bus, *player, id,
                                     world.Semester().Current(),
                                     world.Semester().InterludeReturnTo());
                    OpenNpcDialog(world.Dialog(), *player, id,
                                  world.Semester().Current());     // 對話
                } else if (auto* it = o.AsInteractable()) {
                    it->Interact(player);                            // 拾取／Vendor
                }
            });
    }
}

} // namespace nccu
