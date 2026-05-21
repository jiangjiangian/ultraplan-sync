#ifndef HARNESS_H_
#define HARNESS_H_
#include <memory>
#include <string>

namespace nccu {

class World;
struct HarnessState;   // defined in Harness.cpp

// Autoplay / observe harness. Off by default: with no UMBRELLA_SCRIPT in
// the environment MaybeAttach() returns an inactive Harness and the game
// is bit-for-bit unchanged. When active it installs a scripted input
// source (deterministic, headless) and a fixed timestep, then per frame
// can screenshot (raylib TakeScreenshot, after EndDrawing) and append a
// one-line JSON state record. This is the perception+actuation seam the
// agent team uses to "play" the game and self-iterate.
//
// Environment:
//   UMBRELLA_SCRIPT     timeline file path (presence => active)
//   UMBRELLA_SHOTS      dir for frame_*.png (optional)
//   UMBRELLA_SHOT_EVERY screenshot cadence in frames (default 30)
//   UMBRELLA_STATE      JSONL state file (optional, one line per frame)
//   UMBRELLA_MAXFRAMES  watchdog auto-quit (default 3600 = 60s @ 60fps)
//   UMBRELLA_SPRITE     player sprite for the auto-bypassed char-select
//
// Script grammar — TWO interleavable forms in the same file (additive;
// the classic timed form is byte-for-byte unchanged):
//   classic timed (line starts with a frame number):
//     <frame> down  <KEY>     key held from this frame on
//     <frame> up    <KEY>     key released from this frame on
//     <frame> press <KEY>     one-frame tap (auto-released next frame)
//     <frame> quit            end the run cleanly at this frame
//   high-level plan verbs (line starts with the verb word):
//     goto <X> <Y>            drive the player toward world (X,Y)
//     interact <npcId>        walk adjacent to that NPC, E until dialog
//     choose <index>          move the choice cursor to <index>, confirm
//     advance                 tap the dialog-advance key once
//     wait <frames>           deterministic idle spacer
//     quit                    end the run cleanly once reached
// KEY: A..Z, Enter, Escape, Tab, Space, Up, Down, Left, Right.
// Plan verbs are resolved each frame against the World snapshot captured
// at the previous EndFrame (pure function => deterministic replay). When
// a plan exists, the run also ends once the last verb completes.
class Harness {
public:
    Harness();                                   // inactive
    ~Harness();
    Harness(Harness&&) noexcept;
    Harness& operator=(Harness&&) noexcept;
    Harness(const Harness&)            = delete;
    Harness& operator=(const Harness&) = delete;

    [[nodiscard]] bool Active() const noexcept;
    // True once the script said `quit` or the maxframes watchdog fired.
    // The main loop ORs this into its exit condition so a headless run
    // terminates without a human closing the window.
    [[nodiscard]] bool ShouldQuit() const noexcept;
    // Fixed sprite path so an active run skips the interactive
    // character-select entirely (deterministic). Empty when inactive.
    [[nodiscard]] std::string SpritePath() const;

    // Subscribe to the EventBus. Call AFTER GameController is constructed
    // so teardown ordering (controller dtor Clears the bus) stays safe.
    void WireEvents();

    void BeginFrame();                // advance the script; call before Update()
    void EndFrame(const World& world); // screenshot + state; call after EndDrawing()

private:
    std::unique_ptr<HarnessState> s_;  // stable across moves (EventBus capture)

    friend Harness MaybeAttach();
};

// Reads the environment and returns an active or inactive Harness.
Harness MaybeAttach();

} // namespace nccu

#endif // HARNESS_H_
