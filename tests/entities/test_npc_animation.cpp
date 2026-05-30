#include "doctest/doctest.h"
#include "game/entities/NPC.h"
#include "game/gfx/WalkCycle.h"

#include <set>
#include <vector>

/**
 * @file test_npc_animation.cpp
 * @brief 驗證 NPC 行走動畫的算繪格選擇（CurrentRenderCell）：靜止型 NPC 固定停在
 *        idle 格；走動／校慶繞圈 NPC 會切換步伐欄並依朝向選列；以及貼牆滑行時
 *        朝向列保持穩定不抖動。
 */

namespace {
// 走動 NPC 會播放 Pipoya 四方向行走動畫，靜止的對話／任務型 NPC 停在 idle 格。
// CurrentRenderCell() 是唯一決定 {col,row} 之處，因此這些測試能在沒有 GL
// 環境下驗證實際的算繪格選擇（貼圖本身才需要 GL）。算繪格只是讀取已模擬好的
// 朝向／動畫狀態，純讀取不影響任何序列化結果。
constexpr float kDt = 1.0f / 60.0f;   // harness 的固定步進
}

// 靜止型 NPC 永遠算繪 idle 格（col 1, row 0）。
TEST_CASE("靜止型 NPC 永遠算繪 idle 格（col 1, row 0）") {
    NPC npc(nccu::engine::math::Vec2{500.0f, 500.0f},
            std::vector<std::string>{"hi"}, /*isQuestGiver=*/true, "ta");
    // Before any Update.
    CHECK(npc.CurrentRenderCell().col == 1);
    CHECK(npc.CurrentRenderCell().row == 0);
    // 靜止型 NPC 的 Update 是空操作（它站在原地），因此 idle 格逐幀不變。
    for (int i = 0; i < 120; ++i) npc.Update(kDt);
    CHECK(npc.CurrentRenderCell().col == 1);
    CHECK(npc.CurrentRenderCell().row == 0);
}

// 校慶繞圈跑者會播放行走動畫，並朝向其移動方向。
TEST_CASE("校慶繞圈跑者播放行走動畫，並朝向其移動方向") {
    NPC npc(nccu::engine::math::Vec2{1000.0f, 1000.0f}, {});
    // 圓心放在起點下方，使第一個切線方向的步伐朝向已知方向；angularSpeed
    // 為正代表逆時針。
    npc.EnableCircularRun(nccu::engine::math::Vec2{1000.0f, 1100.0f},
                          /*radius=*/100.0f, /*angularSpeed=*/1.5f,
                          /*startAngle=*/ -1.5707963f /* -pi/2：圓的頂端 */);

    std::set<int> columnsSeen;
    std::set<int> rowsSeen;
    for (int i = 0; i < 240; ++i) {        // 4 秒，遠超繞一整圈
        npc.Update(kDt);
        const NPC::RenderCell c = npc.CurrentRenderCell();
        columnsSeen.insert(c.col);
        rowsSeen.insert(c.row);
        // 列永遠是合法的 Pipoya 朝向列。
        CHECK(c.row >= 0);
        CHECK(c.row <= 3);
    }
    // 繞一圈會輪流經過兩個步伐欄（0 與 2），而非只停在 idle 欄——亦即它確實在
    // 播放動畫而非滑行。
    CHECK(columnsSeen.count(0) == 1);
    CHECK(columnsSeen.count(2) == 1);
    // 繞行讓它朝向多個方向（超過一列），證明朝向驅動了列的選擇。
    CHECK(rowsSeen.size() >= 2);
}

// 環境遊走 NPC 一旦真的移動，就會離開 idle 姿勢。
TEST_CASE("環境遊走 NPC 一旦真的移動就離開 idle 姿勢") {
    NPC npc(nccu::engine::math::Vec2{1000.0f, 1000.0f}, {});
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/12345u);  // 決定性 PRNG
    // 未設定遊走遮罩，所以它在開闊地自由移動，大多數幀都會位移、行走動畫推進。
    // 跑個幾秒，確認它既朝向合法的方向列，又在某些時刻推進步伐欄（不會永遠
    // 停在 idle 欄）。
    std::set<int> columnsSeen;
    for (int i = 0; i < 240; ++i) {
        npc.Update(kDt);
        const NPC::RenderCell c = npc.CurrentRenderCell();
        columnsSeen.insert(c.col);
        // 算繪的列永遠是合法的 Pipoya 朝向列。
        CHECK(c.row >= 0);
        CHECK(c.row <= 3);
    }
    // 至少推進過一次真正的步伐欄（左腳 0 或右腳 2）——亦即它有播放動畫，
    // 而非一直站在 idle 欄。
    CHECK((columnsSeen.count(0) == 1 || columnsSeen.count(2) == 1));
}

// 走動 NPC 貼牆滑行時詭異抖動的回歸測試。抖動成因：facing_ 原本是依每幀的
// 淨位移（step）決定，而 WalkRowForFacing 以主導軸選列。當遊走者沿牆或世界
// 邊緣滑行時，被擋住的軸會歸零，使淨位移在軸向（如 {d,0}）與對角之間反覆切換，
// 導致朝向列每幀在左↔下之間跳動。修正方式是把 facing_ 綁在穩定的重定向朝向
//（wanderDir_）上，它在整段 1～3 秒的行程中保持不變，因此滑行期間列不變。
//
// 決定性建構：seed 51 使第一個遊走朝向為右下對角 {1,1}。把 NPC 緊貼世界底部
// 邊緣（y = kSize-kPlayerHeight）。{1,1} 的移動會使 y 被裁切（已在地板），
// x 持續增加，所以每幀的淨位移都是 {d,0}（軸向往右），即使預期朝向是右下對角。
// WalkRowForFacing 對對角 {1,1} 取垂直 → row 0（下）；對 {d,0} → row 2（右）。
// 因此：
//   * 修正後（facing = wanderDir_）：列維持 0（下），穩定、不抖。
//   * 有 bug 時（facing = step）：列為 2（右），且會隨被擋軸交替而閃爍。
TEST_CASE("貼牆滑行的遊走者維持單一穩定的朝向列（不抖動）") {
    constexpr float kEdge = 2048.0f - 24.0f;   // kSize - kPlayerHeight
    NPC npc(nccu::engine::math::Vec2{1500.0f, kEdge}, {});   // 緊貼地板
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/51u);  // 第一個朝向 {1,1}

    std::set<int> movingRows;
    int movingFrames = 0;
    // 第一段行程至少 60 幀（重定向間隔 1～3 秒），期間 NPC 一直沿地板往右滑，
    // 因此檢視一個落在這段「固定朝向」行程內的視窗。
    for (int i = 0; i < 50; ++i) {
        const nccu::engine::math::Vec2 before = npc.GetPosition();
        npc.Update(kDt);
        const nccu::engine::math::Vec2 after = npc.GetPosition();
        const nccu::engine::math::Vec2 step{after.x - before.x, after.y - before.y};
        if (step.x != 0.0f || step.y != 0.0f) {
            ++movingFrames;
            movingRows.insert(npc.CurrentRenderCell().row);
            // 淨位移是軸向往右（y 被釘在地板），確認這正是觸發抖動的滑行情境。
            CHECK(step.x > 0.0f);
            CHECK(step.y == doctest::Approx(0.0f));
        }
    }
    REQUIRE(movingFrames > 0);            // 它確實滑動了
    // 修正後：整段滑行只有一個朝向列，且為穩定朝向的列（下 = 0），而非每幀
    // 位移所對應的列（右 = 2）。
    CHECK(movingRows.size() == 1);                       // 不閃爍
    CHECK(movingRows.count(0) == 1);                     // 穩定朝向（下）
    CHECK(movingRows.count(2) == 0);                     // 不是位移的列
}

// 暫停（無位移）的遊走者顯示 idle 欄。
TEST_CASE("暫停（無位移）的遊走者顯示 idle 欄") {
    // 製造停頓：建立一個遊走者，但只檢視淨位移為零的幀——那些幀必須算繪 idle
    // 欄（moving_ 為 false 時 Update 會把 animStep_ 重設為 0）。
    NPC npc(nccu::engine::math::Vec2{1000.0f, 1000.0f}, {});
    npc.EnableWander(/*speed=*/40.0f, /*seed=*/777u);
    int idleFramesChecked = 0;
    for (int i = 0; i < 600; ++i) {
        const nccu::engine::math::Vec2 before = npc.GetPosition();
        npc.Update(kDt);
        const nccu::engine::math::Vec2 after = npc.GetPosition();
        if (before.x == after.x && before.y == after.y) {
            CHECK(npc.CurrentRenderCell().col == 1);   // idle 欄
            ++idleFramesChecked;
        }
    }
    // PRNG 包含一個暫停朝向（idx 8 = {0,0}）並加上世界邊緣裁切，因此 10 秒內
    // 至少會出現一個零位移的幀。
    CHECK(idleFramesChecked > 0);
}
