#ifndef ENGINE_AUDIO_AUDIO_MANAGER_H_
#define ENGINE_AUDIO_AUDIO_MANAGER_H_

#include "engine/events/EventBus.h"  // global ::EventBus singleton

namespace nccu::audio {

class AudioDevice;

// Per-run audio orchestrator. Owns the EventBus subscriptions that turn
// gameplay events (item purchased / chapter transitioned / karma swung /
// rain started / ending shown / …) into audio playback. Today the body
// is a no-op scaffold:
//   - no events are subscribed yet (no audio assets exist),
//   - the class shape exists so that the per-run owner (GameController
//     or future SceneManager) already has a hook to construct an
//     AudioManager next to the World/View/GameController triple.
//
// Lifetime model mirrors GameController (BUGLEDGER B2/H1):
//   - subscribers will be installed in the ctor,
//   - the dtor unsubscribes them BEFORE the next per-run scope builds
//     (avoids the dangling-handler / flag-pollution-across-restart
//     class of bug that already bit the controller).
//
// Non-copyable / non-movable: one manager per run, anchored by the
// owning controller's scope.
class AudioManager {
public:
    explicit AudioManager(::EventBus& bus, AudioDevice& device) noexcept;
    ~AudioManager() noexcept;

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

private:
    ::EventBus&  bus_;
    AudioDevice& device_;
};

}  // namespace nccu::audio

#endif  // ENGINE_AUDIO_AUDIO_MANAGER_H_
