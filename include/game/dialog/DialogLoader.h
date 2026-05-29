#pragma once
#include <map>
#include <string>
#include <vector>

/**
 * @file DialogLoader.h
 * @brief 章節對白 Markdown 的解析資料模型與進入點：把一份 docs/content/<chapter>.md
 *        解析成各 NPC 的子段（SubEntry）集合。
 */
namespace nccu::dialog {

/**
 * @brief NPC 段落中一個解析後的「### (x) ...」子段。
 *
 * 承載每個子段的 metadata；各欄位來源如下：
 *  - subState：子段字母對應的整數（a→0 b→1 c→2 d→3），其中 'a' 為開場。
 *  - lines：此子段依序的對白行（若標題下無 `- "台詞"` 項則可能為空）。
 *  - choiceLabel：由標題文字導出的選單標籤。標題中的 「…」 引用段是作者明確指定的覆寫值，
 *    優先採用；否則取「(x)」標記之後、並剝除尾端 （…） 註解的標題文字。"" 表示非選項。
 *  - karmaDelta：此子段 `>` 引言註記中找到的第一個非零 `// karma ±N`（0 = 無／明確 +0）。
 *  - setsFlag／flagValue：同一批引言註記中找到的第一個 `Flag_X = true|false`
 *    （"" = 無；flagValue 預設 false）。
 */
struct SubEntry {
    int                      subState;        ///< 子段索引（a→0 b→1 c→2 d→3）
    std::vector<std::string> lines;           ///< 此子段依序的對白行
    std::string              choiceLabel;     ///< 選單標籤；"" 表示非選項
    int                      karmaDelta = 0;  ///< 業力增減；0 = 無
    std::string              setsFlag;        ///< 要設定的旗標名；"" 表示無
    bool                     flagValue = false; ///< setsFlag 要設定的布林值
};

/**
 * @brief 單一章節 NPC 對白內容的記憶體表示，由一份 docs/content/<chapter>.md 解析而得。
 *
 * key 為中文 NPC 名（例如 "西裝學長"）；value 為該 NPC 依 subState 遞增排序的子段集合。
 */
struct LoadedChapter {
    std::map<std::string, std::vector<SubEntry>> npcs;
};

/**
 * @brief 解析位於 `path` 的單一章節 Markdown 檔，回傳解析後的 NPC 對白資料。
 * @param path 章節 Markdown 檔路徑。
 * @return 解析結果；若檔案無法開啟則回傳空的 LoadedChapter（不丟出例外）。
 */
LoadedChapter LoadChapter(const std::string& path);

}  // namespace nccu::dialog
