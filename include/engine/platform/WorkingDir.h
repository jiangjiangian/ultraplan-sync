#ifndef ENGINE_PLATFORM_WORKING_DIR_H_
#define ENGINE_PLATFORM_WORKING_DIR_H_
#include "raylib.h"

/**
 * @file WorkingDir.h
 * @brief 工作目錄正規化：把遊戲相對資源路徑能解析的 CWD 設定收斂在引擎層。
 *
 * raylib.h 被限制在引擎層，因此 chdir 藏在這層的具型別包裝後，app 層的 main()
 * 只需呼叫它即可。
 */
namespace nccu::engine::platform {

/**
 * @brief 確保程序工作目錄能解析遊戲的相對資源路徑（resources/...、docs/content/...）。
 *
 * 每個材質／字型／地圖／對話檔皆以相對於 CWD 的路徑載入。以建議方式（從專案根
 * 目錄執行 ./build/OOP_Raylib_Lab）啟動沒問題，但從檔案管理員／IDE／點兩下，或
 * 從 build/ 內執行二進位檔，會讓 CWD 落在沒有這些資料夾的位置。在 macOS 上
 * raylib 還停用了 GLFW 的 chdir-to-bundle，沒有任何機制替我們修正；而缺少
 * docs/content 曾使字型載入走向過大圖集的退路（在部分 GPU 上造成啟動崩潰）。
 *
 * CMake 會把 resources/ 與 docs/content/ 一併複製到執行檔旁，因此當 CWD 缺少它們
 * 時，便切換到執行檔自身目錄（保證有這兩者）。具冪等性且保守：若資源已能從 CWD
 * 解析（從專案根執行、ctest、從根執行自動遊玩），則不更動 CWD，正常遊玩與自動
 * 遊玩皆不受影響。請於啟動時、第一次載入資源（EnsureFont）之前呼叫一次。
 */
inline void EnsureAssetWorkingDir() {
    if (::DirectoryExists("resources") && ::DirectoryExists("docs/content"))
        return;                                // 從此處已可正常執行
    const char* appDir = ::GetApplicationDirectory();
    if (appDir != nullptr && appDir[0] != '\0')
        ::ChangeDirectory(appDir);             // .../build，CMake 把兩者都放在這
}

} // namespace nccu::engine::platform

#endif // ENGINE_PLATFORM_WORKING_DIR_H_
