#pragma once
#include "SemesterState.h"

namespace nccu {

class Chapter3SportsDay : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter3_SportsDay; }
    std::string_view Name() const override { return "第三章 運動會"; }
};

} // namespace nccu
