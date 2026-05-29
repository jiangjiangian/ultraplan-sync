#ifndef CHAPTER4_FINALS_H_
#define CHAPTER4_FINALS_H_
#include "game/state/SemesterState.h"

/**
 * @file Chapter4Finals.h
 * @brief State 模式具體狀態：第四章「期末」。
 */

namespace nccu {

/**
 * @brief 學期 State 機的第四章具體狀態（IChapterState 葉節點）。
 *
 * 由 SemesterStateMachine 在轉移到 Chapter4_Finals 時建立；只回報自身的
 * Id() 與顯示名稱。結局的判定不在此處，而由 EndingGate 在本章輪詢旗標決定。
 */
class Chapter4Finals : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter4_Finals; }
    std::string_view Name() const override { return "第四章 期末"; }
};

} // namespace nccu

#endif // CHAPTER4_FINALS_H_
