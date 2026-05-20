#include "doctest/doctest.h"
#include "DialogLoader.h"

#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// Parity net: the runtime LoadChapter parser must extract, from the REAL
// content file docs/content/chapter1.md, the exact per-substate metadata
// asserted below. Expected values were transcribed by hand from
// chapter1.md and are the permanent contract for the Chapter1_AddDrop
// dialog payload.

namespace {

std::string ContentPath(const char* name) {
    return std::string(TEST_CONTENT_DIR) + "/" + name;
}

const nccu::dialog::SubEntry* Find(
    const std::vector<nccu::dialog::SubEntry>& subs, int subState) {
    for (const auto& s : subs) {
        if (s.subState == subState) return &s;
    }
    return nullptr;
}

}  // namespace

TEST_CASE("LoadChapter: chapter1 real content parity with codegen") {
    const auto chapter =
        nccu::dialog::LoadChapter(ContentPath("chapter1.md"));

    // The chapter file defines six NPC sections (西裝學長, 學霸, 助教,
    // 福利社阿姨, 苦主). The pre-NPC `## 章節 metadata` / `## 場景旁白`
    // blocks and the trailing `## 章節結尾分支提示` are not NPC sections
    // and must be excluded.
    REQUIRE(chapter.npcs.count("西裝學長") == 1);
    REQUIRE(chapter.npcs.count("學霸") == 1);
    REQUIRE(chapter.npcs.count("苦主") == 1);
    CHECK(chapter.npcs.count("場景旁白") == 0);
    CHECK(chapter.npcs.count("章節結尾分支提示") == 0);

    // --- 西裝學長: substates {0,1,2,3} in ascending order -------------
    const auto& senior = chapter.npcs.at("西裝學長");
    REQUIRE(senior.size() == 4);
    CHECK(senior[0].subState == 0);
    CHECK(senior[1].subState == 1);
    CHECK(senior[2].subState == 2);
    CHECK(senior[3].subState == 3);

    const auto* s_a = Find(senior, 0);
    const auto* s_b = Find(senior, 1);
    const auto* s_c = Find(senior, 2);
    const auto* s_d = Find(senior, 3);
    REQUIRE(s_a != nullptr);
    REQUIRE(s_b != nullptr);
    REQUIRE(s_c != nullptr);
    REQUIRE(s_d != nullptr);

    // (a) opener = subState 0, 5 lines, first line verbatim. No karma /
    // flag note in the (a) blockquote -> defaults.
    REQUIRE(s_a->lines.size() == 5);
    CHECK(s_a->lines[0] == "欸，加退選也沒搶到嗎？");
    CHECK(s_a->karmaDelta == 0);
    CHECK(s_a->setsFlag == "");
    CHECK(s_a->flagValue == false);

    // (b) GDD §伍 Ch1 漣漪選項 B「憤怒斥責，奪回雨傘」: was the inert
    // "拒絕，無 flag" stub; cycle-8 audit F2 activated the dead
    // Flag_ScoldedSenior wiring (DialogOpener.cpp:101 + Chapter2Quest.cpp:66
    // + chapter4.md:82/88/405 read it; the GDD-named cold-senior cross-
    // chapter arc was permanently unreachable pre-fix). Blockquote scans
    // `// karma -5` (confrontational tier, mirrors (c) -5) and
    // `Flag_ScoldedSenior = true` (first Flag_ line wins). Re-authored as
    // substate (b) — not (e) — because DialogLoader.cpp:83 hard-caps
    // substate letters at 'a'..'d'; only ending_a.txt picks suit_senior
    // (`choose 2` = (d) HelpedSenior), so re-authoring (b) leaves the
    // deterministic ending scripts byte-identical (choice indices 0/1/2
    // still b/c/d, ending_a still picks (d)).
    CHECK(s_b->karmaDelta == -5);
    CHECK(s_b->setsFlag == "Flag_ScoldedSenior");
    CHECK(s_b->flagValue == true);
    CHECK(s_b->choiceLabel == "玩家憤怒斥責，奪回雨傘");

    // (c) blockquote: `// karma -5`, `// Flag_ScoldedSenior = false`.
    // Expected: suit_senior subState 2, 5 dialog lines, karmaDelta -5,
    // setsFlag "Flag_ScoldedSenior" -> false,
    // choiceLabel "玩家接受，取傘後交給學長".
    REQUIRE(s_c->lines.size() == 5);
    CHECK(s_c->karmaDelta == -5);
    CHECK(s_c->setsFlag == "Flag_ScoldedSenior");
    CHECK(s_c->flagValue == false);
    CHECK(s_c->choiceLabel == "玩家接受，取傘後交給學長");

    // (d) NEW positive branch (C.2). Blockquote order: `// karma +3`,
    // `// Flag_HelpedSenior = true`, `// Flag_ScoldedSenior = false` —
    // first Flag_ line wins, so setsFlag is Flag_HelpedSenior / true.
    // 5 lines; choiceLabel from heading (no 「」/（）override).
    REQUIRE(s_d->lines.size() == 5);
    CHECK(s_d->lines[0] == "……怪怪的？你什麼意思？");
    CHECK(s_d->karmaDelta == 3);
    CHECK(s_d->setsFlag == "Flag_HelpedSenior");
    CHECK(s_d->flagValue == true);
    CHECK(s_d->choiceLabel == "玩家點破傘的疑點，轉而提供正規協助");

    // --- 學霸: (a) subState 0; (b) karmaDelta == 3 -------------------
    const auto& bookworm = chapter.npcs.at("學霸");
    REQUIRE(bookworm.size() == 3);
    const auto* b_a = Find(bookworm, 0);
    const auto* b_b = Find(bookworm, 1);
    REQUIRE(b_a != nullptr);
    REQUIRE(b_b != nullptr);
    CHECK(b_a->subState == 0);
    CHECK(b_a->karmaDelta == 0);
    CHECK(b_a->setsFlag == "");
    // (b) blockquote: `// karma +3` (虛心受教). No flag.
    CHECK(b_b->subState == 1);
    CHECK(b_b->karmaDelta == 3);
    CHECK(b_b->setsFlag == "");
    CHECK(b_b->flagValue == false);

    // --- choiceLabel derivation ------------------------------------------
    // No-override case: 西裝學長 (c) heading `### (c) 玩家接受，取傘後交
    // 給學長` has no 「…」 and no trailing （…） -> verbatim (asserted
    // above as "玩家接受，取傘後交給學長").
    //
    // 「…」 author-override case: 苦主 (b) heading is
    //   `### (b) 玩家給予安慰（選擇「我去幫你追」）`
    // The 「我去幫你追」 span wins over the surrounding （…）. Expected:
    // victim subState 1, 5 dialog lines, setsFlag "Flag_PromisedVictim"
    // -> true, choiceLabel "我去幫你追".
    const auto& victim = chapter.npcs.at("苦主");
    const auto* v_b = Find(victim, 1);
    REQUIRE(v_b != nullptr);
    CHECK(v_b->choiceLabel == "我去幫你追");
    CHECK(v_b->karmaDelta == 5);
    CHECK(v_b->setsFlag == "Flag_PromisedVictim");
    CHECK(v_b->flagValue == true);

    // Trailing-（…）-stripped case: 苦主 (a) heading
    //   `### (a) 在雨中蹲著（玩家靠近觸發）` -> "在雨中蹲著".
    const auto* v_a = Find(victim, 0);
    REQUIRE(v_a != nullptr);
    CHECK(v_a->choiceLabel == "在雨中蹲著");
}
