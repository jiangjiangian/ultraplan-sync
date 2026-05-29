#ifndef APP_ISCENE_H_
#define APP_ISCENE_H_
#include <functional>
#include <memory>

namespace nccu::gfx { class IRenderer; }
namespace nccu { class World; }       // harness-snapshot accessor — see below

namespace nccu::app {

class IScene;

// Blueprint Phase 3 — the app-layer Scene control message. A scene
// returns a SceneCommand from each Update() tick; the SceneManager
// applies it AFTER the frame's Draw() (mirrors the deferred-deletion
// red line: never swap the active scene mid-frame). Until step 2-4
// land the matching Title/Select/Loading scenes, only `None` and
// `Quit` fire — Push/Pop/Replace are pre-wired so step 2 only adds
// the scene classes, not the manager surface.
//
// `make` is a thunk that allocates the next scene on demand — keeps
// the scene class definitions out of IScene.h so a new scene type
// landing in step 2/3 needs no edit here. Tests can construct a
// SceneCommand{Push, []{ return std::make_unique<FakeScene>(); }}
// without dragging in the production scene files.
struct SceneCommand {
    enum class Kind { None, Replace, Push, Pop, Quit, Restart };
    Kind kind = Kind::None;
    std::function<std::unique_ptr<IScene>()> make;
};

// Blueprint Phase 3 — uniform Scene contract. Enter/Exit are non-pure
// (default no-op) so a leaf scene can omit them when it has nothing
// to wire on attach. Update reads input + drives the model; Draw
// renders through the abstract IRenderer so a headless doctest can
// exercise the scene flow without a real GL context — the testability
// payoff the blueprint phase-3 audit promises.
//
// The InputSource type is intentionally erased here (every scene
// reads input through the same process-wide gfx::Input seam the
// harness already manipulates with SetSource); a future evolution
// can swap the singleton call for an `InputSource&` arg per the
// blueprint's design without changing IScene's surface.
class IScene {
public:
    virtual ~IScene() = default;

    // One-shot wire-up at the moment the scene becomes the active top
    // of the stack. Override to capture references / subscribe to
    // events. Default no-op.
    virtual void Enter() {}

    // Per-frame model-advance tick. dt is seconds since the previous
    // frame, sourced from nccu::engine::platform::Time. Returns the SceneCommand the
    // SceneManager applies AFTER this frame's Draw — never swap
    // mid-frame. SceneCommand{Kind::None} keeps the scene active.
    [[nodiscard]] virtual SceneCommand Update(float dt) = 0;

    // Per-frame render pass. Called between gfx::DrawScope's
    // BeginDrawing/EndDrawing — the scene paints into the provided
    // renderer. Pure so a leaf must implement it (a scene without
    // visible output is non-sensical; LoadingScene's "warm-up"
    // still emits a clear).
    virtual void Draw(nccu::gfx::IRenderer& renderer) = 0;

    // Counterpart to Enter — unsubscribe, drop refs. Default no-op.
    virtual void Exit() {}

    // Harness-snapshot accessor. The autoplay harness's EndFrame hook
    // needs `const World&` to serialise the player + flags + events
    // into state.jsonl for that frame. Most scenes (Title/Select/
    // Loading) have no World and return nullptr; SceneManager::Run
    // then skips the EndFrame call (matching pre-Phase-3 behaviour —
    // those screens never participated in the harness either).
    // GameplayScene overrides to return its owned world.
    [[nodiscard]] virtual const World*
    WorldForHarnessOrNull() const noexcept { return nullptr; }
};

} // namespace nccu::app

#endif // APP_ISCENE_H_
