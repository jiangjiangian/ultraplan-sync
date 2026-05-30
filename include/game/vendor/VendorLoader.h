#ifndef VENDOR_LOADER_H_
#define VENDOR_LOADER_H_
#include "game/vendor/VendorConfig.h"
#include <string>
#include <vector>

namespace nccu::vendor {

/**
 * @brief 解析市集插曲（Interlude）攤位清單的執行期解析器。
 * @param path 市集 markdown 內容檔的路徑。
 * @return 解析出的 VendorConfig 清單；檔案開不起來時回傳空向量（不丟例外）。
 *
 * 是 dialog::LoadChapter 的「價目表」姊妹版：市集內容檔為唯一真實來源，改內容檔即可
 * 改動遊戲內市集，無須重新編譯、無須改 code（與章節對話完全相同）。藉此取代「手工
 * 抄寫十個字面常數」的舊作法（那正是本專案刻意淘汰的 codegen 模式）。
 *
 * 區段慣例（與 "## NPC：<name>" 平行）：
 *   - `## 攤位：<攤名>`
 *   - `> 攤主：<人>`
 *   - `> 商品：<itemId> = <price>`（0..n 行；itemId 為正規 code id）
 *   - `> 機制：<buy|donate|sell|game|flavor>`
 *   - `> tier：<N>`（選填，設計分組用）
 *   - `> karma：<±N>`（選填，成交時套用）
 *   - `> stock：<N>`（選填，-1 或缺省表示無限）
 *   - `### greeting`：底下 `- "…"` 收進 greetingLines
 *   - `### onPurchase`：成交區塊（onDonate／onAccept 為同義，取第一個非空者）
 *   - `### onLeave`：收進 onLeave
 *
 * 帶變體後綴的區塊（例如 `### onPurchase（陷阱傘殘骸）`）會被解析，但目前只保留第一個
 * 成交區塊（後續設計可能再細分）。台詞行採與對話相同的 `- "…"`（ASCII 或全形引號）格式。
 *
 * 開檔失敗即回傳空向量，與 LoadChapter「降級為空」的契約一致。
 */
std::vector<VendorConfig> LoadInterludeVendors(const std::string& path);

}  // namespace nccu::vendor

#endif // VENDOR_LOADER_H_
