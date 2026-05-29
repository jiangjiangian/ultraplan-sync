#ifndef CHAPTER3_SPORTS_DAY_H_
#define CHAPTER3_SPORTS_DAY_H_
#include "game/state/SemesterState.h"

/**
 * @file Chapter3SportsDay.h
 * @brief State 模式具體狀態：第三章「運動會」。
 */

namespace nccu {

/**
 * @brief 學期 State 機的第三章具體狀態（IChapterState 葉節點）。
 *
 * 由 SemesterStateMachine 在轉移到 Chapter3_SportsDay 時建立；只回報自身的
 * Id() 與顯示名稱，作為 State 模式中可替換的狀態物件。
 */
class Chapter3SportsDay : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter3_SportsDay; }
    std::string_view Name() const override { return "第三章 運動會"; }
};

} // namespace nccu

#endif // CHAPTER3_SPORTS_DAY_H_
