#pragma once
#include "SemesterState.h"
#include <memory>
#include <string_view>

namespace nccu {

class SemesterStateMachine {
public:
    SemesterStateMachine(); // initial state = Chapter1_AddDrop
    ~SemesterStateMachine() = default;

    SemesterStateMachine(const SemesterStateMachine&) = delete;
    SemesterStateMachine& operator=(const SemesterStateMachine&) = delete;

    SemesterState    Current()     const noexcept;
    std::string_view CurrentName() const;
    void             Transition(SemesterState next);
    void             Update(float dt);

private:
    std::unique_ptr<IChapterState> state_;
    SemesterState                  ending_{SemesterState::Chapter1_AddDrop}; // sentinel for endings
    bool                           inEnding_{false};
};

} // namespace nccu
