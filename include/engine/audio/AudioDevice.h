#ifndef ENGINE_AUDIO_AUDIO_DEVICE_H_
#define ENGINE_AUDIO_AUDIO_DEVICE_H_

namespace nccu::audio {

/**
 * @brief 引擎音訊裝置的 RAII 控制代碼。
 *
 * 目前函式體為空殼——專案尚未含任何音訊素材，故不呼叫 raylib InitAudioDevice()、
 * 不佔用任何 GPU／音效狀態。保留完整類別形狀的理由：
 *   1. main() 的解構順序已預留一個音訊資源（與 Window 一同建構、在其之前解構），
 *      與 GPU 資源的紀律一致（ShutdownTextureCache／ShutdownFont 須早於
 *      Window 解構→::CloseWindow）。
 *   2. 當第一個音訊素材（音效／音樂）加入時，建構子改為呼叫 InitAudioDevice()、
 *      解構子在同一 RAII 保證下呼叫 CloseAudioDevice()——無須重整 main()。
 *   3. 無視窗／測試執行保持可決定性：空殼建構子沒有任何 PRNG／計時副作用，故
 *      自動遊玩的對照輸出維持逐位元一致。
 *
 * 不可複製、不可移動：每個程序只有單一裝置。於 main() 建構一次，由 RAII 在堆疊
 * 展開時關閉它。
 */
class AudioDevice {
public:
    /** @brief 建構音訊裝置控制代碼（目前為空殼，不啟用裝置）。 */
    AudioDevice() noexcept;
    /** @brief 解構；裝置真正啟用後將於此呼叫 CloseAudioDevice()。 */
    ~AudioDevice() noexcept;

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;
    AudioDevice(AudioDevice&&) = delete;
    AudioDevice& operator=(AudioDevice&&) = delete;

    /**
     * @brief 裝置是否已啟用。
     * @return InitAudioDevice() 確實被呼叫後回傳 true；目前空殼恆回傳 false。
     *
     * 日後啟用時翻為 true，不影響只判斷 Ready() 的呼叫點。
     */
    bool Ready() const noexcept { return ready_; }

private:
    bool ready_ = false;   ///< 裝置是否已啟用
};

}  // namespace nccu::audio

#endif  // ENGINE_AUDIO_AUDIO_DEVICE_H_
