#include "game/quest/Chapter3Quest.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include <string>

namespace nccu {

void TryAdvanceCh3Trade(EventBus& bus, Player& player,
                        std::string_view npcId, SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;

    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);

    if (npcId == "vendor_sausage_a") {
        // chapter3.md A 系 (b)「交易完成」`// karma +3`（物物交換鏈啟動）.
        if (sausage || loud || known) return;          // chain already past A
        if (!player.HasFlag(kFlagSportsLapDone)) {            // 校慶: run the lap first
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
        // chapter3.md B 系 (b)「交易完成」`// karma +3`（第二環完成）.
        if (loud || known) return;             // already past link 2
        if (!sausage) {                        // out-of-order → point back to A
            bus.Publish(Event{
                EventType::ShowMessage,
                std::string("B 系同學：你手上沒吃的啊？"
                            "先去 A 系烤香腸攤換一根熱香腸來。")});
            return;
        }
        // 香腸換大聲公 — consume the sausage, hand over the 大聲公.
        player.ClearFlag(kFlagHasSausage);
        player.AddKarma(3).SetFlag(kFlagHasLoudspeaker);
        bus.Publish(Event{
            EventType::ShowMessage,
            std::string("B 系同學大喜，把那個大聲公塞進你懷裡。")});
        return;
    }

    if (npcId == "senior_c") {
        // chapter3.md C 系 (b)「情報揭露」`// karma +5`（全環完成，
        // 情報取得）. 大聲公換情報——傘在體育館後台道具箱。
        if (known) return;                     // info already revealed
        if (!loud) {                           // out-of-order → point back to B
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

// Sequential `!` gating for the Ch3 物物交換鏈: only the NEXT link in the
// A→B→C chain shows its indicator, so the three light up in turn instead of
// all at once (player request). View calls this (via QuestIndicatorVisible)
// when state==Chapter3.
//
// Item 4a (the "dead first step" fix): A now lights from chapter ENTRY,
// BEFORE the 操場 lap, not after it. Pre-fix, A stayed dark until
// Flag_SportsLapDone, so on entering Ch3 the player saw NO `!` anywhere and
// had only the HUD objective to go on — "wandered lost". Lighting A
// immediately gives a concrete target: walking up to A pre-lap triggers
// TryAdvanceCh3Trade's "先去操場跑一圈…回來我請你吃香腸" redirect, which
// teaches step 1 in-fiction; after the lap the same `!` on A now hands over
// the sausage. So A is the visible chain head whenever the chain has not
// started (no sausage / loudspeaker / known-loc yet), lap or not.
bool Ch3IndicatorVisible(std::string_view npcId, const Player& player) {
    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);
    if (npcId == "vendor_sausage_a")
        return !sausage && !loud && !known;   // A: chain head, incl. pre-lap
    if (npcId == "loudspeaker_b")    return sausage && !loud;   // B: after A
    if (npcId == "senior_c")         return loud && !known;     // C: after B
    return true;   // any other quest-giver: unchanged (always shown)
}

void TryApplyCh3Ripple(EventBus& bus, Player& player, SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;
    if (player.HasFlag(kFlagCh3RippledProfTrap)) return;       // once
    if (!player.HasFlag(kFlagHasProfessorTrap)) return;
    // chapter3.md 章節結尾分支二: 「後台某個同學看了你手上的傘一眼」
    // → `// karma -10`（Ch1 漣漪延伸至 Ch3）。獨立 once-key，與
    // Flag_Ch2Rippled_TA 分開，故 Ch2 已扣過本次仍照扣（L329）。
    player.AddKarma(-10).SetFlag(kFlagCh3RippledProfTrap);
    bus.Publish(Event{
        EventType::ShowMessage,
        std::string("有人看了你手上的傘一眼："
                    "「那把……是教授研究室借出去的那把嗎？」")});
}

} // namespace nccu
