#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogLoader.h"

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

    // (b) T1 reframe: the Ch1 (b) choice is no longer a hostile 斥責 (-5)
    // but a RATIONAL firm call-out — 「理性指出他品行不該，要回雨傘」 —
    // scanned as `// karma +3` (positive small-good) and
    // `Flag_ScoldedSenior = true` (first Flag_ line wins). The flag KEY is
    // retained so the "保持距離" arc (Ch2 (c) 尷尬讓開 / Ch3 距離 / Ch4
    // 不出場) still routes; only the framing (embarrassment, not resentment)
    // and the karma sign changed. First-person POV: NO 「玩家」 subject in
    // the choice label. Re-authored as substate (b) — DialogLoader.cpp
    // hard-caps substate letters at 'a'..'d'; choice indices 0/1/2 stay
    // b/c/d (ending scripts byte-identical).
    CHECK(s_b->karmaDelta == 3);
    CHECK(s_b->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(s_b->flagValue == true);
    CHECK(s_b->choiceLabel == "理性指出他品行不該，要回雨傘");

    // (c) blockquote: `// karma 0`, `// Flag_ScoldedSenior = false`.
    // T1 reframe: 接受取傘 is karma-neutral at the moment of choice (the
    // real cost is the later ProfTrap recognition / ripples). First-person
    // label, no 「玩家」 subject. suit_senior subState 2, 5 dialog lines.
    REQUIRE(s_c->lines.size() == 5);
    CHECK(s_c->karmaDelta == 0);
    CHECK(s_c->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(s_c->flagValue == false);
    CHECK(s_c->choiceLabel == "接受，取傘後交給學長");

    // (d) the rational best option (C.2). Blockquote order: `// karma +5`,
    // `// Flag_HelpedSenior = true`, `// Flag_ScoldedSenior = false` —
    // first Flag_ line wins, so setsFlag is Flag_HelpedSenior / true.
    // T1 reframe: raised +3 -> +5 (best rational communication option).
    // 5 lines; first-person label from heading (no 「」/（）override).
    REQUIRE(s_d->lines.size() == 5);
    CHECK(s_d->lines[0] == "……怪怪的？你什麼意思？");
    CHECK(s_d->karmaDelta == 5);
    CHECK(s_d->setsFlag == nccu::kFlagHelpedSenior);
    CHECK(s_d->flagValue == true);
    CHECK(s_d->choiceLabel == "點破傘的疑點，轉而提供正規協助");

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
    // victim subState 1, 5 dialog lines, setsFlag nccu::kFlagPromisedVictim
    // -> true, choiceLabel "我去幫你追".
    const auto& victim = chapter.npcs.at("苦主");
    const auto* v_b = Find(victim, 1);
    REQUIRE(v_b != nullptr);
    CHECK(v_b->choiceLabel == "我去幫你追");
    CHECK(v_b->karmaDelta == 5);
    CHECK(v_b->setsFlag == nccu::kFlagPromisedVictim);
    CHECK(v_b->flagValue == true);

    // Trailing-（…）-stripped case: 苦主 (a) heading
    //   `### (a) 在雨中蹲著（玩家靠近觸發）` -> "在雨中蹲著".
    const auto* v_a = Find(victim, 0);
    REQUIRE(v_a != nullptr);
    CHECK(v_a->choiceLabel == "在雨中蹲著");
}
