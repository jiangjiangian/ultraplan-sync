#ifndef GAME_DIALOG_DIALOG_REPOSITORY_H_
#define GAME_DIALOG_DIALOG_REPOSITORY_H_
#include "game/dialog/DialogLoader.h"
#include "game/state/SemesterState.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nccu::dialog {

/**
 * @brief 對話內容的「實例化」倉儲：封裝每個進程的章節快取與內容目錄。
 *
 * 把原本以檔案靜態可變狀態存在的對話快取與內容目錄收斂成實例範圍的物件：每個實例
 * 各自持有「每狀態一份」的 LoadedChapter 快取與自己的內容目錄覆寫值，因此測試可建構
 * 私有倉儲達成隔離，而正式環境仍透過 dialog::Repository() 接縫使用進程預設實例（可由
 * 測試替換的接縫模式）。
 *
 * 不收進實例狀態的部分：兩張純資料表（NpcNameTable、ChapterFileTable）仍為
 * DialogSource.cpp 的檔案靜態——它們是不可變查表，跨實例共用純屬最佳化，絕不造成耦合。
 */
class DialogRepository {
public:
    DialogRepository();

    /**
     * @brief 取 NPC `npcId` 在 `state` 對應章節的子段，依 subState 遞增排序。
     * @param npcId NPC 識別字串。
     * @param state 當前學期狀態。
     * @return 子段向量；npcId、對映的 section 名或章節檔任一未知／遺失時為空。
     *
     * 回傳的參考在「本實例」下一次 Reload() 前皆有效——std::map 節點穩定，插入新狀態的
     * 章節不會移動既有向量。
     */
    const std::vector<SubEntry>& Entries(std::string_view npcId,
                                         SemesterState state);

    /**
     * @brief 清掉「本實例」所有已快取章節，使下次 Entries() 重新自磁碟讀取 Markdown。
     *
     * 提供給作者的熱重載鉤子；測試亦用它做案例間隔離。
     */
    void Reload();

    /**
     * @brief 覆寫「本實例」讀取章節檔的目錄（預設 "docs/content"，相對於進程工作目錄解析）。
     * @param dir 新的內容目錄。
     *
     * 於下次（重新）載入時生效。
     */
    void SetContentDir(std::string dir);

    /// @brief 取目前的內容目錄。
    [[nodiscard]] const std::string& ContentDir() const noexcept {
        return contentDir_;
    }

private:
    const LoadedChapter& ChapterFor(SemesterState state);

    std::string                            contentDir_;       ///< 內容目錄
    std::map<SemesterState, LoadedChapter> cache_;            ///< 每狀態一份的章節快取
};

} // namespace nccu::dialog

#endif // GAME_DIALOG_DIALOG_REPOSITORY_H_
