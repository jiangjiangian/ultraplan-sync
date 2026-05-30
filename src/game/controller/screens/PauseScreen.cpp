#include "game/controller/screens/PauseScreen.h"
#include "game/world/World.h"
#include "game/state/GameHelpPages.h"  // kGameHelpPageCount
#include "engine/input/Input.h"
#include "engine/input/Key.h"

/**
 * @file PauseScreen.cpp
 * @brief 暫停選單與其上層說明疊層的輸入處理：M 開關選單，選單開啟期間凍結模擬，
 *        並把游標選項導向繼續／說明／無障礙開關／重新開始／離開。
 */

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出，以此引入

bool HandlePauseMenu(World& world) {
    using nccu::engine::input::Input;
    using nccu::engine::input::Key;
    const bool toggle = Input::IsPressed(Key::M);
    if (world.MenuOpen()) {
        // 說明（help）疊層覆蓋在已暫停的選單「之上」。它開啟時，M／E／Enter 用來把它
        // 關回選單，期間選單游標與模擬皆維持凍結（ESC 不是關閉鍵——它會結束程式）。此處
        // 「最先」處理，使一個原本要「關閉說明」的按鍵，絕不會同時移動選單游標或觸發
        // AppAction。
        if (world.HelpOpen()) {
            // 說明疊層分頁顯示——←／→ 在「操作＋目標」頁與「雨傘外觀＋道具須知＋結局」頁
            // 之間切換（頁碼會繞回；View 會畫出「第 N／M 頁」的指示）。這是純 UI 狀態
            //（World::HelpPage，不序列化），故分頁不會改變存檔位元組。M／E／Enter 仍可關回
            // 選單。
            constexpr int n = nccu::kGameHelpPageCount;
            if (Input::IsPressed(Key::Right))
                world.SetHelpPage((world.HelpPage() + 1) % n);
            if (Input::IsPressed(Key::Left))
                world.SetHelpPage((world.HelpPage() - 1 + n) % n);
            if (Input::IsPressed(Key::M) ||
                Input::IsPressed(Key::Enter) ||
                Input::IsPressed(Key::E))
                world.SetHelpOpen(false);
            return true;                    // 凍結於說明疊層之後
        }
        if (Input::IsPressed(Key::Up))   world.MoveMenuCursor(-1);
        if (Input::IsPressed(Key::Down)) world.MoveMenuCursor(1);
        if (toggle) {                       // M = 快速繼續
            world.SetMenuOpen(false);
            return true;
        }
        if (Input::IsPressed(Key::Enter)) {
            switch (world.MenuCursor()) {
                case 0:                     // 繼續 (Resume)
                    world.SetMenuOpen(false);
                    break;
                case 1:                     // 說明 (Help) — overlay
                    world.SetHelpOpen(true);
                    break;
                case 2:                     // 減少動畫 (toggle)
                    // ReducedMotion 無障礙旗標的暫停選單開關。就地翻轉、選單維持開啟，
                    // 使玩家能在游標所在的同一列即時看到 [開]/[關] 狀態更新。純 World
                    // 狀態變動——不發 AppAction、不關閉選單。
                    world.SetReducedMotion(!world.ReducedMotion());
                    break;
                case 3:                     // 擴大目標 (toggle)
                    // LargeTargets 無障礙旗標的暫停選單開關。與第 2 列同樣的就地翻轉
                    // 形態——下一個遊玩幀的 E 互動探測距離，會經 World::LargeTargets()
                    // 讀到新值。
                    world.SetLargeTargets(!world.LargeTargets());
                    break;
                case 4:                     // 重新開始 (Restart)
                    world.RequestAppAction(World::AppAction::Restart);
                    break;
                default:                    // 離開 (Quit)
                    world.RequestAppAction(World::AppAction::Quit);
                    break;
            }
        }
        return true;   // 選單開啟期間凍結
    }
    if (toggle) {                           // 自遊玩中開啟
        world.SetMenuOpen(true);
        return true;
    }
    return false;   // 本幀與選單無關——往下穿透
}

} // namespace nccu
