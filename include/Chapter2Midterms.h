#ifndef CHAPTER2_MIDTERMS_H_
#define CHAPTER2_MIDTERMS_H_
#include "SemesterState.h"

namespace nccu {

class Chapter2Midterms : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Chapter2_Midterms; }
    std::string_view Name() const override { return "第二章 期中考"; }
};

} // namespace nccu

#endif // CHAPTER2_MIDTERMS_H_
