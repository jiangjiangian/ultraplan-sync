#include "app/scenes/GameplayScene.h"

#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/platform/Harness.h"
#include "engine/render/DrawScope.h"
#include "game/world/WorldOptions.h"

#include <cstdlib>
#include <cstring>

/**
 * @file GameplayScene.cpp
 * @brief 遊玩場景的實作：建構單次遊玩範圍與事件接線、每幀推進模型、處理重啟／離開意圖。
 */

namespace nccu::app {

GameplayScene::GameplayScene(nccu::CharacterSelectResult selection,
                             nccu::audio::AudioDevice& audioDevice,
                             nccu::Harness& harness,
                             int windowWidth,
                             int windowHeight,
                             RestartFactory restartFactory)
    // 初始化順序具語意。World 是 model；View 是呈現；GameController 以 EventBus 接線
    // 兩者。AudioManager 排在最後，使逆序解構最先拆除音訊。初始化列中的匯流排參考解析
    // 為 EventBus::Instance()，讓 controller 串接的正是實體層 Sink 所導向的同一條
    // 匯流排。
    : world_(selection.spritePath, /*loadSprites=*/true,
             nccu::ReadWorldOptionsFromEnv()),
      view_(windowWidth, windowHeight),
      controller_(world_, EventBus::Instance()),
      audioManager_(EventBus::Instance(), audioDevice),
      harness_(harness),
      restartFactory_(std::move(restartFactory)) {
    // 僅限錄製的除錯跳關：UMBRELLA_START_STATE 在啟動時直接跳轉狀態機，使截圖無須
    // 腳本化整條主線即可抵達後段章節。預設關閉——同時受 harness.Active() 與該環境
    // 變數雙重把關，故一般遊玩與傳統計時錄製皆逐位元不變。
    if (harness_.Active()) {
        if (const char* s = std::getenv("UMBRELLA_START_STATE"); s) {
            using S = nccu::SemesterState;
            S target = world_.Semester().Current();
            bool warp = true;
            if      (std::strcmp(s, "Chapter2")  == 0) target = S::Chapter2_Midterms;
            else if (std::strcmp(s, "Chapter3")  == 0) target = S::Chapter3_SportsDay;
            else if (std::strcmp(s, "Chapter4")  == 0) target = S::Chapter4_Finals;
            else if (std::strcmp(s, "Interlude") == 0) target = S::Interlude_Market;
            else warp = false;
            if (warp) {
                world_.Semester().Transition(target);
                world_.RespawnChapterRoster(target);
            }
        }
    }
    // 將實體層發布接縫綁到 controller 串接於章節關卡／任務鉤子的同一條匯流排。
    // SetSink 把 nccu::events::Sink() 從後備的 EventBus::Instance() 改導向本次執行
    // controller 所擁有的匯流排。於 Exit() 重置為 nullptr，使 Restart 循環能乾淨地
    // 重新綁定。
    nccu::events::SetSink(&EventBus::Instance());
    // 角色顏色調變——純外觀。
    if (Player* p = world_.GetPlayer())
        p->SetTint(selection.tint);
}

void GameplayScene::Enter() {
    // 錄製器的 WireEvents 必須在 controller 已擁有匯流排之後才訂閱，使其訂閱者於
    // controller 解構（在 Exit 清空匯流排）之前先行拆除——維持「先接匯流排與 sink、
    // 後加入 WireEvents」的順序。
    harness_.WireEvents();
}

SceneCommand GameplayScene::Update(float /*dt*/) {
    // 模型推進 tick；關鍵在於此處「不」呼叫 BeginFrame／EndFrame。SceneManager::Run
    // 以 harness.BeginFrame()／harness.EndFrame(world_) 包覆每一次 Update + Draw，
    // 使錄製接縫維持逐位元一致，未來的 Title／Select 場景也能共用同一層包覆而無須
    // 改動遊玩 tick。
    controller_.Update();

    // 遊戲內選單意圖。腳本化錄製輸入永不開啟暫停選單，故錄製路徑逐位元不受影響；
    // 人類路徑的 Restart／Quit 選擇由此流經。此處不使用 Pop：單一場景的堆疊無處可
    // 退回——RunOutcome::Restart 時由 main.cpp 直接重建 SceneManager 來拆除。
    const auto act = world_.PendingAppAction();
    if (act == nccu::World::AppAction::Restart) {
        // 透過 composition root 提供的工廠重建。main.cpp 傳入會建立全新 LoadingScene
        // （-> TitleScene -> CharacterSelectScene -> GameplayScene）的 closure，故整條
        // 人類路徑無須 main 的外層迴圈即可重新進入。錄製路徑收到空的 restartFactory_
        // 而落入 Quit，恰好對應「永不重啟」契約：錄製執行若要求 Restart，將結束程式
        // 而非折返標題。
        if (restartFactory_) {
            auto fact = restartFactory_;
            return SceneCommand{SceneCommand::Kind::Replace,
                                std::move(fact)};
        }
        return SceneCommand{SceneCommand::Kind::Quit, {}};
    }
    if (act == nccu::World::AppAction::Quit)
        return SceneCommand{SceneCommand::Kind::Quit, {}};
    return SceneCommand{SceneCommand::Kind::None, {}};
}

void GameplayScene::Draw(nccu::engine::render::IRenderer& /*renderer*/) {
    // View 仍透過其既有管線直接經由 raylib 渲染；此處刻意維持不變——改走抽象的
    // IRenderer 是其他場景的考量（用以讓 doctest 在沒有真實 GL context 下驅動）。
    // DrawScope 由 SceneManager::Run 在每個 Update+Draw 配對中進入。
    view_.Draw(world_);
}

void GameplayScene::Exit() {
    // 在下一次迭代重建 World／Controller（屆時本就會重新 SetSink）之前，先卸下實體層
    // 發布接縫。重置為 nullptr 使接縫對「場景之間執行的程式碼」（資產清理、下次 Restart
    // 循環的標題畫面）退回 EventBus::Instance()。
    nccu::events::SetSink(nullptr);
}

} // namespace nccu::app
