#ifndef SCRIPT_INPUT_H_
#define SCRIPT_INPUT_H_
#include "engine/input/Input.h"

#include <istream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nccu {

class World;  // 前向宣告：高階計畫以唯讀方式讀取 World

/**
 * @brief 可決定的 InputSource：重播腳本化的按鍵時間軸，使遊戲可無頭且可重現地驅動。
 *
 * 同一檔案支援兩種文法、可自由交錯（某行的第一個 token 非數字即視為高階動詞，
 * 否則為傳統計時指令——完全向後相容）：
 *
 * A) 傳統計時指令（每行一條；'#'／空行略過；60 fps）：
 *      <frame> down  <KEY>     自此幀起按住
 *      <frame> up    <KEY>     自此幀起放開
 *      <frame> press <KEY>     單幀輕點（下一幀自動放開）
 *      <frame> quit            於此幀請求乾淨結束
 *    KEY 可為：A..Z, Enter, Escape, Tab, Space, Up, Down, Left, Right。
 *    邊緣語意比照 raylib：IsPressed 僅在按下幀為真、IsReleased 僅在放開幀為真、
 *    其間每幀 IsDown 為真。
 *
 * B) 高階計畫動詞（無前置幀號；依檔案順序執行、同時僅一個生效，每個動詞每幀被
 *    編譯成合成按鍵邊緣，使時間軸無須逐幀手算即穩健）：
 *      goto <X> <Y>        以既有的 3 px/幀軸向移動驅使玩家走向世界座標 (X,Y)，
 *                          進入小 epsilon 範圍內即完成。為玩家位置的純函式。
 *      interact <npcId>    查出該 NPC 當前 World 位置，goto 至相鄰使 24x24 碰撞盒
 *                          重疊，再按住 E 直到 World 的 dialog.active 為真。
 *      choose <index>      在選項選單開啟時，以 Up/Down 將游標移到 <index>，再以
 *                          E 確認。
 *      advance             輕點對話前進鍵（E），直到該行／頁前進或對話框關閉
 *                          （有界）。
 *      wait <frames>       可決定的間隔：閒置 <frames> 幀。
 *      quit                抵達時乾淨結束本局。
 *
 * 可決定性：計畫每幀由 ResolvePlan(World) 解析，且為「當前計畫步驟 + 載具於上一個
 * EndFrame 擷取的 World 快照」的純函式，不依賴牆鐘時間、無亂數。相同腳本 + 相同
 * 模擬 => 相同合成邊緣 => 各次執行的 state.jsonl 逐位元一致。Advance() 剛好步進一幀，
 * 須在讀取輸入前每幀呼叫一次；載具隨後、在 GameController 讀輸入前呼叫 ResolvePlan()。
 */
class ScriptInput final : public nccu::engine::input::InputSource {
public:
    /**
     * @brief 從輸入串流解析腳本（兩種文法可交錯）。
     * @param[in] in 腳本內容串流。
     */
    void Load(std::istream& in);
    /**
     * @brief 從檔案載入腳本。
     * @param[in] path 腳本檔路徑。
     */
    void LoadFile(const std::string& path);

    /** @brief 步進傳統時間軸一幀；每個遊戲幀呼叫一次。 */
    void Advance();
    /**
     * @brief 依 World 快照解析當前高階計畫步驟，注入本幀的合成按鍵邊緣。
     * @param[in] world 唯讀世界快照；可為 null（如第一個 World 快照之前），此時計畫閒置。
     *
     * 每幀呼叫一次，於 Advance() 之後、控制器讀輸入之前。
     */
    void ResolvePlan(const World* world);

    /** @brief 腳本是否請求結束。@return 已下達 quit 時回傳 true。 */
    [[nodiscard]] bool WantsQuit() const noexcept { return quit_; }
    /**
     * @brief 腳本是否宣告了任何高階計畫動詞。
     * @return 含計畫動詞回傳 true。
     *
     * 純傳統腳本沒有計畫動詞——載具須僅以 quit／maxframes 管控，絕不自動結束它們。
     */
    [[nodiscard]] bool HasPlan() const noexcept { return !plan_.empty(); }
    /**
     * @brief 是否所有高階計畫動詞皆已完成。
     * @return 全部完成回傳 true。
     *
     * 僅在 HasPlan() 為真時有意義：讓計畫驅動的執行在最後一個動詞完成後即可結束，
     * 無須手放 quit／maxframes。
     */
    [[nodiscard]] bool PlanDone() const noexcept {
        return planPc_ >= plan_.size();
    }

    bool IsDown(nccu::engine::input::Key k)     const noexcept override;
    bool IsPressed(nccu::engine::input::Key k)  const noexcept override;
    bool IsReleased(nccu::engine::input::Key k) const noexcept override;

private:
    struct Directive { enum Kind { Down, Up, Press, Quit } kind; int key; };

    enum class Verb { Goto, Interact, Choose, Advance, Wait, Quit };
    struct Step {
        Verb        verb;
        float       x = 0.0f, y = 0.0f;   ///< Goto 目標座標
        std::string arg;                  ///< Interact 的 npcId
        int         n = 0;                ///< Choose 的 index／Wait 的幀數
    };

    // 把單一合成按鍵邊緣套用到當前按鍵集合（比照傳統路徑所用的 raylib 邊緣規則），
    // 使動詞所產生的按鍵與腳本按下的按鍵對 GameController／Player 無從區別。
    void SynthDown(int key);
    void SynthUp(int key);
    void SynthPress(int key);

    // --- 傳統時間軸 -------------------------------------------------------
    std::unordered_map<int, std::vector<Directive>> byFrame_;
    std::vector<int> autoUp_;
    int  frame_ = -1;

    // --- 共用按鍵狀態（傳統與計畫皆寫入此處）-----------------------------
    std::unordered_set<int> down_, pressed_, released_;
    bool quit_ = false;

    // --- 高階計畫 ---------------------------------------------------------
    std::vector<Step> plan_;
    std::size_t       planPc_       = 0;     ///< plan_ 的程式計數器
    int               planSub_      = 0;     ///< 各動詞的子階段
    int               planWatchdog_ = 0;     ///< 各動詞的有界進度看門狗
};

} // namespace nccu

#endif // SCRIPT_INPUT_H_
