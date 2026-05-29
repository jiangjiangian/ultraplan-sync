#ifndef SEMESTER_STATE_MACHINE_H_
#define SEMESTER_STATE_MACHINE_H_
#include "game/state/SemesterState.h"
#include <memory>
#include <string_view>

/**
 * @file SemesterStateMachine.h
 * @brief GoF State 模式的上下文（Context）：驅動學期在章節／幕間／結局之間轉移。
 */

namespace nccu {

/**
 * @brief 學期狀態機——State 模式的 Context。
 *
 * 持有單一目前狀態（IChapterState），Transition() 銷毀舊狀態、建立新狀態並依序
 * 觸發 Exit()／Enter()。結局狀態不另設具體 IChapterState 類別，而以 inEnding_
 * 旗標搭配 ending_ 哨兵表示。不可複製：狀態機代表唯一的學期進度。
 */
class SemesterStateMachine {
public:
    /// @brief 建構時即進入初始狀態 Chapter1_AddDrop 並觸發其 Enter()。
    SemesterStateMachine();
    ~SemesterStateMachine() = default;

    SemesterStateMachine(const SemesterStateMachine&) = delete;
    SemesterStateMachine& operator=(const SemesterStateMachine&) = delete;

    /// @brief 取得目前狀態的 SemesterState（結局期間回傳對應的結局列舉）。
    [[nodiscard]] SemesterState    Current()     const noexcept;
    /// @brief 取得目前狀態的繁中顯示名稱。
    [[nodiscard]] std::string_view CurrentName() const;
    /**
     * @brief 轉移到指定狀態。
     * @param next 目標狀態；章節／幕間會建立對應的 IChapterState，結局則改設旗標。
     */
    void                           Transition(SemesterState next);
    /**
     * @brief 將逐幀更新轉交目前狀態。
     * @param dt 與上一幀的間隔秒數。
     */
    void                           Update(float dt);

    /**
     * @brief 設定離開幕間市集後要返回的章節。
     * @param next 市集結束後的目標章節。
     *
     * 存在狀態機而非 InterludeMarket 物件上：每次 Transition() 都會重建
     * IChapterState，狀態物件無法跨「章節→幕間→章節」往返保存資料。由進入幕間
     * 的章節寫入、幕間出口閘門讀取。
     */
    void                           SetInterludeReturnTo(SemesterState next) noexcept;
    /// @brief 讀取離開幕間後要返回的章節。
    [[nodiscard]] SemesterState    InterludeReturnTo() const noexcept;

private:
    std::unique_ptr<IChapterState> state_;                                  ///< 目前狀態（結局期間為 nullptr）
    SemesterState                  ending_{SemesterState::Chapter1_AddDrop}; ///< 結局哨兵；僅 inEnding_ 為真時有效
    bool                           inEnding_{false};                        ///< 是否已進入某個結局
    SemesterState                  interludeReturnTo_{SemesterState::Chapter2_Midterms}; ///< 幕間結束後的返回章節，預設第二章
};

} // namespace nccu

#endif // SEMESTER_STATE_MACHINE_H_
