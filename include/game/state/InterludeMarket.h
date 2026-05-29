#ifndef INTERLUDE_MARKET_H_
#define INTERLUDE_MARKET_H_
#include "game/state/SemesterState.h"

/**
 * @file InterludeMarket.h
 * @brief State 模式具體狀態：章節之間的「幕間市集」。
 */

namespace nccu {

/**
 * @brief 學期 State 機的幕間市集狀態（IChapterState 葉節點）。
 *
 * 章節清關後進入的過場；玩家在此採買，往南離開後依
 * SemesterStateMachine::InterludeReturnTo() 回到下一章。作為 State 模式中可
 * 替換的狀態物件，只回報自身的 Id() 與顯示名稱。
 */
class InterludeMarket : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Interlude_Market; }
    std::string_view Name() const override { return "幕間 市集"; }
};

} // namespace nccu

#endif // INTERLUDE_MARKET_H_
