#include "game/dialog/DialogSource.h"
#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

/**
 * @file test_dialog_content_dir.cpp
 * @brief 測試支援用的編譯單元（不含 TEST_CASE）：在所有測試開始前，
 *        把 DialogSource 的內容目錄指向真正的對話素材路徑。
 */

// 開啟對話的測試會透過 DialogSource 讀檔，DialogSource 會相對於其 content
// dir（預設為相對 cwd 的 docs/content）去解析各章節的 markdown。測試執行檔的
// cwd 是 build 目錄，用預設值會找不到檔案，那些測試就會悄悄拿到空對話。
// 這個全域靜態初始化器會在 doctest 的 main() 之前執行（所有編譯單元的靜態
// 初始化會先完成），因此每個開啟對話的測試都能解析到正確的內容目錄。
namespace {
struct DialogContentDirInit {
    DialogContentDirInit() { nccu::dialog::SetContentDir(TEST_CONTENT_DIR); }
};
const DialogContentDirInit kDialogContentDirInit;
}  // namespace
