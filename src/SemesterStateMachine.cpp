#include "SemesterStateMachine.h"

#include "Chapter1AddDrop.h"
#include "Chapter2Midterms.h"
#include "Chapter3SportsDay.h"
#include "Chapter4Finals.h"
#include "InterludeMarket.h"

namespace nccu {

SemesterStateMachine::SemesterStateMachine()
    : state_(std::make_unique<Chapter1AddDrop>()) {
    state_->Enter();
}

SemesterState SemesterStateMachine::Current() const noexcept {
    if (inEnding_) {
        return ending_;
    }
    return state_->Id();
}

std::string_view SemesterStateMachine::CurrentName() const {
    if (inEnding_) {
        switch (ending_) {
            case SemesterState::Ending_A: return "結局 A";
            case SemesterState::Ending_B: return "結局 B";
            case SemesterState::Ending_C: return "結局 C";
            default:                      return "";
        }
    }
    return state_->Name();
}

void SemesterStateMachine::Transition(SemesterState next) {
    if (state_) {
        state_->Exit();
    }
    switch (next) {
        case SemesterState::Chapter1_AddDrop:
            state_ = std::make_unique<Chapter1AddDrop>();
            inEnding_ = false;
            break;
        case SemesterState::Interlude_Market:
            state_ = std::make_unique<InterludeMarket>();
            inEnding_ = false;
            break;
        case SemesterState::Chapter2_Midterms:
            state_ = std::make_unique<Chapter2Midterms>();
            inEnding_ = false;
            break;
        case SemesterState::Chapter3_SportsDay:
            state_ = std::make_unique<Chapter3SportsDay>();
            inEnding_ = false;
            break;
        case SemesterState::Chapter4_Finals:
            state_ = std::make_unique<Chapter4Finals>();
            inEnding_ = false;
            break;
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            state_.reset();
            ending_   = next;
            inEnding_ = true;
            break;
    }
    if (state_) {
        state_->Enter();
    }
}

void SemesterStateMachine::SetInterludeReturnTo(SemesterState next) noexcept {
    interludeReturnTo_ = next;
}

SemesterState SemesterStateMachine::InterludeReturnTo() const noexcept {
    return interludeReturnTo_;
}

void SemesterStateMachine::Update(float dt) {
    if (state_) {
        state_->Update(dt);
    }
}

} // namespace nccu
