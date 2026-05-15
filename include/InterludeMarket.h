#ifndef INTERLUDE_MARKET_H_
#define INTERLUDE_MARKET_H_
#include "SemesterState.h"

namespace nccu {

class InterludeMarket : public IChapterState {
public:
    SemesterState    Id()   const override { return SemesterState::Interlude_Market; }
    std::string_view Name() const override { return "幕間 福利社"; }
};

} // namespace nccu

#endif // INTERLUDE_MARKET_H_
