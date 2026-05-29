#include "doctest/doctest.h"
#include "game/dialog/DialogLoader.h"

#include <string>

#ifndef TEST_FIXTURES_DIR
#error "TEST_FIXTURES_DIR must be defined by the build system"
#endif

/**
 * @file test_dialog_loader.cpp
 * @brief 驗證 DialogLoader::LoadChapter 能把 markdown 樣本解析成預期的 NPC 與子狀態
 *        結構：NPC 區段切分、子狀態升序排列、台詞內容、空子區塊保留，以及未標註
 *        時各 metadata 欄位的預設值。
 */

namespace {

std::string FixturePath(const char* name) {
    return std::string(TEST_FIXTURES_DIR) + "/" + name;
}

// 依 subState 整數（a=0, b=1, c=2, d=3）查找某 NPC 的 SubEntry，找不到回傳 nullptr。
const nccu::dialog::SubEntry* Find(
    const std::vector<nccu::dialog::SubEntry>& subs, int subState) {
    for (const auto& s : subs) {
        if (s.subState == subState) return &s;
    }
    return nullptr;
}

}  // namespace

// LoadChapter 把樣本解析成預期的 NPC 與子狀態結構。
TEST_CASE("DialogLoader: parses fixture into expected NPC + substate layout") {
    const auto chapter =
        nccu::dialog::LoadChapter(FixturePath("dialog_sample.md"));

    // 兩位 NPC：學長與學妹。NPC 之前的標題（## 章節 metadata、## 場景旁白）
    // 與 NPC 之後的 ## 章節結尾 標題都會重置目前 NPC，因此會被排除。
    REQUIRE(chapter.npcs.size() == 2);
    REQUIRE(chapter.npcs.count("學長") == 1);
    REQUIRE(chapter.npcs.count("學妹") == 1);

    // 學長：子狀態 a（subState 0，2 行，ASCII 引號）、b（subState 1，1 行）。
    // 各項目依 subState 升序排列。
    const auto& senior = chapter.npcs.at("學長");
    REQUIRE(senior.size() == 2);
    CHECK(senior[0].subState == 0);
    CHECK(senior[1].subState == 1);

    const auto* sa = Find(senior, 0);
    const auto* sb = Find(senior, 1);
    REQUIRE(sa != nullptr);
    REQUIRE(sb != nullptr);

    REQUIRE(sa->lines.size() == 2);
    CHECK(sa->lines[0] == "嗨，學弟。");
    CHECK(sa->lines[1] == "你也來測試嗎？");
    REQUIRE(sb->lines.size() == 1);
    CHECK(sb->lines[0] == "又見面了。");

    // 樣本的標題沒有 「…」 覆寫，也沒有結尾的 （…） 括註，因此 choiceLabel
    // 就是標題文字本身。樣本未使用任何 karma 或 Flag_X = 標註，所以 metadata
    // 欄位維持預設值（那些由 chapter1 的對照測試涵蓋）。
    CHECK(sa->choiceLabel == "初次接觸");
    CHECK(sa->karmaDelta == 0);
    CHECK(sa->setsFlag == "");
    CHECK(sa->flagValue == false);
    CHECK(sb->choiceLabel == "二次接觸");
    CHECK(sb->karmaDelta == 0);
    CHECK(sb->setsFlag == "");
    CHECK(sb->flagValue == false);

    // 學妹：子狀態 a（subState 0）用 CJK 引號 “…”，而子狀態 c（subState 2）
    // 是一個沒有任何 - "line" 項目的標題——它仍必須產生一個 lines 為空的
    // SubEntry（LoadChapter 會保留空的子區塊）。
    const auto& junior = chapter.npcs.at("學妹");
    REQUIRE(junior.size() == 2);
    CHECK(junior[0].subState == 0);
    CHECK(junior[1].subState == 2);

    const auto* ja = Find(junior, 0);
    const auto* jc = Find(junior, 2);
    REQUIRE(ja != nullptr);
    REQUIRE(jc != nullptr);

    REQUIRE(ja->lines.size() == 2);
    CHECK(ja->lines[0] == "學長你好。");
    CHECK(ja->lines[1] == "今天天氣真好。");
    CHECK(ja->choiceLabel == "招呼");

    CHECK(jc->lines.empty());
    CHECK(jc->choiceLabel == "沒台詞的 substate");
    CHECK(jc->karmaDelta == 0);
    CHECK(jc->setsFlag == "");
    CHECK(jc->flagValue == false);
}
