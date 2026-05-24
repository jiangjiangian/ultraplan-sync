#include "dialog/DialogSource.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nccu::dialog {

namespace {

// English NPC id (as carried by the codegen Entry / used by the dialog
// consumers) -> the Chinese section name LoadChapter keys its map on.
const std::map<std::string_view, std::string>& NpcNameTable() {
    static const std::map<std::string_view, std::string> kTable = {
        {"victim",       "苦主"},
        {"suit_senior",  "西裝學長"},
        {"bookworm",     "學霸"},
        {"ta",           "助教"},
        {"shop_auntie",  "福利社阿姨"},
        // Ch2 新角色（圖書館管理員）：純資訊 quest-giver，推進主線的關鍵
        // NPC。section 名對應 chapter2.md「## NPC：圖書館管理員」。
        {"librarian",    "圖書館管理員"},
        // Ch3 物物交換鏈三節點（S5d-1）。section 名逐字對應
        // chapter3.md「## NPC：A 系烤香腸攤主 / B 系大聲公持有者 /
        // C 系學姊」（含 A/B/C 後的 ASCII 空白，ParseNpcName 只剝
        // 前後空白與尾端 （…），內部空白原樣保留）。
        {"vendor_sausage_a", "A 系烤香腸攤主"},
        {"loudspeaker_b",    "B 系大聲公持有者"},
        {"senior_c",         "C 系學姊"},
        // G-2 Ch1 搶課氣氛 NPC（純風味、無任務、無旗標）：section 名對應
        // chapter1.md「## NPC：搶課同學 / 撐傘路人 / 揹書包學生」。各自
        // (a) 段的多行 = 該 NPC 的台詞池，由 World::SpawnChapterNpcs 在生成
        // 時透過 NPC::LoadDialog 灌入 dialogLines_，再由 GameController 的
        // E 互動路由到 NPC::Interact() 逐句循環（決定性、可重現、不碰主線）。
        {"ch1_flavor_grab",  "搶課同學"},
        {"ch1_flavor_rain",  "撐傘路人"},
        {"ch1_flavor_bag",   "揹書包學生"},
    };
    return kTable;
}

// SemesterState -> the chapter markdown file under the content dir.
const std::map<SemesterState, std::string>& ChapterFileTable() {
    static const std::map<SemesterState, std::string> kTable = {
        {SemesterState::Chapter1_AddDrop,   "chapter1.md"},
        {SemesterState::Interlude_Market,   "interlude_market.md"},
        {SemesterState::Chapter2_Midterms,  "chapter2.md"},
        {SemesterState::Chapter3_SportsDay, "chapter3.md"},
        {SemesterState::Chapter4_Finals,    "chapter4.md"},
        {SemesterState::Ending_A,           "ending_a.md"},
        {SemesterState::Ending_B,           "ending_b.md"},
        {SemesterState::Ending_C,           "ending_c.md"},
    };
    return kTable;
}

std::string& ContentDir() {
    static std::string dir = "docs/content";
    return dir;
}

// One parsed chapter per state. std::map is node-stable: inserting a
// new state never moves the vectors already stored, so references
// handed out by Entries() survive later loads of other states. Only
// Reload()'s clear() invalidates them.
std::map<SemesterState, LoadedChapter>& Cache() {
    static std::map<SemesterState, LoadedChapter> cache;
    return cache;
}

// Loads the chapter for `state` once and returns the cached parse.
// A file that could not be opened still gets cached (LoadChapter
// yields an empty LoadedChapter) so repeated misses don't re-read.
const LoadedChapter& ChapterFor(SemesterState state) {
    auto& cache = Cache();
    auto it = cache.find(state);
    if (it != cache.end()) return it->second;

    const auto& files = ChapterFileTable();
    auto fileIt = files.find(state);
    if (fileIt == files.end()) {
        // Unknown state: cache an empty chapter so we stop looking.
        return cache.emplace(state, LoadedChapter{}).first->second;
    }

    const std::string path = ContentDir() + "/" + fileIt->second;
    return cache.emplace(state, LoadChapter(path)).first->second;
}

}  // namespace

const std::vector<SubEntry>& Entries(std::string_view npcId,
                                     SemesterState state) {
    static const std::vector<SubEntry> kEmpty;

    const auto& names = NpcNameTable();
    auto nameIt = names.find(npcId);
    if (nameIt == names.end()) return kEmpty;

    const LoadedChapter& chapter = ChapterFor(state);
    auto npcIt = chapter.npcs.find(nameIt->second);
    if (npcIt == chapter.npcs.end()) return kEmpty;

    return npcIt->second;
}

void Reload() {
    Cache().clear();
}

void SetContentDir(std::string dir) {
    ContentDir() = std::move(dir);
}

}  // namespace nccu::dialog
