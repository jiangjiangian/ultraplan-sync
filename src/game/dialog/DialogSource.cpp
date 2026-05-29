#include "game/dialog/DialogSource.h"
#include "game/dialog/DialogRepository.h"

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nccu::dialog {

namespace {

// 英文 NPC id（對話消費端使用）→ LoadChapter 用來索引其 map 的中文 section 名。
// 不可變純資料——跨 DialogRepository 實例共用，不屬任何實例的可變狀態。
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
        // Ch3 物物交換鏈三節點。section 名逐字對應 chapter3.md「## NPC：A 系烤香腸
        // 攤主 / B 系大聲公持有者 / C 系學姊」（含 A/B/C 後的 ASCII 空白，ParseNpcName
        // 只剝前後空白與尾端 （…），內部空白原樣保留）。
        {"vendor_sausage_a", "A 系烤香腸攤主"},
        {"loudspeaker_b",    "B 系大聲公持有者"},
        {"senior_c",         "C 系學姊"},
        // Ch1 搶課氣氛 NPC（純風味、無任務、無旗標）：section 名對應 chapter1.md
        //「## NPC：搶課同學 / 撐傘路人 / 揹書包學生」。各自 (a) 段的多行 = 該 NPC 的
        // 台詞池，由 World::SpawnChapterNpcs 在生成時透過 NPC::LoadDialog 灌入
        // dialogLines_，再由 GameController 的 E 互動路由到 NPC::Interact() 逐句循環
        //（決定性、可重現、不碰主線）。
        {"ch1_flavor_grab",  "搶課同學"},
        {"ch1_flavor_rain",  "撐傘路人"},
        {"ch1_flavor_bag",   "揹書包學生"},
    };
    return kTable;
}

// SemesterState → 內容目錄下對應的章節 Markdown 檔。
// 不可變純資料——如上方 NpcNameTable，跨實例共用。
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

// 進程預設 DialogRepository。以函式內 static 持有，使其建構執行緒安全且延遲
//（與既有的 ContentDir/Cache 慣用作法一致）。下方接縫（SetRepository/Repository）
// 優先採用注入的覆寫值，未設定時才落到此預設。
DialogRepository& DefaultRepository() {
    static DialogRepository repo;
    return repo;
}

DialogRepository* g_override = nullptr;

} // namespace

// ── DialogRepository 方法 ─────────────────────────────────────────

DialogRepository::DialogRepository()
    : contentDir_("docs/content") {}

const LoadedChapter& DialogRepository::ChapterFor(SemesterState state) {
    auto it = cache_.find(state);
    if (it != cache_.end()) return it->second;

    const auto& files = ChapterFileTable();
    auto fileIt = files.find(state);
    if (fileIt == files.end()) {
        // 未知狀態：快取一份空章節，避免反覆查找
        return cache_.emplace(state, LoadedChapter{}).first->second;
    }

    const std::string path = contentDir_ + "/" + fileIt->second;
    return cache_.emplace(state, LoadChapter(path)).first->second;
}

const std::vector<SubEntry>& DialogRepository::Entries(
        std::string_view npcId, SemesterState state) {
    static const std::vector<SubEntry> kEmpty;

    const auto& names = NpcNameTable();
    auto nameIt = names.find(npcId);
    if (nameIt == names.end()) return kEmpty;

    const LoadedChapter& chapter = ChapterFor(state);
    auto npcIt = chapter.npcs.find(nameIt->second);
    if (npcIt == chapter.npcs.end()) return kEmpty;

    return npcIt->second;
}

void DialogRepository::Reload() {
    cache_.clear();
}

void DialogRepository::SetContentDir(std::string dir) {
    contentDir_ = std::move(dir);
}

// ── 接縫（進程當前倉儲存取器） ────────────────────────────────────

void SetRepository(DialogRepository* repo) noexcept {
    g_override = repo;
}

DialogRepository& Repository() noexcept {
    return g_override ? *g_override : DefaultRepository();
}

// ── 自由函式委派（維持既有 API 不變） ─────────────────────────────

const std::vector<SubEntry>& Entries(std::string_view npcId,
                                     SemesterState state) {
    return Repository().Entries(npcId, state);
}

void Reload() {
    Repository().Reload();
}

void SetContentDir(std::string dir) {
    Repository().SetContentDir(std::move(dir));
}

} // namespace nccu::dialog
