#ifndef APP_SCENE_MANAGER_H_
#define APP_SCENE_MANAGER_H_
#include "app/IScene.h"
#include <memory>
#include <vector>

namespace nccu::engine::render { class IRenderer; class Window; }
namespace nccu { class Harness; }

namespace nccu::app {

// Blueprint Phase 3 — minimal scene stack + the ONE app loop. Owns a
// `vector<unique_ptr<IScene>>` (the stack); only the top is active
// each frame. Push/Replace/Pop/Restart/Quit applied AFTER the
// frame's Draw to keep the "never swap mid-frame" invariant the
// blueprint phase-3 risk register calls out.
//
// In Phase 3 step 1 only `GameplayScene` lives on the stack — main.cpp
// still drives Title/Select/Loading via their existing free-function
// blocking loops, then hands GameplayScene to the manager. Step 2-4
// land the matching scenes so main.cpp shrinks to "compose + push
// initial".
//
// Run() is the integration of: nccu::engine::render::Window's per-frame poll +
// nccu::engine::render::DrawScope + Harness BeginFrame/EndFrame + the scene's
// Update/Draw + deferred-command apply. It returns RunOutcome so
// the composition root can decide between Restart (rebuild the
// scene stack from scratch — back to title in Phase 3.2+) and
// Quit (final teardown).
//
// Harness hook mapping (blueprint phase-3 red line — keep
// bit-identical to today's main.cpp loop):
//   MaybeAttach   — stays in main.cpp BEFORE the SceneManager runs.
//   WireEvents    — moves into GameplayScene::Enter()
//                   (scene-scoped subscription lifetime).
//   BeginFrame /
//   EndFrame      — wrap each Run() iteration here.
class SceneManager {
public:
    // RunOutcome lets main.cpp resolve a Restart back into a fresh
    // SceneManager + initial Push, the per-run RAII discipline today's
    // main.cpp uses for the World/View/GameController scope.
    enum class RunOutcome { Quit, Restart };

    SceneManager() = default;
    ~SceneManager() = default;

    SceneManager(const SceneManager&)            = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    // Push a scene onto the stack BEFORE Run is entered. Called from
    // the composition root (main.cpp) with either the harness-driven
    // GameplayScene or, in Phase 3 step 2+, a TitleScene.
    void Push(std::unique_ptr<IScene> scene);

    // The one app loop. Returns when the active scene emits
    // SceneCommand{Quit} or {Restart}, or when the Window/Harness
    // asks to exit. Window is borrowed (not owned) so main.cpp keeps
    // the GL lifetime discipline; renderer is the nccu::engine::render::IRenderer the
    // scenes paint through; harness wraps each frame's Begin/EndFrame
    // — its lifetime spans the program, not the run.
    [[nodiscard]] RunOutcome Run(nccu::engine::render::Window& window,
                                 nccu::engine::render::IRenderer& renderer,
                                 nccu::Harness& harness);

    [[nodiscard]] bool Empty() const noexcept { return stack_.empty(); }

private:
    // Apply the deferred SceneCommand the active scene emitted this
    // frame. Called by Run AFTER Draw / EndFrame. Returns the
    // RunOutcome when the command terminates the loop, or
    // std::nullopt if the loop should keep running.
    enum class StepResult { Continue, Quit, Restart };
    [[nodiscard]] StepResult ApplyCommand(SceneCommand cmd);

    std::vector<std::unique_ptr<IScene>> stack_;
};

} // namespace nccu::app

#endif // APP_SCENE_MANAGER_H_
