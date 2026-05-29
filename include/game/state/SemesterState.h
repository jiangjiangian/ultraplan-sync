#ifndef SEMESTER_STATE_H_
#define SEMESTER_STATE_H_
#include <string_view>

/**
 * @file SemesterState.h
 * @brief 學期流程的狀態列舉與 State 模式的抽象狀態介面 IChapterState。
 */

namespace nccu {

/**
 * @brief 學期流程的全部狀態：四章節、幕間市集與四種結局。
 *
 * 同時作為 SemesterStateMachine 的對外身分識別與 EndingGate 的轉移目標。
 * 結局項的列舉順序刻意對應結局判定優先序 A→B→D→C。
 */
enum class SemesterState {
    Chapter1_AddDrop,
    Interlude_Market,
    Chapter2_Midterms,
    Chapter3_SportsDay,
    Chapter4_Finals,
    Ending_A,
    Ending_B,
    /// 風雨同行：選擇體諒但分數未達 A、亦未落入 B 的苦甜結局（傘已磨損成破傘）。
    /// 刻意排在 Ending_C 之前以對應判定優先序 A→B→D→C。
    Ending_D,
    Ending_C,
};

/**
 * @brief State 模式的抽象狀態：每個學期狀態（章節／幕間）的共同介面。
 *
 * SemesterStateMachine 持有一個此型別的指標代表「目前所處狀態」，轉移時銷毀舊
 * 物件、建立新物件。子類別至少回報 Id() 與 Name()；Enter()／Exit()／Update()
 * 提供進入／離開／逐幀的擴充掛勾，預設為空實作。
 */
class IChapterState {
public:
    virtual ~IChapterState() = default;
    /// @brief 回報此狀態對應的 SemesterState 列舉值。
    virtual SemesterState    Id()   const = 0;
    /// @brief 回報此狀態的繁中顯示名稱。
    virtual std::string_view Name() const = 0;
    /// @brief 轉入此狀態時呼叫一次（預設不做事）。
    virtual void Enter() {}
    /// @brief 轉出此狀態時呼叫一次（預設不做事）。
    virtual void Exit()  {}
    /// @brief 逐幀更新掛勾（預設不做事）。@param dt 與上一幀的間隔秒數。
    virtual void Update(float /*dt*/) {}
};

} // namespace nccu

#endif // SEMESTER_STATE_H_
