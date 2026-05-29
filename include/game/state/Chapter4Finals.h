#ifndef CHAPTER4_FINALS_H_
#define CHAPTER4_FINALS_H_
#include "game/state/SemesterState.h"

namespace nccu {

class Chapter4Finals : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter4_Finals; }
    std::string_view Name() const override { return "第四章 期末"; }
};

} // namespace nccu

#endif // CHAPTER4_FINALS_H_
