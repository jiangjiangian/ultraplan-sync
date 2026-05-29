#ifndef CHAPTER2_MIDTERMS_H_
#define CHAPTER2_MIDTERMS_H_
#include "game/state/SemesterState.h"

/**
 * @file Chapter2Midterms.h
 * @brief State 模式具體狀態：第二章「期中考」。
 */

namespace nccu {

/**
 * @brief 學期 State 機的第二章具體狀態（IChapterState 葉節點）。
 *
 * 由 SemesterStateMachine 在轉移到 Chapter2_Midterms 時建立；只回報自身的
 * Id() 與顯示名稱，作為 State 模式中可替換的狀態物件。
 */
class Chapter2Midterms : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter2_Midterms; }
    std::string_view Name() const override { return "第二章 期中考"; }
};

} // namespace nccu

#endif // CHAPTER2_MIDTERMS_H_
