/**
 * @file main.cpp
 * @brief MVC 的 composition root：開窗、載入共用資源，再把整段畫面流程交給
 *        SceneManager。
 *
 * Model = World（純資料），View = 渲染，Controller = 輸入＋模擬＋事件接線。main()
 * 維持精簡：只負責組裝、呼叫 Run、以及依序拆除。測試／人類分支與
 * Loading → Title → CharacterSelect → Gameplay 場景鏈封裝於 SceneBootstrap，故本檔
 * 僅有組裝 ＋ Run 呼叫 ＋ 有序拆除。
 *
 * 畫面流程（人類遊玩）：Loading → Title → 角色選擇 → 遊玩。遊戲內選單的「重新開始」
 * 會重建全新的 World／View／GameController 場景，使狀態不跨次外洩，也不留下懸空的
 * EventBus 訂閱者（controller 解構會在建立下一個 World 之前清空事件匯流排）。
 *
 * 自動錄製：當環境設定啟用時，標題與角色選擇會被略過——以決定性角色直接進入單次
 * 遊玩——使腳本化時間軸產生逐位元一致的 state.jsonl。重新開始僅限人類路徑（腳本化
 * 輸入永不開啟暫停選單）。
 */
#include "app/SceneManager.h"
#include "app/SceneBootstrap.h"
#include "engine/platform/Harness.h"
#include "engine/platform/WorkingDir.h"
#include "engine/render/Window.h"
#include "engine/render/Font.h"
#include "engine/render/RaylibRenderer.h"
#include "engine/render/Texture.h"
#include "engine/audio/AudioDevice.h"
#include "engine/events/EventSink.h"  // 實體層發布接縫：拆除時呼叫 SetSink(nullptr)

int main() {
    constexpr int kWinW = 800;
    constexpr int kWinH = 450;

    auto win = nccu::engine::render::Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    // 最先正規化工作目錄（在任何資產載入之前），使遊戲的相對路徑 resources/ 與
    // docs/content/ 不論如何啟動（Finder／IDE／從 build 目錄執行）都能解析。如此
    // EnsureFont 走在「內容齊備」的安全字型圖集路徑上，而非曾在某些 GPU 上崩潰的
    // 過大後備路徑。若已從具備資產的目錄執行（專案根／ctest／錄製器）則為 no-op。
    nccu::engine::platform::EnsureAssetWorkingDir();

    // raylib 預設字型僅含 ASCII；趁 GL context 已存在、且在任何文字（標題／選擇／
    // HUD）繪製之前，先載入中日韓字型。
    nccu::engine::render::EnsureFont();

    // 音訊裝置——每行程唯一的 RAII handle，於此宣告使其拆除順序與 Window／Font 同處
    // composition root。
    nccu::audio::AudioDevice audioDevice;

    // 預設關閉；唯有啟用錄製時，才以無頭方式驅動輸入並略過互動式標題與角色選擇，
    // 以取得具決定性的執行。
    auto harness = nccu::MaybeAttach();

    // ESC 在各處皆為惰性（Window::Open 呼叫 SetExitKey(KEY_NULL)），故
    // WindowShouldClose() 只會由視窗關閉鈕觸發。離開遊戲走「離開」選單項（標題選單／
    // 遊戲內暫停選單）。
    nccu::app::SceneManager sm;
    nccu::engine::render::RaylibRenderer renderer;

    // 組裝並推入初始場景（錄製略過路徑，或人類的 Loading→Title→Select→Gameplay
    // 鏈）。所有畫面流程接線都在 SceneBootstrap，使 main() 維持為精簡的 composition
    // root。
    nccu::app::PushInitialScene(sm, harness, audioDevice, kWinW, kWinH);

    // 唯一的 Run 呼叫驅動整段畫面流程。Restart 重新進入全新的鏈；Quit／視窗關閉／
    // 錄製器 ShouldQuit 則結束程式。
    (void)sm.Run(win, renderer, harness);

    // 在下方 GL 拆除前，先卸下實體層發布接縫。
    nccu::events::SetSink(nullptr);

    // 在 Window 解構執行 ::CloseWindow() 之前，先卸載字型與紋理快取：兩者都持有 GPU
    // 資源，而 static 生命週期的儲存體否則會在 GL context 消失之後才解構（觸碰失效的
    // GL → UB）。本行 `win` 仍存活，故 GL 仍有效。快取對每張紋理只擁有一次，因此這裡
    // 是共用（快取）紋理唯一的 ::UnloadTexture 點。
    nccu::engine::render::ShutdownTextureCache();
    nccu::engine::render::ShutdownFont();
    return 0;
}
