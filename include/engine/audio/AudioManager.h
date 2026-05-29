#ifndef ENGINE_AUDIO_AUDIO_MANAGER_H_
#define ENGINE_AUDIO_AUDIO_MANAGER_H_

#include "engine/events/EventBus.h"  // 全域 ::EventBus 單例

namespace nccu::audio {

class AudioDevice;

/**
 * @brief 單局音訊調度器：把遊戲事件轉成音訊播放。
 *
 * 持有 EventBus 訂閱，將遊戲事件（購買道具／章節切換／karma 變動／開始下雨／
 * 顯示結局／…）轉成音訊播放。目前函式體為空殼：
 *   - 尚未訂閱任何事件（無音訊素材），
 *   - 保留類別形狀，使單局擁有者（GameController 或未來的 SceneManager）已有
 *     掛點，可在 World／View／GameController 三件組旁一併建構 AudioManager。
 *
 * 生命週期模型比照 GameController：訂閱於建構子安裝，並在下一個單局範圍建立
 * 之前於解構子取消訂閱，避免懸空 handler／重開時旗標污染這類曾困擾控制器的問題。
 *
 * 不可複製、不可移動：每局一個管理器，錨定於擁有它的控制器範圍。
 */
class AudioManager {
public:
    /**
     * @brief 以事件匯流排與音訊裝置建構單局音訊調度器。
     * @param[in] bus    用以訂閱遊戲事件的匯流排。
     * @param[in] device 音訊裝置控制代碼。
     */
    explicit AudioManager(::EventBus& bus, AudioDevice& device) noexcept;
    /** @brief 解構；將於此取消所有事件訂閱。 */
    ~AudioManager() noexcept;

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

private:
    ::EventBus&  bus_;      ///< 訂閱事件的來源匯流排
    AudioDevice& device_;   ///< 音訊裝置控制代碼
};

}  // namespace nccu::audio

#endif  // ENGINE_AUDIO_AUDIO_MANAGER_H_
