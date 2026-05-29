#ifndef CHAPTER1_ADD_DROP_H_
#define CHAPTER1_ADD_DROP_H_
#include "game/state/SemesterState.h"

/**
 * @file Chapter1AddDrop.h
 * @brief State 模式具體狀態：第一章「加退選」。
 */

namespace nccu {

/**
 * @brief 學期 State 機的第一章具體狀態（IChapterState 葉節點）。
 *
 * 由 SemesterStateMachine 在轉移到 Chapter1_AddDrop 時建立；只回報自身的
 * Id() 與顯示名稱，章節邏輯（生成、劇情）分散在世界與任務層，本型別僅作為
 * State 模式中可替換的狀態物件。
 */
class Chapter1AddDrop : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter1_AddDrop; }
    std::string_view Name() const override { return "第一章 加退選"; }
};

} // namespace nccu

#endif // CHAPTER1_ADD_DROP_H_
