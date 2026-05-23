#include "quest/Chapter3Quest.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include <string>

namespace nccu {

void TryAdvanceCh3Trade(Player& player, std::string_view npcId,
                        SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;

    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);

    if (npcId == "vendor_sausage_a") {
        // chapter3.md A 系 (b)「交易完成」`// karma +3`（物物交換鏈
        // 順利啟動）. Fires once: any later chain flag gates it out.
        if (sausage || loud || known) return;
        player.AddKarma(3).SetFlag(kFlagHasSausage);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("A 系攤主塞給你一根烤香腸。熱的，很燙手。")});
        return;
    }

    if (npcId == "loudspeaker_b") {
        // chapter3.md B 系 (b)「交易完成」`// karma +3`（第二環完成）.
        if (loud || known) return;             // already past link 2
        if (!sausage) {                        // out-of-order → point back to A
            EventBus::Instance().Publish(Event{
                EventType::ShowMessage,
                std::string("B 系同學：你手上沒吃的啊？"
                            "先去 A 系烤香腸攤換一根熱香腸來。")});
            return;
        }
        // 香腸換大聲公 — consume the sausage, hand over the 大聲公.
        player.ClearFlag(kFlagHasSausage);
        player.AddKarma(3).SetFlag(kFlagHasLoudspeaker);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("B 系同學大喜，把那個大聲公塞進你懷裡。")});
        return;
    }

    if (npcId == "senior_c") {
        // chapter3.md C 系 (b)「情報揭露」`// karma +5`（全環完成，
        // 情報取得）. 大聲公換情報——傘在體育館後台道具箱。
        if (known) return;                     // info already revealed
        if (!loud) {                           // out-of-order → point back to B
            EventBus::Instance().Publish(Event{
                EventType::ShowMessage,
                std::string("C 系學姊：空手來談情報？"
                            "先去 B 系把香腸換成大聲公再來找我。")});
            return;
        }
        player.ClearFlag(kFlagHasLoudspeaker);
        player.AddKarma(5).SetFlag(kFlagKnowsUmbrellaLoc);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("C 系學姊壓低聲音："
                        "你的傘在體育館後台道具箱，第三個。")});
        return;
    }
}

// Sequential `!` gating for the Ch3 物物交換鏈: only the NEXT link in the
// A→B→C chain shows its indicator, so the three light up in turn instead of
// all at once (player request). View calls this when state==Chapter3.
// NOTE: A's gate will also require the 操場-lap flag once the lap mechanic
// lands; for now A is visible from chapter entry.
bool Ch3IndicatorVisible(std::string_view npcId, const Player& player) {
    const bool sausage = player.HasFlag(kFlagHasSausage);
    const bool loud    = player.HasFlag(kFlagHasLoudspeaker);
    const bool known   = player.HasFlag(kFlagKnowsUmbrellaLoc);
    if (npcId == "vendor_sausage_a") return !sausage;           // A: until traded
    if (npcId == "loudspeaker_b")    return sausage && !loud;   // B: after A
    if (npcId == "senior_c")         return loud && !known;     // C: after B
    return true;   // any other quest-giver: unchanged (always shown)
}

void TryApplyCh3Ripple(Player& player, SemesterState state) {
    if (state != SemesterState::Chapter3_SportsDay) return;
    if (player.HasFlag(kFlagCh3RippledProfTrap)) return;       // once
    if (!player.HasFlag("Flag_HasProfessorTrap")) return;
    // chapter3.md 章節結尾分支二: 「後台某個同學看了你手上的傘一眼」
    // → `// karma -10`（Ch1 漣漪延伸至 Ch3）。獨立 once-key，與
    // Flag_Ch2Rippled_TA 分開，故 Ch2 已扣過本次仍照扣（L329）。
    player.AddKarma(-10).SetFlag(kFlagCh3RippledProfTrap);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("有人看了你手上的傘一眼："
                    "「那把……是教授研究室借出去的那把嗎？」")});
}

} // namespace nccu
