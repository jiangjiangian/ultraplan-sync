#ifndef APP_SCENE_MANAGER_H_
#define APP_SCENE_MANAGER_H_
#include "app/IScene.h"
#include <memory>
#include <vector>

namespace nccu::engine::render { class IRenderer; class Window; }
namespace nccu { class Harness; }

namespace nccu::app {

/**
 * @file SceneManager.h
 * @brief 場景狀態機核心：最小場景堆疊 ＋ 全程式唯一的應用主迴圈。
 */

/**
 * @brief 持有場景堆疊並驅動唯一主迴圈的場景管理器。
 *
 * 內含一個 `vector<unique_ptr<IScene>>` 作為堆疊，每幀只有頂端場景為現役。
 * Push／Replace／Pop／Restart／Quit 一律於「該幀 Draw 之後」才套用，以維持
 * 「絕不在幀中途抽換場景」的不變式。
 *
 * Run() 把以下環節整合為一：Window 的每幀輪詢、DrawScope、自動錄製的
 * BeginFrame／EndFrame、場景的 Update／Draw，以及延後指令的套用。它回傳
 * RunOutcome，讓 composition root 得以區分 Restart（從頭重建場景堆疊，回到標題）
 * 與 Quit（最終拆除）。
 *
 * 自動錄製鉤子的歸屬（須與主迴圈逐位元一致）：
 *   - MaybeAttach：留在 main.cpp，於 SceneManager 執行之前。
 *   - WireEvents ：移入 GameplayScene::Enter()（訂閱生命週期綁定場景）。
 *   - BeginFrame／EndFrame：包覆此處每一次 Run() 迭代。
 */
class SceneManager {
public:
    /**
     * @brief 一次 Run 的結束結果。
     *
     * 讓 main.cpp 能將 Restart 收斂為「重建一個全新 SceneManager ＋ 初始 Push」，
     * 正是 World／View／GameController 範圍所仰賴的每回合 RAII 紀律。
     */
    enum class RunOutcome { Quit, Restart };

    SceneManager() = default;
    ~SceneManager() = default;

    SceneManager(const SceneManager&)            = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    /**
     * @brief 於進入 Run 之前，將一個場景推入堆疊。
     * @param scene 要接管的場景；空指標會被忽略。
     *
     * 由 composition root（main.cpp）呼叫，傳入錄製驅動的 GameplayScene
     * 或人類遊玩路徑起點的 TitleScene。
     */
    void Push(std::unique_ptr<IScene> scene);

    /**
     * @brief 全程式唯一的應用主迴圈。
     * @param window   借用（非擁有）的視窗，GL 生命週期仍由 main.cpp 把關。
     * @param renderer 場景作畫所經的抽象渲染器。
     * @param harness  包覆每幀 Begin／EndFrame 的自動錄製器；其生命週期橫跨整個
     *                 程式，而非單一回合。
     * @return 現役場景發出 Quit／Restart，或視窗／錄製器要求結束時回傳。
     */
    [[nodiscard]] RunOutcome Run(nccu::engine::render::Window& window,
                                 nccu::engine::render::IRenderer& renderer,
                                 nccu::Harness& harness);

    /** @brief 堆疊是否為空（無任何場景）。 */
    [[nodiscard]] bool Empty() const noexcept { return stack_.empty(); }

private:
    /**
     * @brief 套用現役場景本幀發出的延後 SceneCommand。
     * @param cmd 本幀待套用的場景切換指令。
     * @return 指令終止迴圈時回傳對應結果，否則回傳 Continue 讓迴圈續跑。
     *
     * 由 Run 在 Draw／EndFrame 之後呼叫。
     */
    enum class StepResult { Continue, Quit, Restart };
    [[nodiscard]] StepResult ApplyCommand(SceneCommand cmd);

    std::vector<std::unique_ptr<IScene>> stack_;  ///< 場景堆疊；僅頂端為現役
};

} // namespace nccu::app

#endif // APP_SCENE_MANAGER_H_
