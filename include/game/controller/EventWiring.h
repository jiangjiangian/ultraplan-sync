#ifndef EVENT_WIRING_H_
#define EVENT_WIRING_H_
#include "game/state/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/World.h"
#include <iostream>
#include <string>

namespace nccu {

/**
 * @file EventWiring.h
 * @brief 把各 EventBus 訂閱者（Observer 模式的觀察者）接到匯流排的安裝函式集合。
 *
 * 涵蓋：純記錄訂閱者、章節轉場狀態訂閱者、HUD 橫幅鏡射訂閱者、業力提示訂閱者，
 * 以及保留舊單一進入點的彙整函式。各訂閱者多以參考捕捉呼叫端所擁有的狀態，故呼叫端
 * 須比訂閱更長壽，或在那些參考離開作用域前先呼叫 EventBus::Clear()。
 */

/**
 * @brief 接上只輸出到 cout 的記錄型訂閱者（ShowMessage、UmbrellaClaimed）。
 * @param[in,out] bus 要註冊訂閱者的事件匯流排。
 *
 * 純 I/O——不持有任何遊戲狀態。
 */
inline void WireLoggingSubscribers(EventBus& bus) {
    bus.Subscribe(EventType::ShowMessage,
            [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; })
       .Subscribe(EventType::UmbrellaClaimed,
            [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
}

/**
 * @brief 接上狀態轉場訂閱者：EnteredBuilding 追蹤者與真正的章節推進 gate。
 * @param[in,out] bus                 要註冊訂閱者的事件匯流排。
 * @param[in,out] semester            被轉場推進的學期狀態機。
 * @param[out]    currentBuildingName 餵給「當前建築」HUD 標籤的字串。
 *
 * EnteredBuilding 追蹤者更新當前建築 HUD 標籤；章節 gate 在玩家於 Chapter 1 取得
 * TrueUmbrella 時清關並推進到市集過場。以參考捕捉呼叫端所擁有的狀態——呼叫端須比
 * 訂閱更長壽，或在那些參考離開作用域前先呼叫 EventBus::Clear()（main.cpp 於
 * return 0 之前採後者）。
 */
inline void WireStateTransitionSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName)
{
    bus.Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName](const Event& e) {
            currentBuildingName = e.text;
            std::cout << "[Game] Entered: " << e.text << '\n';
        });

    // 真正的 Ch1 章節 gate：在 Chapter 1 取得 TrueUmbrella 即清關並推進到市集過場。
    // 這是 chapter1 兩個清關條件中較窄的一個（取得 TrueUmbrella）；較寬的「持有任意
    // 傘種離開集英樓」位置路徑則延後處理（刻意為之，非 bug）。日後新增 gate 請以此
    // 完全相同形狀新增並列 if——暫不一般化。
    bus.Subscribe(EventType::UmbrellaClaimed,
        [&bus, &semester](const Event& e) {
            if (e.text == "TrueUmbrella" &&
                semester.Current() == SemesterState::Chapter1_AddDrop) {
                // 進入市集前先種下「第一座市集返回何處」（Ch2）——InterludeMarket
                // 狀態物件每次 Transition 都會重建，故 returnTo 存在狀態機上。
                // CheckChapterGates 於離開市集時讀它；稍後的 Ch2/Ch3 gate 會為
                // 第二、三座市集重新種下。
                semester.SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
                semester.Transition(SemesterState::Interlude_Market);
                // Ch1 -> 市集的跳轉在此觸發，而非在 ChapterGate（chapter1 沒有對應
                // 的清關旗標樁）。若無此提示橫幅，序列化記錄中唯一可見的變化只有取傘
                // 文字，會遮蔽章節的瞬間切換。傳入 lambda 捕捉的 bus（而非拉取單例）：
                // 此訂閱者本就掛在 bus 上，是同一個實例，只是不再隱含。
                nccu::PublishChapterTransitionToast(
                    bus, SemesterState::Interlude_Market);
            }
            // Ch3 清關：啦啦隊的 TrueUmbrella，自體育館後台道具箱取回。形狀同上方
            // Ch1 並列 if（與 Ch1 同構的實體取回）——第三座市集返回 Ch4。Ch2 的清關
            // 改走「喚醒 + LiftChapter2Clear」路徑（一場社交交換，無撿傘動作）。
            // ChapterGate.cpp 中對應的並列 if 仍作為 test_chapter_spine 的測試樁；
            // 遊戲內真正的路徑是此處。
            if (e.text == "TrueUmbrella" &&
                semester.Current() == SemesterState::Chapter3_SportsDay) {
                semester.SetInterludeReturnTo(SemesterState::Chapter4_Finals);
                semester.Transition(SemesterState::Interlude_Market);
                nccu::PublishChapterTransitionToast(
                    bus,
                    SemesterState::Interlude_Market);
            }
        });
}

/**
 * @brief 把每則 ShowMessage 鏡射到 World 的暫時性 HUD 橫幅。
 * @param[in,out] bus   要註冊訂閱者的事件匯流排。
 * @param[in,out] world 接收最新訊息文字以供 View 繪製淡出提示的世界。
 *
 * 每則 EventType::ShowMessage（任務提示、喚醒提示、章節清關旁白、漣漪反應、攤主
 * 購買文字）都被鏡射進呼叫端所擁有的 World，使 View 能把最新一則畫成淡出提示。
 * WireLoggingSubscribers 仍會把同一事件記到 cout——此為「額外」訂閱者，非取代。
 * 以參考捕捉 world，捕捉生命週期同 WireStateTransitionSubscribers：呼叫端
 * （GameController）須比訂閱更長壽，或先呼叫 EventBus::Clear()——GameController 的
 * 解構子採後者。
 *
 * 事件的 slot 欄位把文字導向 World 上的 Top 或 Bottom HUD 通道（見 HudSlot.h）。
 * 早期所有發布者預設 Bottom（處理函式本體毋須更動），唯三個明確選用的 ChapterToast／
 * EndingGate 站點用 Top。故同幀的 Top 與 Bottom 發布得以並存而不互相覆蓋。
 */
inline void WireHudMessageSubscriber(EventBus& bus, World& world) {
    bus.Subscribe(EventType::ShowMessage,
        [&world](const Event& e) { world.SetHudMessage(e.slot, e.text); });
}

/**
 * @brief 把原本無人消費的 KarmaChanged 通道轉成可見的業力提示。
 * @param[in,out] bus 同時供訂閱與重發 ShowMessage 的事件匯流排。
 *
 * Player::AddKarma 以帶正負號的差值文字（"+5"、"-3"）發布 KarmaChanged；此訂閱者
 * 把它重發為加上「業力」前綴的 ShowMessage，再由上面的 HUD 訂閱者鏡射進
 * World::HudMessage()。業力提示與章節／敘事提示共用同一個 HUD 欄位，故與章節轉場
 * 同幀落地的業力變化會被章節橫幅刻意覆蓋（章節回饋優先）。當呼叫端傳入 AddKarma(0)
 * 時略過 0 差值提示，使 HUD 保持乾淨（少見，但過濾成本低）。
 */
inline void WireKarmaToastSubscriber(EventBus& bus) {
    bus.Subscribe(EventType::KarmaChanged,
        [&bus](const Event& e) {
            if (e.text.empty() || e.text == "+0" || e.text == "-0")
                return;
            // 「業力 」加上帶正負號的差值文字遠在對話框 28 格的預算之內（「業力」每字
            // 佔 2 格，差值即使含正負號、±100 也至多 5 個 ASCII 字元）。
            // 在「我們所訂閱的同一個 bus」上重發——絕不用 EventBus::Instance()。在
            // 重導的事件接收端（測試隔離／驗證環境）下，衍生的 ShowMessage 必須落在
            // 與其 KarmaChanged 來源相同的 bus，否則該處的 HUD 訂閱者永遠看不到提示
            // （慣例：處理函式用捕捉到的 bus，絕不重新取單例）。生命週期安全：
            // GameController 解構子會在它被銷毀前 Clear 此 bus，故捕捉的參考絕不懸空。
            bus.Publish(Event{EventType::ShowMessage, "業力 " + e.text});
        });
}

/**
 * @brief 便利彙整函式——保留原本的單一呼叫進入點，使 main.cpp 既有呼叫點毋須更動。
 * @param[in,out] bus                 要註冊訂閱者的事件匯流排。
 * @param[in,out] semester            傳給狀態轉場訂閱者的學期狀態機。
 * @param[out]    currentBuildingName 傳給狀態轉場訂閱者的當前建築名稱字串。
 */
inline void WireDefaultSubscribers(
    EventBus&                                                  bus,
    SemesterStateMachine&                                      semester,
    std::string&                                               currentBuildingName)
{
    WireLoggingSubscribers(bus);
    WireStateTransitionSubscribers(bus, semester, currentBuildingName);
}

} // namespace nccu

#endif // EVENT_WIRING_H_
