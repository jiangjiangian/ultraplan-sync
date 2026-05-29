#include "engine/audio/AudioManager.h"

#include "engine/audio/AudioDevice.h"

namespace nccu::audio {

// 目前為空殼骨架。保留 (bus, device) 參考的用意：
//   - 日後的訂閱者（玩法事件 → 音效播放）可直接在此建構式安裝，
//     而不必更動簽章，
//   - 解構式能在相同的 RAII 保證下取消訂閱，與 GameController 一致
//     （重新開始時先清空再重建，確保沒有 EventBus handler 變成懸空）。
// 現階段尚無任何音效資產，故不安裝訂閱，也沒有狀態需要回收。
AudioManager::AudioManager(::EventBus& bus, AudioDevice& device) noexcept
    : bus_(bus), device_(device) {
    (void)bus_;     // 透過成員參照；抑制「未使用私有欄位」警告
    (void)device_;  // 同上——待實作填入後兩者才會真正啟用
}

AudioManager::~AudioManager() noexcept = default;

}  // namespace nccu::audio
