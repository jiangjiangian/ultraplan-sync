#ifndef ENGINE_AUDIO_AUDIO_DEVICE_H_
#define ENGINE_AUDIO_AUDIO_DEVICE_H_

namespace nccu::audio {

// RAII handle for the engine's audio device. Today the body is a no-op
// scaffold — the project ships no audio assets yet, so no raylib
// InitAudioDevice() is called and no GPU/sound state is taken. The
// class shape is real so that:
//   1. main()'s teardown order already accommodates an audio resource
//      (constructed alongside Window, destroyed before it), which
//      mirrors the GPU-resource discipline (ShutdownTextureCache /
//      ShutdownFont before Window::dtor → ::CloseWindow).
//   2. When the first audio asset (sfx/music) lands, the ctor flips on
//      InitAudioDevice() and the dtor calls CloseAudioDevice() under
//      the same RAII guarantee — no main() restructure.
//   3. Headless / harness runs stay deterministic: a no-op ctor takes
//      no PRNG/timing side effects, so the playtest oracle remains
//      byte-identical.
//
// Non-copyable / non-movable: a single device per process. Construct
// once in main(), let RAII close it on stack unwind.
class AudioDevice {
public:
    AudioDevice() noexcept;
    ~AudioDevice() noexcept;

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;
    AudioDevice(AudioDevice&&) = delete;
    AudioDevice& operator=(AudioDevice&&) = delete;

    // True once InitAudioDevice() has actually been called and the device
    // is live. The no-op scaffold returns false; future activation flips
    // this on without touching call-sites that only branch on Ready().
    bool Ready() const noexcept { return ready_; }

private:
    bool ready_ = false;
};

}  // namespace nccu::audio

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_H_
