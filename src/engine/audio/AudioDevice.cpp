#include "engine/audio/AudioDevice.h"

namespace nccu::audio {

// No-op scaffold. When the first audio asset arrives, the ctor will
// call ::InitAudioDevice() and set ready_ = true; the dtor will mirror
// that with ::CloseAudioDevice() iff Ready() is true. The header
// contract (non-copyable, non-movable, RAII, Ready() accessor) already
// matches that future activation — no main() or call-site change
// needed when the body fills in.
AudioDevice::AudioDevice() noexcept = default;
AudioDevice::~AudioDevice() noexcept = default;

}  // namespace nccu::audio
