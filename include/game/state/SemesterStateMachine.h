#ifndef SEMESTER_STATE_MACHINE_H_
#define SEMESTER_STATE_MACHINE_H_
#include "game/state/SemesterState.h"
#include <memory>
#include <string_view>

namespace nccu {

class SemesterStateMachine {
public:
    SemesterStateMachine(); // initial state = Chapter1_AddDrop
    ~SemesterStateMachine() = default;

    SemesterStateMachine(const SemesterStateMachine&) = delete;
    SemesterStateMachine& operator=(const SemesterStateMachine&) = delete;

    [[nodiscard]] SemesterState    Current()     const noexcept;
    [[nodiscard]] std::string_view CurrentName() const;
    void                           Transition(SemesterState next);
    void                           Update(float dt);

    // Where the Interlude returns to once the player leaves the market.
    // Stored on the machine (not the InterludeMarket object) because every
    // Transition() recreates the IChapterState, so a state object cannot
    // carry data across the chapter -> Interlude -> chapter round trip.
    // The chapter that *enters* the Interlude sets this; the Interlude
    // exit gate reads it. Defaults to Chapter2_Midterms (the first market
    // always follows Chapter 1).
    void                           SetInterludeReturnTo(SemesterState next) noexcept;
    [[nodiscard]] SemesterState    InterludeReturnTo() const noexcept;

private:
    std::unique_ptr<IChapterState> state_;
    SemesterState                  ending_{SemesterState::Chapter1_AddDrop}; // sentinel for endings
    bool                           inEnding_{false};
    SemesterState                  interludeReturnTo_{SemesterState::Chapter2_Midterms};
};

} // namespace nccu

#endif // SEMESTER_STATE_MACHINE_H_
