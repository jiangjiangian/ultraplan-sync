#include "ui/overlay/PauseMenu.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

namespace nccu {

void DrawPauseMenu(nccu::engine::render::IRenderer& r,
                   const World& world,
                   float screenW,
                   float screenH) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    if (!world.MenuOpen()) return;

    // 遊戲內暫停選單疊層——最後繪製，使其位於世界、HUD、對話與物品欄之上。反應式：
    // 為 World::MenuOpen + MenuCursor 的純函式（View 內不保留 UI 狀態）。先全螢幕變暗，
    // 再畫置中面板；游標所在列加上標記與高亮色，使鍵盤選取明確無歧義。
    const float W = screenW;
    const float H = screenH;
    r.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 150});

    constexpr float kPanelW = 340.0f;
    // 面板由 250 增至 330 px 以容納 6 列（原為 4）——kFirstY 78 + 5 * kRowH 40 =
    // 距頂邊 278 px，再加約 50 px 的提示帶 → 330 可在各列上下保留與 4 列版面相同的
    // 視覺呼吸空間。
    constexpr float kPanelH = 330.0f;
    const float px = W * 0.5f - kPanelW * 0.5f;
    const float py = H * 0.5f - kPanelH * 0.5f;
    r.DrawRect(Rect{px, py, kPanelW, kPanelH},
               Color{20, 22, 30, 230});

    TextBuilder{"遊戲選單"}
        .At(Vec2{px + kPanelW * 0.5f - 64.0f, py + 24.0f})
        .Size(28).Color(Colors::Gold).Draw();

    // 現有 6 個項目——繼續 / 說明 / 減少動畫[開|關] / 擴大目標[開|關] / 重新開始 /
    // 離開。可切換的列（2、3）會顯示當前狀態後綴；順序對齊 GameController 的 switch。
    // 「說明」開啟下方繪製的說明疊層；第 2、3 列就地翻轉 World::SetReducedMotion /
    // SetLargeTargets（游標停留在該列，使玩家能確認變更）。具破壞性的項目（重新開始／
    // 離開）距索引 0 最遠，使在「繼續」上誤按 Enter 不致冒進度遺失之險。
    static const char* kLabels[World::kMenuItemCount] = {
        "繼續", "說明", "減少動畫", "擴大目標", "重新開始", "離開"};
    constexpr float kFirstY = 78.0f;
    constexpr float kRowH   = 40.0f;
    for (int i = 0; i < World::kMenuItemCount; ++i) {
        const bool sel = (i == world.MenuCursor());
        std::string row =
            (sel ? std::string("> ") : std::string("  ")) +
            kLabels[i];
        // 可切換的列：在方括號中附上當前開／關狀態。前導的 "  " / "> " 維持每列的
        // x 對齊，使游標插字符的欄位在上下移動時保持穩定。
        if (i == 2) {
            row += world.ReducedMotion() ? "  [開]" : "  [關]";
        } else if (i == 3) {
            row += world.LargeTargets() ? "  [開]" : "  [關]";
        }
        TextBuilder{row}
            .At(Vec2{px + 70.0f, py + kFirstY + i * kRowH})
            .Size(24)
            .Color(sel ? Color{255, 153, 0, 255} : Colors::White)
            .Draw();
    }
    // 原為 Colors::DarkGray (80,80,80) 疊在 Color{20,22,30,230} 的暫停選單面板上——
    // 對比約 1.05:1，幾乎不可見。改用 Color{180,180,180,255} 在同一底色上達到約 7:1
    //（大字 AAA、一般字 AA）。
    TextBuilder{"↑ ↓ 選擇   Enter 確認   M 繼續"}
        .At(Vec2{px + 20.0f, py + kPanelH - 30.0f})
        .Size(14).Color(Color{180, 180, 180, 255}).Draw();
}

}  // namespace nccu
