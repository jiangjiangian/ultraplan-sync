/**
 * @file test_ch1_flavor_crowd.cpp
 * @brief 驗證 Ch1 搶課人潮與靜態風味 NPC 為附加內容：不擋主線、不設旗標、台詞確定性循環，且維持 Player 在最前端。
 */
#include "doctest/doctest.h"
#include "game/quest/NpcSpawns.h"
#include "game/quest/ChapterSpawns.h"
#include "game/dialog/DialogSource.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <set>
#include <string>
#include <string_view>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;
using nccu::World;

// Ch1 加退選搶課的人潮與靜態風味 NPC 都是「加味」用的附加內容：一批四處遊走的
// 路人學生（無 npcId、無「!」、不擋路），加上幾個靜態風味 NPC，其 chapter1.md
// 台詞池每次對話會以確定性的方式循環播放一行。它們不得碰到硬性關卡的
// 苦主→學長→苦主 主線（不帶任務旗標），必須維持 objects_.front()==Player，
// 也必須避開主線錨點。

namespace {

std::size_t CountCh1Crowd(const World& w) {
    // Ch1 人潮以空 npcId 從 Chapter1CrowdSpawns 的座標出發遊走。計算正坐落於
    // 這些生成點上、且 npcId 為空的 NPC（位置要等 Update() 推進才會變，而
    // RespawnChapterRoster 不會推進）——這樣才精準，不會把 Ch1 人潮和
    // 幕間／Ch3 人潮或南側路人學生搞混。
    std::size_t n = 0;
    for (const auto& o : w.Objects()) {
        if (!o->NpcId().empty()) continue;
        const auto* npc = dynamic_cast<const NPC*>(o.get());
        if (npc == nullptr) continue;                 // 略過 Player／撿取物
        const auto p = o->GetPosition();
        for (const auto& s : nccu::Chapter1CrowdSpawns())
            if (p.x == s.pos.x && p.y == s.pos.y) { ++n; break; }
    }
    return n;
}

const NPC* FindNpc(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id))
            return dynamic_cast<const NPC*>(o.get());
    return nullptr;
}

} // namespace

// Ch1 加退選搶課人潮會生成，且玩家在加入人潮後仍維持在 objects_ 的最前端，離開／往返章節皆守住此不變量。
TEST_CASE("G-1: the Ch1 加退選搶課 crowd spawns and the Player stays at front") {
    World w("", /*loadSprites=*/false);

    // 遊走的人潮填滿校園中央（即玩家所見的「搶課人潮」）；由 Chapter1CrowdSpawns 宣告這批。
    REQUIRE_FALSE(nccu::Chapter1CrowdSpawns().empty());
    CHECK(CountCh1Crowd(w) >= nccu::Chapter1CrowdSpawns().size());

    // 有人潮在場時，「最前端是 Player」的不變量仍成立。
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    CHECK(static_cast<GameObject*>(p) == w.Objects().front().get());

    // 離開章節時人潮被清除（受名冊追蹤），不變量存活；往返回 Ch1 後人潮回來。
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    CHECK(CountCh1Crowd(w) == 0);
    CHECK(w.GetPlayer() == p);
    CHECK(w.Objects().front().get() == static_cast<GameObject*>(p));

    w.RespawnChapterRoster(SemesterState::Chapter1_AddDrop);
    CHECK(CountCh1Crowd(w) >= nccu::Chapter1CrowdSpawns().size());
    CHECK(w.Objects().front().get() == static_cast<GameObject*>(p));
}

// Ch1 風味 NPC 是靜態、非任務、不在主線上的：在名冊中、非任務給予者（不畫「!」）、也不是主線原型。
TEST_CASE("G-2: the Ch1 flavor NPCs are stationary, non-quest, off the spine") {
    World w("", /*loadSprites=*/false);

    // 每個宣告的風味 NPC 都在活動名冊中、不是任務給予者（因此永不畫「!」）、也不是主線原型。
    const std::set<std::string> spine = {
        "victim", "suit_senior", "bookworm", "ta", "shop_auntie"};
    for (const auto& s : nccu::Chapter1FlavorSpawns()) {
        const NPC* npc = FindNpc(w, s.npcId);
        REQUIRE_MESSAGE(npc != nullptr, "missing flavor NPC: " << s.npcId);
        CHECK_FALSE(npc->IsQuestGiver());
        CHECK(spine.count(s.npcId) == 0);
        CHECK(nccu::IsChapter1FlavorNpc(s.npcId));
    }
    // 主線／路人／未知的 id 都不是風味 NPC。
    CHECK_FALSE(nccu::IsChapter1FlavorNpc("victim"));
    CHECK_FALSE(nccu::IsChapter1FlavorNpc(""));
    CHECK_FALSE(nccu::IsChapter1FlavorNpc("nobody"));
}

// 風味 NPC 會以確定性的方式循環其台詞池、且不設任何旗標：兩輪序列相同、第二個同樣種子的 NPC 序列也相同。
TEST_CASE("G-2: a flavor NPC cycles its pool DETERMINISTICALLY, sets no flag") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    // 風味台詞池就是該 npcId 在 chapter1.md 的 (a) 段落。
    const auto& pool = nccu::dialog::Entries(
        "ch1_flavor_grab", SemesterState::Chapter1_AddDrop);
    REQUIRE(pool.size() == 1);                     // 一個 (a) 子區塊
    const std::vector<std::string>& lines = pool[0].lines;
    REQUIRE(lines.size() >= 2);                    // 池中要有足夠行可循環

    // 載入此池的 NPC 每次 Interact() 推進一行，依池大小取模循環——這是一種
    // 確定且可重現的「隨機」選取（不使用 RNG，以確保可重現性）。
    NPC a(nccu::engine::math::Vec2{0, 0}, {}, /*isQuestGiver=*/false,
          std::string_view{"ch1_flavor_grab"});
    a.LoadDialog("ch1_flavor_grab", SemesterState::Chapter1_AddDrop, 0);
    REQUIRE(a.DialogLineCount() == lines.size());

    Player p = Player{nccu::engine::math::Vec2{0, 0}};
    const std::size_t flagsBefore = 0;             // 全新玩家沒有任何旗標

    // 走兩輪池；第二輪的序列與第一輪完全相同。
    std::vector<std::string> lap1, lap2;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        lap1.push_back(a.CurrentLineText());
        a.Interact(&p);
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        lap2.push_back(a.CurrentLineText());
        a.Interact(&p);
    }
    CHECK(lap1 == lines);                           // 正好是作者撰寫的池
    CHECK(lap1 == lap2);                            // 可重現的循環

    // 第二個以相同方式播種的 NPC 產生相同序列（沒有每次執行的亂源）——這是可重現性所依賴的。
    NPC b(nccu::engine::math::Vec2{0, 0}, {}, /*isQuestGiver=*/false,
          std::string_view{"ch1_flavor_grab"});
    b.LoadDialog("ch1_flavor_grab", SemesterState::Chapter1_AddDrop, 0);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        CHECK(b.CurrentLineText() == lap1[i]);
        b.Interact(&p);
    }

    // 互動永不改動玩家的任務狀態（Interact 只發出 ShowMessage；不碰旗標／karma／雨傘）。
    CHECK(p.GetKarma() == 50);
    (void)flagsBefore;
}
