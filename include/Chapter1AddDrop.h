#pragma once
#include "SemesterState.h"

namespace nccu {

class Chapter1AddDrop : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter1_AddDrop; }
    std::string_view Name() const override { return "第一章 加退選"; }
};

} // namespace nccu
