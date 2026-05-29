#ifndef WORLD_OPTIONS_H_
#define WORLD_OPTIONS_H_

/**
 * @file WorldOptions.h
 * @brief World 建構時注入的無障礙選項 DTO，以及從環境變數解析它的工廠函式。
 */

namespace nccu {

/**
 * @brief 注入 World 建構子的無障礙偏好設定。
 *
 * 將環境變數的讀取移出 World，使 World 對其引數保持純粹：建構子接收已解析好的布林
 * 值，而非自行呼叫 std::getenv；環境解析由 main.cpp（組裝根）執行一次。測試可直接
 * 建構 WorldOptions{}（兩者皆 false），讓單元測試不相依於宿主行程的環境。預設值對
 * 應環境變數未設時的舊行為：兩者皆關閉。
 */
struct WorldOptions {
    /// @brief 減少動畫偏好：開啟時 View 抑制章節卡片滑動與跑道環的動態微光。
    bool reducedMotion = false;
    /// @brief 擴大目標偏好：開啟時 E 互動探測範圍由每側 8 px 增為 16 px，
    ///        讓手部顫抖的玩家不必像素級對齊即可觸發 NPC 對話。
    bool largeTargets = false;
};

/**
 * @brief 從行程環境變數解析 WorldOptions。
 * @return 解析後的選項；對應變數值為 "1" 才開啟，未設或 "0" 維持預設 false。
 *
 * 供 main.cpp（組裝根）使用，把環境讀取集中在這一個明確位置，而非散落於 World 建構
 * 子內。不呼叫此函式的測試因而能取得確定性的選項結構。
 */
[[nodiscard]] WorldOptions ReadWorldOptionsFromEnv();

} // namespace nccu

#endif // WORLD_OPTIONS_H_
