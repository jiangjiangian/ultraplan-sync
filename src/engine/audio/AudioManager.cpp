#include "engine/audio/AudioManager.h"

#include "engine/audio/AudioDevice.h"

namespace nccu::audio {

// No-op scaffold. The (bus, device) references are stored so that:
//   - future subscribers (gameplay-event → audio playback) can be
//     installed in this ctor without changing the signature,
//   - the dtor can unsubscribe them under the same RAII guarantee that
//     GameController uses (clear-before-rebuild on Restart so no
//     EventBus handler dangles; see BUGLEDGER B2/H1).
// Today no audio asset exists so no subscription is installed and no
// state needs unwinding.
AudioManager::AudioManager(::EventBus& bus, AudioDevice& device) noexcept
    : bus_(bus), device_(device) {
    (void)bus_;     // referenced via member; suppress unused-private-field
    (void)device_;  // same — both come alive when the body fills in
}

AudioManager::~AudioManager() noexcept = default;

}  // namespace nccu::audio
