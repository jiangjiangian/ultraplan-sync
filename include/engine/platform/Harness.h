#ifndef HARNESS_H_
#define HARNESS_H_
#include <memory>
#include <string>

namespace nccu {

class World;
struct HarnessState;   // 定義於 Harness.cpp

/**
 * @brief 自動遊玩／觀測載具：以腳本與固定步長無頭驅動遊戲，並輸出截圖與狀態紀錄。
 *
 * 預設關閉：環境中沒有 UMBRELLA_SCRIPT 時 MaybeAttach() 回傳未啟用的 Harness，
 * 遊戲逐位元不變。啟用時安裝腳本化輸入來源（可決定、無頭）與固定時間步長，並可
 * 逐幀截圖（在 EndDrawing 後呼叫 raylib TakeScreenshot）與附加一行 JSON 狀態紀錄。
 * 此為「感知＋操作」的接縫，供自動化遊玩與自我迭代之用。
 *
 * 環境變數：
 *   UMBRELLA_SCRIPT     時間軸腳本檔路徑（存在即啟用）
 *   UMBRELLA_SHOTS      frame_*.png 的輸出資料夾（選用）
 *   UMBRELLA_SHOT_EVERY 截圖間隔幀數（預設 30）
 *   UMBRELLA_STATE      JSONL 狀態檔（選用，每幀一行）
 *   UMBRELLA_MAXFRAMES  看門狗自動結束幀數（預設 3600 = 60fps 下 60 秒）
 *   UMBRELLA_SPRITE     自動略過選角時所用的玩家 sprite
 *
 * 腳本語法——同一檔案可交錯使用兩種形式（為附加功能；傳統計時形式逐位元不變）：
 *   傳統計時（行首為幀號）：
 *     <frame> down  <KEY>     自此幀起按住該鍵
 *     <frame> up    <KEY>     自此幀起放開該鍵
 *     <frame> press <KEY>     單幀輕點（下一幀自動放開）
 *     <frame> quit            於此幀乾淨結束
 *   高階計畫動詞（行首為動詞）：
 *     goto <X> <Y>            驅使玩家走向世界座標 (X,Y)
 *     interact <npcId>        走到該 NPC 旁，按 E 直到對話開啟
 *     choose <index>          將選項游標移到 <index> 並確認
 *     advance                 輕點一次對話前進鍵
 *     wait <frames>           可決定的閒置間隔
 *     quit                    抵達時乾淨結束
 * KEY 可為：A..Z, Enter, Escape, Tab, Space, Up, Down, Left, Right。
 * 計畫動詞每幀依「上一個 EndFrame 擷取的 World 快照」解析（純函式 => 可決定重播）。
 * 有計畫時，最後一個動詞完成後該局也會結束。
 */
class Harness {
public:
    /** @brief 建構一個未啟用的載具。 */
    Harness();
    ~Harness();
    Harness(Harness&&) noexcept;
    Harness& operator=(Harness&&) noexcept;
    Harness(const Harness&)            = delete;
    Harness& operator=(const Harness&) = delete;

    /** @brief 載具是否已啟用。@return 有腳本而啟用時回傳 true。 */
    [[nodiscard]] bool Active() const noexcept;
    /**
     * @brief 是否應結束本局。
     * @return 腳本下達 quit 或 maxframes 看門狗觸發後回傳 true。
     *
     * 主迴圈把此結果 OR 進其結束條件，使無頭執行無須人為關閉視窗即可終止。
     */
    [[nodiscard]] bool ShouldQuit() const noexcept;
    /**
     * @brief 取得固定的玩家 sprite 路徑，使啟用的執行完全略過互動式選角（可決定）。
     * @return 啟用時回傳 sprite 路徑；未啟用時回傳空字串。
     */
    [[nodiscard]] std::string SpritePath() const;

    /**
     * @brief 向 EventBus 訂閱以收集事件。
     *
     * 須於 GameController 建構之後呼叫，以維持解構順序安全（控制器解構時會 Clear
     * 匯流排）。
     */
    void WireEvents();

    /** @brief 推進腳本一幀；於 Update() 之前呼叫。 */
    void BeginFrame();
    /**
     * @brief 截圖並輸出狀態；於 EndDrawing() 之後呼叫。
     * @param[in] world 本幀更新後的世界（唯讀擷取供下一幀計畫解析）。
     */
    void EndFrame(const World& world);

private:
    std::unique_ptr<HarnessState> s_;  ///< 跨移動保持穩定（EventBus 捕捉指向它）

    friend Harness MaybeAttach();
};

/** @brief 讀取環境變數並回傳已啟用或未啟用的 Harness。@return 視 UMBRELLA_SCRIPT 而定的載具。 */
Harness MaybeAttach();

} // namespace nccu

#endif // HARNESS_H_
