#include "game/controller/screens/DialogScreen.h"
#include "game/controller/DialogChoiceApply.h"   // ApplyDialogChoice
#include "game/controller/InputHandler.h"
#include "game/controller/SceneRouter.h"
#include "game/controller/VendorMenu.h"         // kVendorContext
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"            // kDialogExitLabel
#include "game/quest/ChapterGate.h"
#include "game/quest/Chapter1Quest.h"            // TryBuyAuntieUglyUmbrella
#include "game/quest/Chapter4Quest.h"            // TryGrantTaFinaleUmbrella
#include "game/quest/Flags.h"
#include "game/state/EndingGate.h"
#include "game/state/SemesterStateMachine.h"
#include "game/state/SemesterState.h"
#include "game/vendor/Vendor.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include <cstddef>
#include <string>

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出

bool HandleDialog(EventBus& bus, World& world, Vendor*& pendingVendor,
                  InputHandler& input, SceneRouter& sceneRouter) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    using nccu::engine::platform::Time;
    DialogState& dlg = world.Dialog();
    // 一旦攤販選單不再是開啟中的對話（只剩招呼語就關閉、無庫存而被翻過、被某 NPC 對話
    // 取代、被章節轉場清掃），立刻丟棄待處理的攤販目標。使這個非擁有的 Vendor* 絕不會
    // 比它所指向的 World 物件活得久——一經清空，確認分支的 `pendingVendor && ...` 防護
    // 即失效。
    if (pendingVendor &&
        (!dlg.Active() || dlg.NpcId() != kVendorContext))
        pendingVendor = nullptr;
    if (dlg.Active()) {
        Player* p = world.GetPlayer();
        if (dlg.AtChoice()) {
            if (Input::IsPressed(Key::Up))   dlg.MoveChoice(-1);
            if (Input::IsPressed(Key::Down)) dlg.MoveChoice(1);
        }
        // 按住 E 可快速推進對話。點按（邊緣 IsPressed）維持其既有的「每次按下推進一次」
        // 語意；而「按住」E 達 300 ms 以上後，也會每約 4 幀觸發同一個推進分支，使話多的
        // NPC 能不費手指地略讀。玩法用的 E 探測（位於此對話分支之外）僅靠邊緣且未受更動
        // ——按住 E 只會自動推進對話，絕不會重新觸發互動／攤販選單。
        //
        // 邊緣／按住的時機判斷現位於 InputHandler::TickDialogAdvance——與其完全相同的
        //「邊緣，或按住 ≥300 ms + 4 幀冷卻」約定，由 test_input_handler 固定。控制器只
        // 詢問「本幀是否應推進？」並據以動作。
        const float ddt = nccu::engine::platform::Time::DeltaSeconds();
        const bool advanceE = input.TickDialogAdvance(ddt);
        if (advanceE) {
            // 在 Advance()「之前」擷取 npc——確認最後一個選項可能會 Close() 對話，進而
            // 清掉 NpcId()。
            const std::string npc = dlg.NpcId();
            // 在 Advance()「之前」擷取高亮的庫存索引——確認會重置 choiceCursor_
            //（關閉／後續台詞）。
            const bool atChoice = dlg.AtChoice();
            const std::size_t stockIdx =
                static_cast<std::size_t>(dlg.ChoiceCursor());
            if (const DialogChoice* c = dlg.Advance(); c && p) {
                // 已確認的攤販庫存品項會驅動真正的購買。「所有」經濟副作用（DeductMoney、
                // AddConsumable、ShowMessage + PickupAcquired 事件、金錢軟上限、
                // item.setsFlag → 例如結局 C 的 Flag_BoughtUglyUmbrella）都留在
                // Vendor::TryBuy 內，完全如固定的 test_vendor 約定所斷言——DialogChoice
                // 本身不帶業力／旗標，故下方泛用的 ApplyDialogChoice 對它是空操作。第二章
                // 的能量飲料也經這同一條 TryBuy → AddConsumable 路徑進入計數型物品欄，使
                // TryRescueBookworm 的 ConsumeOne("EnergyDrink") 如今能在引擎內成功
                //（Flag_Ch2Cleared 變得可達）。
                if (npc == kVendorContext && pendingVendor &&
                    atChoice) {
                    // 最後一個選項永遠是「不買」（OpenVendorMenu 在每個庫存品項之後附上
                    // 它），故其索引恰等於庫存數量。選它「不得」購買——關閉對話並丟棄待處理
                    // 攤販，且「零」經濟變動（不 DeductMoney、不 AddConsumable、無
                    // item.setsFlag、無購買事件）。只有真正的庫存索引（< 庫存數量）才會
                    // 進入 Vendor::TryBuy，其固定的副作用約定未變。
                    const std::size_t stockN =
                        pendingVendor->Config().stock.size();
                    if (stockIdx >= stockN) {        // 不買
                        pendingVendor = nullptr;
                        dlg.Close();
                        // 名冊整理會接住任何 FSM 轉場（此處預期沒有，但作為對 Close() 內
                        // 未來副作用的廉價保險）。僅供顯示——不改寫 player.pos／旗標／
                        // 事件，故 harness 存檔可觀察時間線不變。
                        sceneRouter.SettleRoster(world);
                        return true;
                    }
                    (void)pendingVendor->TryBuy(p, stockIdx);
                    pendingVendor = nullptr;
                    // 不在此解算結局。已確認的庫存品項沒有後續台詞，故攤販對話框此刻已
                    //「關閉」——現在呼叫 CheckEndingGates 會在買下醜傘的同一幀就切到結局 C、
                    // 毫無收尾節拍（即作者抱怨的突兀結局）。改由非對話輪詢（Update 末端）
                    // 先跑 TryOpenEndingConfession → 開啟務實自白 → CheckEndingGates 延後
                    // 於其後 → 待玩家關閉獨白後 C 才觸發。保留 CheckChapterGates（購買絕非
                    // 章節通關觸發源，故與先前一樣是廉價的空操作保險）。
                    CheckChapterGates(bus, *p, world.Semester(), dlg);
                    // 上方閘門呼叫可能 Transition()——「現在」就整理名冊，使玩家所見的
                    // 那幀有一致的 npcs[]。修正前，重生要等到「下」一幀 Update 最前端的
                    // 檢查。僅供顯示：SettleSideEffects（玩家位置、消耗品、事件）仍在「下」
                    // 一次 Update 最前端執行，故 harness 存檔逐位元一致。
                    sceneRouter.SettleRoster(world);
                    return true;
                }
                ApplyDialogChoice(*p, *c);
                // 結尾的「我再想想…」退出（kDialogExitLabel）是不做承諾的退出——它「不得」
                // 觸發下方任何選單的選後記帳。ApplyDialogChoice 對它本已是空操作（零業力／
                // 空旗標）；此處的防護額外阻止助教終局的自我上鎖，使該道德選擇維持「未做」
                // 且可再次接洽（與攤販不買的「無副作用」約定一致）。
                const bool exitChoice = c->label == kDialogExitLabel;
                // 已確認的西裝學長選項會鎖住分支選單，使重新對話無法堆疊互斥的漣漪旗標。
                // DialogOpener 會讀取此旗標，其後只做台詞回顧。
                if (!exitChoice && npc == "suit_senior")
                    p->SetFlag(kFlagSuitSeniorChoiceMade);
                // 第四章中已確認的助教 (d) 結算 選項會鎖住選單（一次性，與西裝學長同理），
                // 使該道德選擇（體諒 → Flag_ConsoledTA，+15）無法在重新對話時被翻轉／
                // 重複套用。ApplyDialogChoice 已設下 Flag_ConsoledTA + 業力；下方的
                // CheckEndingGates 會在其 業力>80 + TrueUmbrella 條件也成立時路由至結局 A。
                // 排除「我再想想…」退出，使退出「不」設下 Flag_TaFinaleChoiceMade（避免
                // 過早進入結局 C／B）。
                if (!exitChoice && npc == "ta" &&
                    world.Semester().Current() ==
                        SemesterState::Chapter4_Finals) {
                    p->SetFlag(kFlagTaFinaleChoiceMade);
                    // 溫柔的終局會把「你的」傘還給你。當玩家選擇體諒（ApplyDialogChoice
                    // 剛設下 Flag_ConsoledTA）時，助教把真傘塞回——TryGrantTaFinaleUmbrella
                    // 設下 Flag_HasTrueUmbrella + HasUmbrella，使溫柔路線「無需」另外找到
                    // 隱藏的第四章雨傘也能抵達結局 A（兩條路線如今皆可達 A；EndingGate 維持
                    // 業力>80 的閘）。嚴厲的質問分支永不設下 Flag_ConsoledTA，故此輔助函式
                    // 空操作、該路線解算為結局 B（冷淡終局）。口白「拿回你的傘」這一節拍位於
                    // 體諒選項的後續台詞中（見 DialogOpener）。
                    TryGrantTaFinaleUmbrella(
                        *p, npc, world.Semester().Current());
                }
                // 第一章福利社阿姨 (c) 購買醜綠傘 是一筆「真正」的購買。DialogChoice 本身
                // 不帶金錢／雨傘（維持為純粹的選項開場條目——setsFlag ""、業力 0，使既有的
                // test_dialog_opener 斷言成立）；經濟在「此處」落地，由 npc + 所選標籤歸因，
                // 與助教終局在確認時授予真傘的方式一致。扣 80 元 + 授予持有型醜傘並附花費／
                // 餘額提示；「不」設下 Flag_BoughtUglyUmbrella（那是第四章攤販的結局 C 鎖）。
                // 對阿姨的其他選項與「我再想想…」退出皆為空操作（標籤不符）。具冪等性（已
                // 持有醜傘 → 不重複扣款），且內部有資金防護。
                if (!exitChoice)
                    (void)TryBuyAuntieUglyUmbrella(
                        bus, *p, npc, c->label, world.Semester().Current());
                // 先結局閘門，再章節閘門（既有慣例：EndingGate 早於此）。兩種順序皆安全
                // ——一旦結局觸發，Current() 即為 Ending_X，CheckChapterGates 的第二／
                // 三章／插曲段兄弟 if 皆無法相符。
                CheckEndingGates(bus, *p, world.Semester(), dlg);
                CheckChapterGates(bus, *p, world.Semester(), dlg);
            }
        }
        // 對話分支產生的任何轉場（CheckEndingGates／CheckChapterGates）都必須在本幀繪製
        //「之前」把名冊併入 View。上方的提前返回路徑已各自呼叫過 SettleRoster；此處涵蓋
        // 落空路徑（非購買、非終端的推進）。僅供顯示：harness 可觀察的副作用（玩家位置、
        // 消耗品、事件）等到下一次 Update 最前端的 SettleSideEffects，故存檔逐位元一致。
        sceneRouter.SettleRoster(world);
        return true;
    } else {
        // 本 tick 對話未啟用：丟棄任何殘留的按住 E 累積量，使下一段對話從頭開始。廉價且
        // 冪等——InputHandler 自身會追蹤 IsDown(E)、放開時本就會重置，但此處明確丟棄使
        // 約定一目了然。
        input.ResetDialogAdvance();
    }
    return false;   // 無進行中的對話——落空
}

} // namespace nccu
