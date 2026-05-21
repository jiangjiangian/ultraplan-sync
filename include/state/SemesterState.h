#ifndef SEMESTER_STATE_H_
#define SEMESTER_STATE_H_
#include <string_view>

namespace nccu {

enum class SemesterState {
    Chapter1_AddDrop,
    Interlude_Market,
    Chapter2_Midterms,
    Chapter3_SportsDay,
    Chapter4_Finals,
    Ending_A,
    Ending_B,
    Ending_C,
};

class IChapterState {
public:
    virtual ~IChapterState() = default;
    virtual SemesterState    Id()   const = 0;
    virtual std::string_view Name() const = 0;
    virtual void Enter() {}
    virtual void Exit()  {}
    virtual void Update(float /*dt*/) {}
};

} // namespace nccu

#endif // SEMESTER_STATE_H_
