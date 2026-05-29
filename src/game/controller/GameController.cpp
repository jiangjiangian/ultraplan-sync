#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogOpener.h"
#include "game/state/EndingGate.h"
#include "game/quest/ChapterGate.h"
#include "game/state/ChapterToast.h"
#include "game/controller/screens/EndingScreen.h"    // HandleEndingMenu 自由函式
#include "game/controller/screens/PauseScreen.h"     // HandlePauseMenu 自由函式
#include "game/controller/screens/InventoryScreen.h" // HandleInventory 自由函式
#include "game/controller/InteractDispatch.h"        // DispatchInteract 自由函式
#include "game/controller/screens/DialogScreen.h"    // HandleDialog 自由函式
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/NpcSpawns.h"     // IsChapter1FlavorNpc 路由
#include "game/state/EndingMenuModel.h"  // IsEndingState + EndingMenuChoiceAt
#include "game/state/GameHelpPages.h"    // kGameHelpPageCount
#include "game/quest/InventoryPaging.h"  // kInventoryRowsPerPage
#include "game/vendor/Vendor.h"
#include "game/state/InterludeExit.h"
#include "game/controller/GameObjectQueries.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/controller/SimSystem.h"
#include "game/quest/QuestHookTable.h"
#include "game/world/Physics.h"
#include "game/world/WorldConfig.h"
#include "game/gfx/Bounds.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include <algorithm>
#include <memory>
#include <string_view>

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出

// 原本檔案內的 OpenVendorMenu + kVendorContext + kVendorDeclineLabel 已移入
// `controller/VendorMenu.{h,cpp}`。ApplyDialogChoice 已移入
// `controller/DialogChoiceApply.{h,cpp}`，使 DialogScreen.cpp 能直接取用，
// 而不必相依於 GameController.h。

GameController::GameController(World& world, EventBus& bus)
    : world_(world),
      bus_(bus),
      worldSize_{::world::kSize, ::world::kSize},
      playerSize_{::world::kPlayerWidth, ::world::kPlayerHeight},
      sceneRouter_(world.Semester().Current()) {
    frameColliders_.reserve(64);  // 僅動態角色；地形是遮罩
    // 訂閱／接線端早已收受 EventBus&；發布端從此也開始一併傳遞。自此以後，這裡登記的
    // 每個訂閱者，以及本控制器每幀可達的每個 Publish()，都改經 bus_（章節轉場這一疊
    // 最先完整整理乾淨；實體／任務的發布目前仍呼叫 Instance()，將於後續步驟串接）。
    WireDefaultSubscribers(bus_, world_.Semester(),
                           world_.CurrentBuildingName());
    // 額外的 ShowMessage 訂閱者：把事件文字鏡像進 world_ 供短暫的 HUD 橫幅使用。生命期
    // 與上方訂閱者相同——由下方 ~GameController 的 Clear() 拆除。
    WireHudMessageSubscriber(bus_, world_);
    // 業力變化經 KarmaChanged -> ShowMessage 的轉接餵給同一個 HUD 橫幅。在 HUD 訂閱者
    //「之後」接線，使登記順序符合意圖（KarmaChanged 重新發布 ShowMessage；HUD 訂閱者
    // 在下一次分派時看到產生的 ShowMessage——兩個訂閱者都存活於控制器生命期，並由同一個
    // Clear() 拆除）。
    WireKarmaToastSubscriber(bus_);

    // 把有序的模型推進管線建構一次。此處的順序「就是」每幀模擬順序——生存 -> 移動 ->
    // 碰撞 -> 生成——且必須完全對齊原本的內聯序列（逐位元一致的存檔閘已證明）。
    // SweepSystem 是終端階段，在互動／閘門邏輯之後另行執行（見 Update），故以一般成員
    // 持有。
    advanceSystems_.push_back(std::make_unique<SurvivalSystem>());
    advanceSystems_.push_back(std::make_unique<MovementSystem>());
    advanceSystems_.push_back(std::make_unique<CollisionSystem>());
    advanceSystems_.push_back(std::make_unique<SpawnSystem>());
}

GameController::~GameController() {
    // 在訂閱者所捕捉的 World 參照消亡「之前」先卸除它們——關閉靜態解構期的未定義行為
    // 窗口（否則綁定於單例的 lambda 會參照到已釋放的 currentBuildingName / semester）。
    // 使用注入的 bus，而非 Instance()。在正式環境中為同一個實例（main.cpp 傳入
    // Instance()）——此處的改動只是把相依關係明確化。
    bus_.Clear();
}

void GameController::Update() {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 名冊與副作用跟隨 FSM。任何觸發源（EndingGate、EventWiring、ChapterGate 等）
    // 都會改寫純狀態機；SceneRouter 在此於 Update 的「最前端」觀察到變化，並套用
    // harness 可觀察的副作用（玩家位置移動、消耗品清除、第四章旗標清除、抵達提示、
    // 閂鎖重置）。只供顯示用的名冊替換則前移到 Update 的「末端」（下方的
    // sceneRouter_.SettleRoster），使轉場觸發的同一幀就以新章節的 NPC 繪製——在不動到
    // harness 存檔可觀察時間線的前提下，消除 npcs[] 落後一幀的問題。
    sceneRouter_.SettleSideEffects(world_);

    // 各畫面的「輸入」處理器，依優先序排列。每個會凍結世界，並在它擁有本幀時回傳 true
    //（結局畫面 > 暫停選單 > 對話框 > Tab 物品欄）。其順序與「先凍結再回傳」的語意完全
    // 等同原本的內聯區塊——只是把主體移進各自聚焦的方法（SRP）。
    if (HandleEndingMenu()) return;
    if (HandlePauseMenu())  return;
    if (HandleDialog())     return;
    if (HandleInventory())  return;

    const float dt = nccu::engine::platform::Time::DeltaSeconds();
    // 略過提示的按鍵。當橫幅在畫面上時（HudMessage 非空「且」尚未過期），Backspace 會
    // 強制使其過期，讓已讀完該行的玩家能即時關閉，而不必等滿 4 秒 TTL。在此讀取（在上方
    // 選單／對話／物品欄的提前返回「之後」，使選單中輸入的 Backspace 無法逃出暫停；在
    // TickHud「之前」，使略過能與按下落在同一幀），使此鍵嚴格僅為 HUD 副作用——不移動、
    // 不改降雨、不發事件。預期自動更新的內容應可被玩家隱藏；這是最小的此類輸入。
    if (Input::IsPressed(Key::Backspace) && !world_.HudMessage().empty() &&
        !world_.HudExpired()) {
        world_.DismissHud();
    }
    // 讓短暫橫幅隨模擬老化。只在上方對話「與」Tab 物品欄的提前返回「之後」才到達，故對話
    // 或物品欄開啟期間它會凍結——由對話選項觸發的章節通關通知因此能存活到對話框關閉後被
    // 讀到，而不會在其後方燒掉 TTL。
    world_.TickHud(dt);

    // 有序的模型推進管線：生存（降雨）-> 移動（物件 tick + 記錄前一位置）-> 碰撞
    //（玩家 AABB + 地形）-> 生成（跑圈 + 延後生成）。MVC 純淨——每個 ISystem 只改寫
    // 模型。與內聯區塊執行的序列相同；SimContext 把 tick 前的玩家位置從移動串接到碰撞，
    // 並重用 frameColliders_ 暫存區。
    SimContext ctx{world_, worldSize_, playerSize_, frameColliders_, {}};
    for (const std::unique_ptr<ISystem>& sys : advanceSystems_)
        sys->Run(ctx, dt);

    // 按 E 互動分派（對話／拾取／開商店）。會讀取輸入，故留在控制器；任務副作用則經
    // 已登記的 QuestHook 表路由。
    DispatchInteract();

    Player* player = world_.GetPlayer();
    if (player) {
        // 偵測進入建築（僅轉場）。
        const Vec2 playerCentre{
            player->GetPosition().x + playerSize_.x * 0.5f,
            player->GetPosition().y + playerSize_.y * 0.5f
        };
        if (world_.Tracker().Update(playerCentre) == nullptr) {
            world_.CurrentBuildingName().clear();
        }
    }

    // 插曲段出口（進程主線的位置觸發那一半；對話選項那一半在上方凍結分支執行）。走進
    // 南側觸發區會武裝 Flag_LeaveInterlude；既有的 CheckChapterGates 插曲段兄弟 if 會
    // 消費它 -> Transition(InterludeReturnTo())。由於此觸發沒有事件可搭載，
    // CheckChapterGates 在每個非對話幀都輪詢；它具冪等性（第二／三章兄弟 if 以各自任務
    // 旗標為閘，插曲段這個以 Flag_LeaveInterlude 為閘，皆於使用時消費）。
    if (player &&
        world_.Semester().Current() == SemesterState::Interlude_Market) {
        const Vec2 c{player->GetPosition().x + playerSize_.x * 0.5f,
                     player->GetPosition().y + playerSize_.y * 0.5f};
        if (InInterludeExitZone(c)) {
            // 在本次造訪中，玩家踏入南側帶的「第一」幀發布「準備離開市集」；其後保持
            // 沉默。輔助函式掌管閂鎖翻轉 + Publish，使正式路徑與回歸測試走同一段程式碼
            //（測試直接對其自有閂鎖驅動 MaybeAnnounceInterludeExit）。閂鎖會在上方插曲段
            // 抵達分支重置，故日後再次造訪會恰好重發一次提示。
            //
            // 閂鎖現在存放於 SceneRouter；插曲段抵達時的閂鎖重置位於
            // SceneRouter::Settle()，故 GameController 在此只需轉發可變參照。
            MaybeAnnounceInterludeExit(bus_, sceneRouter_.InterludeExitLatchMut());
            player->SetFlag(kFlagLeaveInterlude);
        }
    }
    if (player) {
        // 只在苦主的 (d) 重逢致謝 對話關閉「之後」才觸發第一章通關（UmbrellaClaimed →
        // 插曲段），使玩家在第一章切往插曲段前先讀到該重逢場景。為 LiftChapter2Clear 的
        // 兄弟；在此（一個非對話幀，於上方對話提前返回之後）執行，使延後的發布在重逢
        // 對話框關閉後落地。在第一章之外／授予之前／對話進行中皆為空操作。
        LiftChapter1Clear(bus_, *player, world_.Semester().Current(),
                          world_.Dialog());
        // 只在學霸獲救「且」(d) 致謝對話已關閉時才升起 Flag_Ch2Cleared（延後處理，使
        // 閘門不會在該對話開啟的同一幀就把它關掉）。在 CheckChapterGates「之前」執行，
        // 使升起的旗標在同一幀被消費。在第二章之外／獲救之前皆為空操作。
        LiftChapter2Clear(*player, world_.Semester().Current(),
                          world_.Dialog());
        CheckChapterGates(bus_, *player, world_.Semester(), world_.Dialog());
        // 先延後、再解算結局。「先」開啟任何待播的第四章結局自白（內心獨白）——它會使
        // world_.Dialog() 進入啟用，故緊接其後的 CheckEndingGates 輪詢看到開啟的對話框
        // 便返回（結局等到玩家讀完並關閉旁白）。在稍後某個非對話輪詢時，自白已播畢
        //（一次性旗標）、對話框已關閉，CheckEndingGates 才觸發轉場。這與上方的
        // LiftChapter1/2Clear 是同樣的延後形狀，推廣到全部四種結局：閘門不再「只」於對話
        // 選項確認處輪詢，故收尾節拍（助教終局的後續台詞或這些自白）總是在結局切換前被
        // 讀到（即「不要突然按下選項後跳結局」）。在第四章之外／對話框開啟時／已解算後
        // 皆為空操作。
        TryOpenEndingConfession(*player, world_.Dialog(),
                                world_.Semester().Current());
        CheckEndingGates(bus_, *player, world_.Semester(), world_.Dialog());
    }

    // 幀末的延後刪除（管線的終端階段）。與先前內聯清掃的操作相同、幀時序相同——在互動／
    // 閘門邏輯「之後」執行，使閘門剛標記為死亡的物件能在本幀被回收。
    sweep_.Run(ctx, dt);

    // 為 npcs[] 列表關閉同幀轉場窗口。上方位置觸發分支產生的任何 FSM 轉場
    //（CheckChapterGates 消費 Flag_LeaveInterlude -> Transition；第二／三章 Clear
    // 旗標被消費 -> Transition 等）都必須在 View::Draw 繪製本幀「之前」併入「名冊」。
    // 修正前，轉場幀 N 的存檔有 semester=新、但 npcs[]=舊：每種結局的 7 個主線轉場之一。
    //
    // SettleSideEffects（玩家位置、消耗品、抵達提示等）刻意留在 Update 最前端，使
    // harness 的 lastWorld 快照在一個 tick 內看到「舊」的玩家位置——這完全符合修正前的
    // harness 約定，而腳本的 `interact <coord>`「已抵達」偵測正依賴它（若玩家在拾傘的
    // 同一幀就傳送，會使下一幀的計畫解析誤以為目標仍然很遠而卡住腳本）。此處僅供顯示。
    sceneRouter_.SettleRoster(world_);
}

// 結局畫面底部的 3 選項選單現位於 `controller/screens/EndingScreen.{h,cpp}`。保留本
// 方法作為薄轉發層，使 Update() 開頭的協調者提前返回鏈讀來不變。
bool GameController::HandleEndingMenu() {
    return nccu::HandleEndingMenu(world_);
}

// M 暫停選單 + 說明疊層現位於 `controller/screens/PauseScreen.{h,cpp}`。保留本方法
// 作為薄轉發層，使 Update() 開頭的協調者提前返回鏈讀來不變。
bool GameController::HandlePauseMenu() {
    return nccu::HandlePauseMenu(world_);
}

// 對話凍結 + 攤販確認 + 終局選項鎖 + 各選項閘門現位於
// `controller/screens/DialogScreen.{h,cpp}`。保留本方法作為薄轉發層，使 Update() 開頭
// 的協調者提前返回鏈讀來不變；pendingVendor_、input_、sceneRouter_ 以參照傳入，使該
// 自由函式能改寫它們。
bool GameController::HandleDialog() {
    return nccu::HandleDialog(bus_, world_, pendingVendor_, input_, sceneRouter_);
}


// Tab 物品欄疊層現位於 `controller/screens/InventoryScreen.{h,cpp}`。保留本方法作為
// 薄轉發層，使 Update() 開頭的協調者提前返回鏈讀來不變。
bool GameController::HandleInventory() {
    return nccu::HandleInventory(bus_, world_);
}

// 按 E 互動分派現位於 `controller/InteractDispatch.{h,cpp}`。保留本方法作為薄轉發層，
// 使 Update() 的呼叫點讀來不變；`pendingVendor_` 以參照傳入，使該自由函式能在一次 E
// 點按開啟商店選單時設定它。
void GameController::DispatchInteract() {
    nccu::DispatchInteract(bus_, world_, pendingVendor_);
}

} // namespace nccu
