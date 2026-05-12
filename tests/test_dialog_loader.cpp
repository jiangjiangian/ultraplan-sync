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

}  // namespace

TEST_CASE("DialogLoader: parses fixture into expected NPC + substate layout") {
    const auto chapter = nccu::dialog::LoadChapter(FixturePath("dialog_sample.md"));

    // Two NPCs: 學長 and 學妹. Pre-NPC headings (`## 章節 metadata`,
    // `## 場景旁白`) and the post-NPC `## 章節結尾` heading must reset
    // the active NPC and therefore be excluded.
    REQUIRE(chapter.npcs.size() == 2);
    REQUIRE(chapter.npcs.count("學長") == 1);
    REQUIRE(chapter.npcs.count("學妹") == 1);

    // 學長: substates a (2 lines, ASCII quotes), b (1 line).
    const auto& senior = chapter.npcs.at("學長");
    REQUIRE(senior.count('a') == 1);
    REQUIRE(senior.count('b') == 1);
    CHECK(senior.at('a').size() == 2);
    CHECK(senior.at('a')[0] == "嗨，學弟。");
    CHECK(senior.at('a')[1] == "你也來測試嗎？");
    CHECK(senior.at('b').size() == 1);
    CHECK(senior.at('b')[0] == "又見面了。");
}
