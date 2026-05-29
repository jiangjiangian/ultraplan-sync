#ifndef APP_SCENE_BOOTSTRAP_H_
#define APP_SCENE_BOOTSTRAP_H_

// Screen-flow assembly for the composition root. Keeping this out of
// main.cpp lets main() stay a thin composition root (window / font / audio
// / teardown) and confines the harness-vs-human branching + the scene
// factory chain to one place.
namespace nccu { class Harness; }
namespace nccu::audio { class AudioDevice; }

namespace nccu::app {

class SceneManager;

// Build the initial scene and push it onto `sm`:
//   - harness active : skip straight to one deterministic GameplayScene
//     (no Title/Select; empty restart factory => the never-restart
//     contract a scripted run relies on).
//   - human play     : Loading -> Title -> CharacterSelect -> Gameplay,
//     where the in-game 重新開始 rebuilds the whole chain from a fresh
//     Loading scene.
// `harness` and `audio` are BORROWED (owned by main, outlive the run);
// `winW`/`winH` size the per-run World/View.
void PushInitialScene(SceneManager& sm, nccu::Harness& harness,
                      nccu::audio::AudioDevice& audio, int winW, int winH);

} // namespace nccu::app

#endif // APP_SCENE_BOOTSTRAP_H_
