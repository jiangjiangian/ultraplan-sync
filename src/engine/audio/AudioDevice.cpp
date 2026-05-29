#include "engine/audio/AudioDevice.h"

namespace nccu::audio {

// 空殼。第一個音訊素材加入時，建構子將呼叫 ::InitAudioDevice() 並設 ready_ = true，
// 解構子則在 Ready() 為真時對稱地呼叫 ::CloseAudioDevice()。標頭契約（不可複製、
// 不可移動、RAII、Ready() 存取器）已與該未來啟用相符——函式體補上時不需更動 main()
// 或任何呼叫點。
AudioDevice::AudioDevice() noexcept = default;
AudioDevice::~AudioDevice() noexcept = default;

}  // namespace nccu::audio
