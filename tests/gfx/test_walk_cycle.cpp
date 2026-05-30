#include "doctest/doctest.h"
#include "game/gfx/WalkCycle.h"

/**
 * @file test_walk_cycle.cpp
 * @brief 驗證 Player 與 NPC 共用的 Pipoya 走路圖計算：步伐欄位選擇與面向對應的列。
 *
 * 這些是純函式，可在無 GL 環境下測試；它們釘住 NPC::Render 依賴的格位選擇
 *（其貼圖繪製需 GL，無法在無 GL context 下直接測試）。
 */

using nccu::engine::math::Vec2;
using nccu::game::gfx::WalkColumn;
using nccu::game::gfx::WalkRowForFacing;
using nccu::game::gfx::kWalkColumns;

// 四步循環為 idle -> left -> idle -> right，並對任意整數步數正確環繞。
TEST_CASE("WalkColumn 四步循環為 idle -> left -> idle -> right 並正確環繞") {
    // 圖序為 idle(1)、左腳(0)、idle(1)、右腳(2)：兩個跨步格之間夾一個 idle
    // 格，讓踏步看起來自然——第 0 步是靜止/idle 姿勢。
    CHECK(WalkColumn(0) == 1);
    CHECK(WalkColumn(1) == 0);
    CHECK(WalkColumn(2) == 1);
    CHECK(WalkColumn(3) == 2);
    // 對任意整數都環繞（NPC/Player 計數器本身已做 % 4，但此 helper 必須是
    // 全函式，避免過期的步數索引到範圍外）。
    CHECK(WalkColumn(4) == WalkColumn(0));
    CHECK(WalkColumn(7) == WalkColumn(3));
    CHECK(WalkColumn(-1) == WalkColumn(3));   // 負數也環繞
    CHECK(WalkColumn(-4) == WalkColumn(0));
}

// WalkColumn 與標準的 kWalkColumns 表一致。
TEST_CASE("WalkColumn 與標準的 kWalkColumns 表一致") {
    for (int s = 0; s < 4; ++s)
        CHECK(WalkColumn(s) == kWalkColumns[static_cast<std::size_t>(s)]);
}

// WalkRowForFacing 將四個基本方向對應到列（0=下..3=上）。
TEST_CASE("WalkRowForFacing 將四個基本方向對應到列（0=下..3=上）") {
    CHECK(WalkRowForFacing(Vec2{0.0f,  1.0f}) == 0);   // 下
    CHECK(WalkRowForFacing(Vec2{-1.0f, 0.0f}) == 1);   // 左
    CHECK(WalkRowForFacing(Vec2{1.0f,  0.0f}) == 2);   // 右
    CHECK(WalkRowForFacing(Vec2{0.0f, -1.0f}) == 3);   // 上
}

// 取絕對值較大的主軸決定方向；相等時偏向垂直方向。
TEST_CASE("WalkRowForFacing：絕對值較大的主軸決定方向，相等時偏向垂直") {
    // |x| > |y| -> 水平列。
    CHECK(WalkRowForFacing(Vec2{-3.0f, 1.0f}) == 1);   // 偏左
    CHECK(WalkRowForFacing(Vec2{ 3.0f, 1.0f}) == 2);   // 偏右
    // |y| >= |x| -> 垂直列（完美對角線時偏向上/下）。
    CHECK(WalkRowForFacing(Vec2{1.0f,  1.0f}) == 0);   // 平手 -> 下
    CHECK(WalkRowForFacing(Vec2{1.0f, -1.0f}) == 3);   // 平手 -> 上
    CHECK(WalkRowForFacing(Vec2{1.0f,  3.0f}) == 0);   // 偏下
    CHECK(WalkRowForFacing(Vec2{1.0f, -3.0f}) == 3);   // 偏上
}

// 零向量（無方向）時靜止面向下（列 0）。
TEST_CASE("WalkRowForFacing：零向量時靜止面向下（列 0）") {
    CHECK(WalkRowForFacing(Vec2{0.0f, 0.0f}) == 0);
}
