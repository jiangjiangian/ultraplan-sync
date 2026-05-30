#include "game/quest/Chapter3Quest.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include <string>

/**
 * @file Chapter3Quest.cpp
 * @brief 第三章（校慶）任務邏輯：A→B→C 物物交換鏈的推進與順序提示、`!` 指示
 *        的依序點亮，以及 Ch1 漣漪延伸至 Ch3 的扣分。
 */

namespace nccu {

void TryAdvanceCh3Trade(EventBus& bus, Player& player,
                        std::string_view npcId, SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;

    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);

    if (npcId == "vendor_sausage_a") {
        // A 系（交易完成，karma +3）：物物交換鏈的啟動點。
        if (sausage || loud || known) return;          // 鏈已推進過 A，不再重複
        if (!player.HasFlag(kFlagSportsLapDone)) {      // 校慶：得先去操場跑一圈
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("A 系攤主：先去操場跑一圈參加校慶，"
                            "回來我請你吃香腸！")});
            return;
        }
        player.AddKarma(3).SetFlag(kFlagHasSausage);
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("A 系攤主塞給你一根烤香腸。熱的，很燙手。")});
        return;
    }

    if (npcId == "loudspeaker_b") {
        // B 系（交易完成，karma +3）：第二環完成。
        if (loud || known) return;             // 已推進過第二環
        if (!sausage) {                        // 順序不對 → 指回 A 系
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("B 系同學：你手上沒吃的啊？"
                            "先去 A 系烤香腸攤換一根熱香腸來。")});
            return;
        }
        // 香腸換大聲公——消耗香腸，交付大聲公。
        player.ClearFlag(kFlagHasSausage);
        player.AddKarma(3).SetFlag(kFlagHasLoudspeaker);
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("B 系同學大喜，把那個大聲公塞進你懷裡。")});
        return;
    }

    if (npcId == "senior_c") {
        // C 系（情報揭露，karma +5）：全環完成、取得情報。
        // 大聲公換情報——傘在體育館後台道具箱。
        if (known) return;                     // 情報已揭露
        if (!loud) {                           // 順序不對 → 指回 B 系
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("C 系學姊：空手來談情報？"
                            "先去 B 系把香腸換成大聲公再來找我。")});
            return;
        }
        player.ClearFlag(kFlagHasLoudspeaker);
        player.AddKarma(5).SetFlag(kFlagKnowsUmbrellaLoc);
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("C 系學姊壓低聲音："
                        "你的傘在體育館後台道具箱，第三個。")});
        return;
    }
}

// Ch3 物物交換鏈的依序 `!` 點亮：只有 A→B→C 鏈上的「下一環」會顯示提示，
// 使三者輪流亮起而非同時全亮。View 在 state==Chapter3 時透過
// QuestIndicatorVisible 呼叫本函式。
//
// A 自章節「進入」起就亮，而非等跑完操場那一圈之後。否則 A 會一直暗到跑完操場
// 為止，玩家進入 Ch3 時將看不到任何 `!`，只能靠 HUD 目標摸索而容易迷失方向。
// 立即點亮 A 給出具體目標：跑圈前走近 A 會觸發 TryAdvanceCh3Trade 的「先去操場
// 跑一圈…回來我請你吃香腸」轉向提示，以情境教會第一步；跑完圈後同一個 A 上的
// `!` 就會交付香腸。因此只要鏈尚未開始（還沒有香腸／大聲公／已知傘位），不論
// 跑圈與否，A 都是可見的鏈頭。
bool Ch3IndicatorVisible(std::string_view npcId, const Player& player) {
    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);
    if (npcId == "vendor_sausage_a")
        return !sausage && !loud && !known;   // A：鏈頭，含跑圈前
    if (npcId == "loudspeaker_b")    return sausage && !loud;   // B：A 之後
    if (npcId == "senior_c")         return loud && !known;     // C：B 之後
    return true;   // 其他任務給予者：維持不變（恆顯示）
}

void TryApplyCh3Ripple(EventBus& bus, Player& player, SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;
    if (player.HasFlag(kFlagCh3RippledProfTrap)) return;       // 只觸發一次
    if (!player.HasFlag(kFlagHasProfessorTrap)) return;
    // 章節結尾分支：「後台某個同學看了你手上的傘一眼」，karma -10
    //（Ch1 的漣漪延伸到 Ch3）。本次以獨立的「只觸發一次」旗標
    // kFlagCh3RippledProfTrap 把關，與 Ch2 的漣漪旗標分開，因此即使
    // Ch2 已扣過，本次仍照扣。
    player.AddKarma(-10).SetFlag(kFlagCh3RippledProfTrap);
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string("有人看了你手上的傘一眼："
                    "「那把……是教授研究室借出去的那把嗎？」")});
}

} // namespace nccu
