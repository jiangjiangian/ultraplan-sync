/**
 * @file test_loadchapter_chapter1.cpp
 * @brief 對照網：驗證 LoadChapter 解析器從實際的 chapter1.md 內容檔，抽出每個子狀態的台詞數、karma、旗標與選項標籤。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogLoader.h"

#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// 對照網：執行期的 LoadChapter 解析器，必須從實際內容檔 docs/content/chapter1.md
// 抽出下方斷言的每個子狀態 metadata。預期值是手動從 chapter1.md 謄錄而來，
// 是 Chapter1_AddDrop 對話資料的永久契約。

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

// 對照實際 chapter1.md 內容：NPC 段落的辨識，以及各子狀態的台詞數、karma、旗標與選項標籤推導皆正確。
TEST_CASE("LoadChapter: chapter1 real content parity with codegen") {
    const auto chapter =
        nccu::dialog::LoadChapter(ContentPath("chapter1.md"));

    // 章節檔定義了數個 NPC 段落（西裝學長、學霸、助教、福利社阿姨、苦主）。
    // NPC 之前的「## 章節 metadata」「## 場景旁白」區塊，以及結尾的
    // 「## 章節結尾分支提示」都不是 NPC 段落，必須被排除。
    REQUIRE(chapter.npcs.count("西裝學長") == 1);
    REQUIRE(chapter.npcs.count("學霸") == 1);
    REQUIRE(chapter.npcs.count("苦主") == 1);
    CHECK(chapter.npcs.count("場景旁白") == 0);
    CHECK(chapter.npcs.count("章節結尾分支提示") == 0);

    // --- 西裝學長：子狀態 {0,1,2,3} 由小到大排列 -------------
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

    // (a) 開場 = 子狀態 0，5 行，首行逐字比對。(a) 的引言區沒有 karma /
    // 旗標標註 -> 採預設值。
    REQUIRE(s_a->lines.size() == 5);
    CHECK(s_a->lines[0] == "欸，加退選也沒搶到嗎？");
    CHECK(s_a->karmaDelta == 0);
    CHECK(s_a->setsFlag == "");
    CHECK(s_a->flagValue == false);

    // (b) Ch1 的 (b) 選擇不再是帶敵意的斥責（-5），而是理性而堅定的指正——
    // 「理性指出他品行不該，要回雨傘」——在內容檔被掃描為 `// karma +3`
    //（小善、正分）與 `Flag_ScoldedSenior = true`（以第一個 Flag_ 行為準）。
    // 旗標鍵保留，使「保持距離」這條支線（Ch2 (c) 尷尬讓開 / Ch3 距離 /
    // Ch4 不出場）仍能路由；只有框架（尷尬而非怨恨）與 karma 正負號改變。
    // 第一人稱視角：選項標籤中沒有「玩家」這個主詞。
    CHECK(s_b->karmaDelta == 3);
    CHECK(s_b->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(s_b->flagValue == true);
    CHECK(s_b->choiceLabel == "理性指出他品行不該，要回雨傘");

    // (c) 引言區：`// karma 0`、`// Flag_ScoldedSenior = false`。
    // 接受取傘在做選擇的當下是 karma 中性（真正的代價是後續認出 ProfTrap／漣漪）。
    // 第一人稱標籤、無「玩家」主詞。suit_senior 子狀態 2，5 行對白。
    REQUIRE(s_c->lines.size() == 5);
    CHECK(s_c->karmaDelta == 0);
    CHECK(s_c->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(s_c->flagValue == false);
    CHECK(s_c->choiceLabel == "接受，取傘後交給學長");

    // (d) 理性的最佳選項。引言區順序：`// karma +5`、
    // `// Flag_HelpedSenior = true`、`// Flag_ScoldedSenior = false`——
    // 以第一個 Flag_ 行為準，故 setsFlag 為 Flag_HelpedSenior / true。
    // karma 調為 +5（最佳的理性溝通選項）。5 行；標籤直接取自標題
    //（沒有「」／（）覆寫）。
    REQUIRE(s_d->lines.size() == 5);
    CHECK(s_d->lines[0] == "……怪怪的？你什麼意思？");
    CHECK(s_d->karmaDelta == 5);
    CHECK(s_d->setsFlag == nccu::kFlagHelpedSenior);
    CHECK(s_d->flagValue == true);
    CHECK(s_d->choiceLabel == "點破傘的疑點，轉而提供正規協助");

    // --- 學霸：(a) 子狀態 0；(b) karmaDelta == 3 -------------------
    const auto& bookworm = chapter.npcs.at("學霸");
    REQUIRE(bookworm.size() == 3);
    const auto* b_a = Find(bookworm, 0);
    const auto* b_b = Find(bookworm, 1);
    REQUIRE(b_a != nullptr);
    REQUIRE(b_b != nullptr);
    CHECK(b_a->subState == 0);
    CHECK(b_a->karmaDelta == 0);
    CHECK(b_a->setsFlag == "");
    // (b) 引言區：`// karma +3`（虛心受教）。無旗標。
    CHECK(b_b->subState == 1);
    CHECK(b_b->karmaDelta == 3);
    CHECK(b_b->setsFlag == "");
    CHECK(b_b->flagValue == false);

    // --- choiceLabel 的推導 ------------------------------------------
    // 無覆寫的情況：西裝學長 (c) 標題「### (c) 玩家接受，取傘後交給學長」
    // 沒有「…」也沒有結尾的（…）-> 逐字採用（上方斷言為
    // "玩家接受，取傘後交給學長"）。
    //
    // 作者用「…」覆寫的情況：苦主 (b) 標題為
    //   「### (b) 玩家給予安慰（選擇「我去幫你追」）」
    // 其中「我去幫你追」這段勝過外圍的（…）。預期：victim 子狀態 1、
    // 5 行對白、setsFlag 為 nccu::kFlagPromisedVictim -> true、
    // choiceLabel 為 "我去幫你追"。
    const auto& victim = chapter.npcs.at("苦主");
    const auto* v_b = Find(victim, 1);
    REQUIRE(v_b != nullptr);
    CHECK(v_b->choiceLabel == "我去幫你追");
    CHECK(v_b->karmaDelta == 5);
    CHECK(v_b->setsFlag == nccu::kFlagPromisedVictim);
    CHECK(v_b->flagValue == true);

    // 去除結尾（…）的情況：苦主 (a) 標題
    //   「### (a) 在雨中蹲著（玩家靠近觸發）」 -> "在雨中蹲著"。
    const auto* v_a = Find(victim, 0);
    REQUIRE(v_a != nullptr);
    CHECK(v_a->choiceLabel == "在雨中蹲著");
}
