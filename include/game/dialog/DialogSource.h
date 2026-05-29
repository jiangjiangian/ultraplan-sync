#pragma once
#include "game/dialog/DialogLoader.h"
#include "game/state/SemesterState.h"

#include <string>
#include <string_view>
#include <vector>

namespace nccu::dialog {

class DialogRepository;     // 前向宣告——詳見 DialogRepository.h

/**
 * @file DialogSource.h
 * @brief 執行期對話供給的自由函式介面：把 (NPC id, SemesterState) 對映到對應章節
 *        中該 NPC 的解析子段。
 *
 * 依需求透過 LoadChapter 載入章節 Markdown，每狀態快取一份 LoadedChapter。
 *
 * 為何存在：對白內容過去在建置期凍結進生成的標頭，任何文字微調都得重新生成並重編。
 * 本供給層改為在執行期直接讀取作者撰寫的 docs/content 下的 *.md，並提供 Reload()，讓作者
 * 編輯章節檔後於下一次對話即見效、無需重建。
 *
 * 「英文 npcId → 中文 section 名」與「SemesterState → 章節檔名」是固定查表，置於 .cpp。
 * 未知 id／名／檔皆不丟例外——回傳空向量，與 LoadChapter 的 no-throw 契約一致。
 *
 * 參考生命週期：Entries() 回傳指向每狀態快取的參考，於下次 Reload()（清空快取）前有效；
 * 之後針對尚未載入狀態的 Entries() 只追加、不使既有參考失效。
 *
 * 快取與內容目錄現已收斂到 DialogRepository「實例」（見 DialogRepository.h）。以下自由
 * 函式透過下方 SetRepository / Repository() 接縫委派給進程當前倉儲；預設落到進程層級的
 * 倉儲，故所有既有呼叫端維持原狀不變。
 */

/**
 * @brief 取 NPC `npcId` 在 `state` 對應章節的子段，依 subState 遞增排序。
 * @return 子段向量；npcId、對映 section 名或章節檔任一未知／遺失時為空。
 */
const std::vector<SubEntry>& Entries(std::string_view npcId,
                                     SemesterState state);

/**
 * @brief 清掉所有已快取章節，使下次 Entries() 重新自磁碟讀取 Markdown。
 *
 * 熱重載鉤子：編輯 docs/content 下的 *.md 後呼叫 Reload()，即供給新文字。
 */
void Reload();

/**
 * @brief 覆寫讀取章節檔的目錄（預設 "docs/content"，相對進程工作目錄解析）。
 * @param dir 新的內容目錄。
 *
 * 讓單元測試指向固定內容目錄而不依賴 cwd；於下次（重新）載入狀態時生效。
 */
void SetContentDir(std::string dir);

/**
 * @brief 覆寫當前倉儲（測試替換接縫）。
 * @param repo 要使用的倉儲；傳 nullptr 則回退到進程預設倉儲。
 *
 * 欲完全隔離的測試自建一個 DialogRepository，呼叫 SetRepository(&myRepo) 跑完案例後，
 * 還原（SetRepository(nullptr)）或留給下一個 subcase。
 */
void SetRepository(DialogRepository* repo) noexcept;

/// @brief 取當前倉儲——SetRepository 的覆寫值或進程預設。
[[nodiscard]] DialogRepository& Repository() noexcept;

}  // namespace nccu::dialog
