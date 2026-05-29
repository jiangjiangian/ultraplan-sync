#include "app/SceneManager.h"

#include "engine/platform/Harness.h"
#include "engine/platform/Time.h"
#include "engine/render/DrawScope.h"
#include "engine/render/Window.h"
#include "game/world/World.h"

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
            // Phase 3 step 1 has only GameplayScene on the stack so a
            // Push never fires from production code; the branch is
            // here so the doctest fake-scene path can exercise it
            // before step 2 lands TitleScene + the real transitions.
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
            // An empty stack after Replace would dead-end the loop —
            // treat that as Quit so the program never spins idle.
            return stack_.empty() ? StepResult::Quit
                                  : StepResult::Continue;
        case SceneCommand::Kind::Pop:
            if (!stack_.empty()) {
                stack_.back()->Exit();
                stack_.pop_back();
            }
            // The composition root never has more than one scene live
            // in step 1, so a Pop equals "stack empty" = end the run;
            // step 2+ may push Title under Gameplay, in which case
            // Pop returns to Title and the loop keeps going.
            return stack_.empty() ? StepResult::Quit
                                  : StepResult::Continue;
        case SceneCommand::Kind::Quit:
            return StepResult::Quit;
        case SceneCommand::Kind::Restart:
            return StepResult::Restart;
    }
    return StepResult::Continue;
}

SceneManager::RunOutcome SceneManager::Run(nccu::gfx::Window& window,
                                           nccu::gfx::IRenderer& renderer,
                                           nccu::Harness& harness) {
    while (!window.ShouldClose() && !harness.ShouldQuit() &&
           !stack_.empty()) {
        // BeginFrame mirrors the pre-Phase-3 inline loop: the harness
        // captures input edges + drives the scripted plan resolution
        // for this tick BEFORE the active scene's Update reads input.
        harness.BeginFrame();

        const float dt = nccu::gfx::Time::DeltaSeconds();
        SceneCommand cmd = stack_.back()->Update(dt);

        {
            // DrawScope owns the raylib Begin/EndDrawing pair; entering
            // it here means every scene's Draw() runs INSIDE the same
            // GL frame the inline pre-Phase-3 main.cpp used. Closed
            // before EndFrame so the harness sees the post-EndDrawing
            // pixel state.
            nccu::gfx::DrawScope frame;
            stack_.back()->Draw(renderer);
        }

        // EndFrame mirrors the pre-Phase-3 inline call: pass the
        // active scene's World so the harness can serialise
        // {player.pos, karma, money, rain, flags, events, objects}
        // for that tick. Title/Select/Loading scenes (step 2-4) will
        // return nullptr, so EndFrame stays skipped for them — which
        // mirrors pre-Phase-3 main.cpp's behaviour (those screens
        // never called EndFrame either; the harness was inactive
        // until the gameplay scope opened).
        if (const nccu::World* world =
                stack_.back()->WorldForHarnessOrNull()) {
            harness.EndFrame(*world);
        }

        const StepResult res = ApplyCommand(std::move(cmd));
        if (res == StepResult::Quit)    return RunOutcome::Quit;
        if (res == StepResult::Restart) return RunOutcome::Restart;
    }
    // Window-close / harness ShouldQuit / empty stack all funnel into
    // a clean Quit — main.cpp's outer teardown handles GL shutdown.
    return RunOutcome::Quit;
}

} // namespace nccu::app
