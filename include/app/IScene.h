#ifndef APP_ISCENE_H_
#define APP_ISCENE_H_
#include <functional>
#include <memory>

namespace nccu::engine::render { class IRenderer; }
namespace nccu { class World; }       // 前向宣告——供測試錄製存取器使用（見下方 WorldForHarnessOrNull）

namespace nccu::app {

class IScene;

/**
 * @file IScene.h
 * @brief app 層的場景契約與場景控制訊息：所有畫面（Loading／Title／
 *        CharacterSelect／Gameplay）共用的統一介面，以及場景間切換的指令型別。
 */

/**
 * @brief 場景控制訊息：場景每次 Update() 回傳一個 SceneCommand 告知 SceneManager
 *        下一步該如何切換場景。
 *
 * SceneManager 在「該幀 Draw() 之後」才套用此指令，沿用延後刪除的不變式
 * （絕不在幀中途抽換現役場景，避免狀態錯亂）。Replace／Push／Pop／Restart／Quit
 * 皆已預先接好，新增場景型別時無需改動本結構。
 */
struct SceneCommand {
    enum class Kind { None, Replace, Push, Pop, Quit, Restart };
    Kind kind = Kind::None;
    /// @brief 延後配置下一個場景的工廠 thunk。
    ///
    /// 以 closure 形式按需建立下一個場景，讓具體場景類別的定義不必出現在
    /// IScene.h——既避免循環相依，測試也能直接塞入
    /// `SceneCommand{Push, []{ return std::make_unique<FakeScene>(); }}`
    /// 而不必拖入正式場景檔。
    std::function<std::unique_ptr<IScene>()> make;
};

/**
 * @brief 統一的場景契約：每個畫面都實作此介面，由 SceneManager 以單一迴圈驅動。
 *
 * Enter／Exit 為非純虛擬（預設空實作），葉場景若在掛載時無事可接便可省略覆寫。
 * Update 讀輸入並推進模型；Draw 透過抽象的 IRenderer 繪製，使無頭 doctest
 * 不需真實 GL context 即可驅動整套場景流程——這正是抽象渲染介面換來的可測試性。
 *
 * 輸入來源刻意未出現在介面上：每個場景都透過行程內統一的
 * nccu::engine::input::Input seam 讀輸入（測試以 SetSource 改寫之），日後若要改成
 * 傳入 `InputSource&` 參數，也無需更動 IScene 的對外形貌。
 */
class IScene {
public:
    virtual ~IScene() = default;

    /**
     * @brief 場景成為堆疊頂端（現役）當下的一次性接線。
     *
     * 覆寫以擷取參考、訂閱事件等。預設空實作。
     */
    virtual void Enter() {}

    /**
     * @brief 每幀的模型推進。
     * @param dt 距前一幀的秒數，來源為 nccu::engine::platform::Time。
     * @return SceneManager 於「本幀 Draw 之後」才套用的場景切換指令；
     *         SceneCommand{Kind::None} 表示維持現役場景、不切換。
     */
    [[nodiscard]] virtual SceneCommand Update(float dt) = 0;

    /**
     * @brief 每幀的繪製。
     * @param renderer 場景作畫的目標抽象渲染器。
     *
     * 於 nccu::engine::render::DrawScope 的 BeginDrawing／EndDrawing 之間被呼叫。
     * 設為純虛擬，強制葉場景實作（沒有可見輸出的場景無意義；即便是 LoadingScene
     * 的暖機階段也仍會送出一次清屏）。
     */
    virtual void Draw(nccu::engine::render::IRenderer& renderer) = 0;

    /** @brief Enter 的對應收尾：取消訂閱、釋放參考。預設空實作。 */
    virtual void Exit() {}

    /**
     * @brief 取得供自動遊玩錄製用的世界快照。
     * @return 持有 World 的場景回傳其指標；無 World 者（Title／Select／Loading）
     *         回傳 nullptr。
     *
     * 自動遊玩錄製機制的 EndFrame 需要 `const World&` 才能把玩家、旗標、事件序列化
     * 進當幀的 state.jsonl。回傳 nullptr 時 SceneManager::Run 會略過該幀的 EndFrame
     * 呼叫——這些畫面本就不參與錄製。僅 GameplayScene 覆寫以回傳它所擁有的 world。
     */
    [[nodiscard]] virtual const World*
    WorldForHarnessOrNull() const noexcept { return nullptr; }
};

} // namespace nccu::app

#endif // APP_ISCENE_H_
