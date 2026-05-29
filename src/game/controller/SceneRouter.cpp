#include "game/controller/SceneRouter.h"
#include "game/quest/Flags.h"
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/state/InterludeExit.h"
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "game/world/World.h"

namespace nccu {

void SceneRouter::SettleRoster(World& world) {
    // 僅做名冊抽換，無可觀察副作用。於 Update() 結尾執行，使 View 在轉場幀就畫出
    // （並由序列化快照記錄）「新章節」的 npcs[]，而非晚一幀。早期整個重生＋副作用
    // 區塊都在 Update() 開頭；此處把僅影響檢視的那半（章節 NPC 清單）前移，以關閉
    // 可見的延遲，而不動到唯讀世界快照觀察到的 {player.pos, consumables, flags,
    // events}。另一半留在 Update() 開頭（下方 SettleSideEffects），故各結局的
    // 可觀察序列除了每局 7 個轉場幀上的 npcs[] 欄位（已記載、刻意為之的位元差異）
    // 外與基準逐位元相同。
    const SemesterState cur = world.Semester().Current();
    if (cur == lastRosterRespawnState_) return;

    // 實際的移除／生成由 World 透過單次延遲清除完成——絕不在迭代途中、也絕不重排
    // 元素 0（Player）。純資料變動，不發事件、不依賴輸入。
    world.RespawnChapterRoster(cur);

    lastRosterRespawnState_ = cur;
}

void SceneRouter::SettleSideEffects(World& world) {
    // 轉場觀察者的副作用半部。於 Update() 開頭執行——正是早期內嵌區塊觸發之處——使
    // 可觀察的 {player.pos, consumables, flags, events} 時間軸與先前一致。上方的名冊
    // 修正已關閉 npcs[] 的可見延遲；此入口點使其餘時間軸維持逐位元相同。
    const SemesterState cur = world.Semester().Current();
    if (cur == lastRosterState_) return;

    // 冪等的雙重保險：若此狀態的名冊尚未重生（Update() 結尾的 SettleRoster 因故被
    // 略過），就在此補做，避免 SpawnChapterNpcs 被重複呼叫。RespawnChapterRoster
    // 本身冪等，但呼叫兩次會做白工——以游標避免之。
    if (cur != lastRosterRespawnState_) {
        world.RespawnChapterRoster(cur);
        lastRosterRespawnState_ = cur;
    }

    // ----- 各目的地的副作用（可觀察的半部）-----

    // 抵達市集：把玩家放在其入口，遠在南側離開帶狀區之北。若無此步，在南側結束的
    // 章節會讓玩家一進場就已在離開區內，立刻被彈回（等於跳過市集）。章節進入點屬
    // 各章自身的範疇；市集是本類別唯一掌管的狀態。
    if (cur == SemesterState::Interlude_Market) {
        if (Player* ip = world.GetPlayer()) {
            ip->SetPosition(nccu::kInterludeEntry);
            // 「消耗品當章用完」：重新進入市集會清空消耗品背包，使為某章購買的物品
            // 無法越過市集邊界囤積到下一章——每次市集到訪都是一次全新的「為接下來的
            // 章節採買」決策（迴圈的張力來源）。
            ip->ClearConsumables();
        }
        // 前一章的清關提示與 FSM 跳轉同幀落地；此抵達提示約晚一幀覆蓋它，使玩家在
        // 同一次抵達同時看到兩者（先瞬間切換、再方向指引）。重置南側帶狀區閂鎖，
        // 使離開提示每次到訪觸發一次（重新進入時再觸發一次，但絕不連續兩次）。
        EventBus::Instance().Publish(
            Event{EventType::ShowMessage, kInterludeArrivalHint});
        interludeExitZoneLatched_ = false;
    }

    // 讓每章的「傘又掉了」字卡在機制上成真。所持的傘先前僅在 Ch4 進入時清除，故在
    // Ch1 取得（或 Ch2 借得）的傘會殘留到 Ch2/Ch3——View 顯示「傘又掉了」字卡，
    // 背包卻仍列著那把傘（即「真傘仍在 Ch2 背包」的 bug）。改為每章重新開始：玩家
    // 空手進場，該章再重新提供一把（Ch2 是管理員借出的、Ch3 是取回的真傘、Ch4 是
    // 結局／取回）——故空手抵達是刻意設計（已再次確認：每章皆有自己的傘路徑，可通關）。
    //
    // SetHasUmbrella(false) 同時清空所持傘槽位（見 Player.h），使背包的雨傘列消失
    // ——字卡與背包終於一致。清除 Flag_HasTrueUmbrella 使 Ending A 的「持有 TrueUmbrella」
    // 條件精確地只表示「重新取得本局 Ch4 的 TrueUmbrella」（見 EndingGate.cpp）：
    // Ch3 取得時重設它、Ch4 進入時再清除、Ch4 取回／結局時再重設——而它只在
    // Chapter4_Finals 有後果（CheckEndingGates 在其他狀態提早返回），故在 Ch2/Ch3
    // 進入時清除它是安全的。僅進入時觸發（SettleSideEffects 開頭的游標守門使其每章
    // 進入觸發一次）。跨市集的倖存者只有金錢、Flag_FoundForm 與本章剛取得的物品。
    if (cur == SemesterState::Chapter2_Midterms ||
        cur == SemesterState::Chapter3_SportsDay ||
        cur == SemesterState::Chapter4_Finals) {
        if (Player* ip = world.GetPlayer()) {
            ip->SetHasUmbrella(false);
            ip->ClearFlag(kFlagHasTrueUmbrella);
            // 受詛咒污染的衰減：每進入一個編號章節，從業力扣除 -5 * cursedTaint_。
            // 污染為 0 的局完全略過 AddKarma 呼叫（不發 KarmaChanged），使非詛咒
            // 的試玩與標準答案逐位元相同；污染 = 1 的局在 Ch2/Ch3/Ch4 進入時各扣
            // -5（乾淨地在 Ch1 撿了詛咒傘者共 -15）；污染 = 2 則加倍為每次轉場 -10，
            // 以此類推。
            ip->ApplyCursedTaintDecay();
        }
    }

    lastRosterState_ = cur;
}

} // namespace nccu
