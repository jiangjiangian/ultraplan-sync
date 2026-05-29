#ifndef APP_SCENE_BOOTSTRAP_H_
#define APP_SCENE_BOOTSTRAP_H_

/**
 * @file SceneBootstrap.h
 * @brief composition root 的「畫面流程」組裝點：把場景工廠鏈與測試／人類分支
 *        集中於一處，讓 main() 維持為精簡的 composition root。
 *
 * 將這段組裝抽離 main.cpp，使 main() 只需負責視窗／字型／音訊／拆除等最小職責，
 * 而把「測試錄製 vs 人類遊玩」的分支與整條場景工廠鏈封裝在單一檔案。
 */
namespace nccu { class Harness; }
namespace nccu::audio { class AudioDevice; }

namespace nccu::app {

class SceneManager;

/**
 * @brief 建立初始場景並推入 `sm`。
 * @param sm      要接收初始場景的場景管理器。
 * @param harness 借用（非擁有）的自動錄製器，其生命週期由 main 把關並橫跨整個程式。
 * @param audio   借用（非擁有）的音訊裝置，同樣由 main 擁有、壽命長於本次執行。
 * @param winW    本次執行 World／View 的視窗寬度。
 * @param winH    本次執行 World／View 的視窗高度。
 *
 * 兩條路徑：
 *   - 錄製器啟用：直接跳進單一且具決定性的 GameplayScene（略過 Title／Select；
 *     restart 工廠留空 => 腳本化執行所仰賴的「永不重啟」契約）。
 *   - 人類遊玩：Loading -> Title -> CharacterSelect -> Gameplay，其中遊戲內的
 *     「重新開始」會從全新的 Loading 場景重建整條鏈。
 */
void PushInitialScene(SceneManager& sm, nccu::Harness& harness,
                      nccu::audio::AudioDevice& audio, int winW, int winH);

} // namespace nccu::app

#endif // APP_SCENE_BOOTSTRAP_H_
