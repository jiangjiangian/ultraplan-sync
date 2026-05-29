#include "game/state/SemesterStateMachine.h"

#include "game/state/Chapter1AddDrop.h"
#include "game/state/Chapter2Midterms.h"
#include "game/state/Chapter3SportsDay.h"
#include "game/state/Chapter4Finals.h"
#include "game/state/InterludeMarket.h"

/**
 * @file SemesterStateMachine.cpp
 * @brief 學期狀態機的轉移實作：建立／替換具體狀態並維持結局以哨兵表示的不變式。
 */

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
            case SemesterState::Ending_D: return "結局 D";
            case SemesterState::Ending_C: return "結局 C";
            default:                      return "";
        }
    }
    return state_->Name();
}

void SemesterStateMachine::Transition(SemesterState next) {
    // 先讓舊狀態收尾（Exit），再依目標建立新狀態。章節／幕間各對應一個具體
    // IChapterState；四種結局不另建類別，而是釋放 state_、改以 ending_ 哨兵 +
    // inEnding_ 旗標表示——這也是 Current()／CurrentName() 須先檢查 inEnding_
    // 的原因。
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
        case SemesterState::Ending_D:
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
