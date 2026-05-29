#include "app/SceneManager.h"

#include "engine/platform/Harness.h"
#include "engine/platform/Time.h"
#include "engine/render/DrawScope.h"
#include "engine/render/Window.h"
#include "game/world/World.h"

/**
 * @file SceneManager.cpp
 * @brief 場景管理器的實作：延後套用的場景切換語意，與整合錄製鉤子的唯一主迴圈。
 */

namespace nccu::app {

void SceneManager::Push(std::unique_ptr<IScene> scene) {
    if (!scene) return;
    scene->Enter();
    stack_.push_back(std::move(scene));
}

SceneManager::StepResult SceneManager::ApplyCommand(SceneCommand cmd) {
    switch (cmd.kind) {
        case SceneCommand::Kind::None:
            return StepResult::Continue;
        case SceneCommand::Kind::Push:
            // 正式遊玩流程目前不會發出 Push；此分支存在是為了讓 doctest 的假場景
            // 路徑能驗證推入語意。
            if (cmd.make) {
                auto next = cmd.make();
                if (next) {
                    next->Enter();
                    stack_.push_back(std::move(next));
                }
            }
            return StepResult::Continue;
        case SceneCommand::Kind::Replace:
            if (!stack_.empty()) {
                stack_.back()->Exit();
                stack_.pop_back();
            }
            if (cmd.make) {
                auto next = cmd.make();
                if (next) {
                    next->Enter();
                    stack_.push_back(std::move(next));
                }
            }
            // Replace 後若堆疊為空會讓迴圈走入死路——視同 Quit，使程式不會空轉。
            return stack_.empty() ? StepResult::Quit
                                  : StepResult::Continue;
        case SceneCommand::Kind::Pop:
            if (!stack_.empty()) {
                stack_.back()->Exit();
                stack_.pop_back();
            }
            // Pop 後若堆疊清空即結束本次執行；若堆疊下方仍有場景（例如 Title 被壓在
            // Gameplay 之下），則 Pop 回到該場景並讓迴圈續跑。
            return stack_.empty() ? StepResult::Quit
                                  : StepResult::Continue;
        case SceneCommand::Kind::Quit:
            return StepResult::Quit;
        case SceneCommand::Kind::Restart:
            return StepResult::Restart;
    }
    return StepResult::Continue;
}

SceneManager::RunOutcome SceneManager::Run(nccu::engine::render::Window& window,
                                           nccu::engine::render::IRenderer& renderer,
                                           nccu::Harness& harness) {
    while (!window.ShouldClose() && !harness.ShouldQuit() &&
           !stack_.empty()) {
        // BeginFrame 必須在現役場景的 Update 讀輸入之前：錄製器於此擷取輸入邊緣，
        // 並推進本 tick 的腳本化計畫解析。
        harness.BeginFrame();

        const float dt = nccu::engine::platform::Time::DeltaSeconds();
        SceneCommand cmd = stack_.back()->Update(dt);

        {
            // DrawScope 持有 raylib 的 Begin／EndDrawing 配對；在此進入即代表每個
            // 場景的 Draw() 都跑在同一個 GL 幀內。於 EndFrame 前關閉，使錄製器看到
            // EndDrawing 之後的像素狀態。
            nccu::engine::render::DrawScope frame;
            stack_.back()->Draw(renderer);
        }

        // EndFrame 傳入現役場景的 World，讓錄製器序列化本 tick 的
        // {player.pos, karma, money, rain, flags, events, objects}。Title／Select／
        // Loading 場景回傳 nullptr，故對它們略過 EndFrame——這些畫面本就不參與錄製。
        if (const nccu::World* world =
                stack_.back()->WorldForHarnessOrNull()) {
            harness.EndFrame(*world);
        }

        const StepResult res = ApplyCommand(std::move(cmd));
        if (res == StepResult::Quit)    return RunOutcome::Quit;
        if (res == StepResult::Restart) return RunOutcome::Restart;
    }
    // 視窗關閉／錄製器 ShouldQuit／堆疊清空都收斂為乾淨的 Quit——GL 關閉由 main.cpp
    // 的外層拆除負責。
    return RunOutcome::Quit;
}

} // namespace nccu::app
