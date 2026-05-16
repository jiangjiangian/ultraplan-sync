#include "doctest/doctest.h"
#include "DialogLoader.h"

#include <string>

#ifndef TEST_FIXTURES_DIR
#error "TEST_FIXTURES_DIR must be defined by the build system"
#endif

namespace {

std::string FixturePath(const char* name) {
    return std::string(TEST_FIXTURES_DIR) + "/" + name;
}

// Look up an NPC's SubEntry by subState int (a=0, b=1, c=2, d=3). Returns
// nullptr if absent.
const nccu::dialog::SubEntry* Find(
    const std::vector<nccu::dialog::SubEntry>& subs, int subState) {
    for (const auto& s : subs) {
        if (s.subState == subState) return &s;
    }
    return nullptr;
}

}  // namespace

TEST_CASE("DialogLoader: parses fixture into expected NPC + substate layout") {
    const auto chapter =
        nccu::dialog::LoadChapter(FixturePath("dialog_sample.md"));

    // Two NPCs: 學長 and 學妹. Pre-NPC headings (`## 章節 metadata`,
    // `## 場景旁白`) and the post-NPC `## 章節結尾` heading must reset
    // the active NPC and therefore be excluded.
    REQUIRE(chapter.npcs.size() == 2);
    REQUIRE(chapter.npcs.count("學長") == 1);
    REQUIRE(chapter.npcs.count("學妹") == 1);

    // 學長: substates a (subState 0, 2 lines, ASCII quotes), b (subState 1,
    // 1 line). Entries are in ascending subState order.
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

    // The fixture headings carry no 「…」 override and no trailing （…）
    // parenthetical, so choiceLabel is the heading text verbatim. The
    // fixture exercises no `// karma` or `Flag_X =` notes, so the metadata
    // fields stay at their defaults (the chapter1 parity test covers those).
    CHECK(sa->choiceLabel == "初次接觸");
    CHECK(sa->karmaDelta == 0);
    CHECK(sa->setsFlag == "");
    CHECK(sa->flagValue == false);
    CHECK(sb->choiceLabel == "二次接觸");
    CHECK(sb->karmaDelta == 0);
    CHECK(sb->setsFlag == "");
    CHECK(sb->flagValue == false);

    // 學妹: substate a (subState 0) uses CJK “…” quotes, and substate c
    // (subState 2) is a header with NO `- "line"` items — it must still
    // produce a SubEntry with an empty `lines` list (LoadChapter preserves
    // empty sub-blocks).
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
