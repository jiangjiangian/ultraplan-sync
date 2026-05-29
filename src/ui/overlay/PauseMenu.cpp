#include "ui/overlay/PauseMenu.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

namespace nccu {

void DrawPauseMenu(nccu::gfx::IRenderer& r,
                   const World& world,
                   float screenW,
                   float screenH) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;

    if (!world.MenuOpen()) return;

    // In-game pause menu overlay — drawn LAST so it sits above the world,
    // HUD, dialog and inventory. Reactive: a pure function of
    // World::MenuOpen + MenuCursor (no retained UI state in the View).
    // A full-screen dim then a centred panel; the cursor row gets a
    // marker + highlight colour so keyboard selection is unambiguous.
    const float W = screenW;
    const float H = screenH;
    r.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 150});

    constexpr float kPanelW = 340.0f;
    // Cycle 9.E.3: panel grew from 250 to 330 px to accommodate 6
    // rows (was 4) — kFirstY 78 + 5 * kRowH 40 = 278 px from the
    // top edge, plus a ~50 px hint band → 330 keeps the same visual
    // breathing room above & below the rows the 4-row layout had.
    constexpr float kPanelH = 330.0f;
    const float px = W * 0.5f - kPanelW * 0.5f;
    const float py = H * 0.5f - kPanelH * 0.5f;
    r.DrawRect(Rect{px, py, kPanelW, kPanelH},
               Color{20, 22, 30, 230});

    TextBuilder{"遊戲選單"}
        .At(Vec2{px + kPanelW * 0.5f - 64.0f, py + 24.0f})
        .Size(28).Color(Colors::Gold).Draw();

    // Cycle 9.E.3: 6 items now — 繼續 / 說明 / 減少動畫[開|關] /
    // 擴大目標[開|關] / 重新開始 / 離開. Toggle rows (2,3) show the
    // current state suffix; ordering mirrors GameController's
    // switch. 說明 opens the help overlay drawn below; rows 2 and 3
    // flip World::SetReducedMotion / SetLargeTargets in place
    // (cursor stays on the row so the player can verify the change).
    // Destructive items (Restart/Quit) sit farthest from index 0 so
    // an accidental Enter on Resume doesn't risk progress loss.
    static const char* kLabels[World::kMenuItemCount] = {
        "繼續", "說明", "減少動畫", "擴大目標", "重新開始", "離開"};
    constexpr float kFirstY = 78.0f;
    constexpr float kRowH   = 40.0f;
    for (int i = 0; i < World::kMenuItemCount; ++i) {
        const bool sel = (i == world.MenuCursor());
        std::string row =
            (sel ? std::string("> ") : std::string("  ")) +
            kLabels[i];
        // Toggle rows: append the current on/off state in brackets.
        // The leading "  " / "> " keeps every row's x-alignment so
        // the cursor caret column stays stable as it moves up/down.
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
    // Audit D3 / SC 1.4.3: was Colors::DarkGray (80,80,80) on the
    // Color{20,22,30,230} pause-menu panel — ~1.05:1, effectively
    // invisible. Color{180,180,180,255} hits ~7:1 (AAA-large,
    // AA-normal) on the same backing.
    TextBuilder{"↑ ↓ 選擇   Enter 確認   M 繼續"}
        .At(Vec2{px + 20.0f, py + kPanelH - 30.0f})
        .Size(14).Color(Color{180, 180, 180, 255}).Draw();
}

}  // namespace nccu
