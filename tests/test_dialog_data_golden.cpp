#include "doctest/doctest.h"
#include "DialogData.h"
#include "SemesterState.h"

#include <string>
#include <string_view>
#include <vector>

// Characterization (golden) snapshot of the Chapter1_AddDrop dialog
// content as currently produced by the build-time generated table in
// include/DialogData.h. This locks the exact runtime payload BEFORE the
// dialog runtime is migrated off the codegen table onto the runtime
// LoadChapter parser, so the migration cannot silently regress Chapter 1.
//
// Every expected value below is transcribed faithfully from the current
// include/DialogData.h. If a future change to the source script or the
// parser legitimately alters Chapter 1, update this snapshot deliberately
// in the same commit — do not weaken the assertions.

using nccu::SemesterState;

namespace {

struct GoldenEntry {
    std::string_view npcId;
    int              subState;
    int              lineCount;
    std::string_view firstLine;
    std::string_view lastLine;
    int              karmaDelta;
    std::string_view setsFlag;
    bool             flagValue;
    std::string_view choiceLabel;
};

// Order matches the Chapter1_AddDrop rows of kEntries in DialogData.h
// (kL0..kL15). npcId + subState anchor each row so a reorder or swap
// also trips the snapshot, not just a value drift.
const GoldenEntry kGolden[] = {
    { "__scene__", 0, 4,
      "三月初，加退選系統的伺服器再次撐不住了。",
      "外面的雨，似乎打算整個學期都沒有停的意思。",
      0, "", false, "" },
    { "suit_senior", 0, 5,
      "欸，加退選也沒搶到嗎？",
      "我借的嘛，不是偷的，放心。",
      0, "", false, "初次接觸" },
    { "suit_senior", 1, 3,
      "喔，那算了啦，反正那把傘也不重要。",
      "（轉身繼續滑手機，不再理你）",
      0, "", false, "玩家拒絕拿傘" },
    { "suit_senior", 2, 5,
      "太好了，這才是好學弟／妹嘛。",
      "（走廊遠端傳來急促的腳步聲）",
      -5, "Flag_ScoldedSenior", false, "玩家接受，取傘後交給學長" },
    { "bookworm", 0, 4,
      "加退選系統崩了，你也沒搶到課嗎？",
      "去試試看吧，反正淋雨也不是辦法。",
      0, "", false, "路過搭話" },
    { "bookworm", 1, 4,
      "你說傘不見了？",
      "你快去追，我繼續讀書了。",
      3, "", false, "玩家主動詢問傘或課程資訊" },
    { "bookworm", 2, 3,
      "找到了嗎？",
      "不過你有去試，這樣就夠了。",
      0, "", false, "玩家再次回來對話" },
    { "ta", 0, 5,
      "同學，加退選截止了，現在不受理。",
      "（低頭繼續整理資料夾）",
      0, "", false, "第一次相遇" },
    { "ta", 1, 4,
      "謝謝你……那份表格要是不見我真的完了。",
      "加退選的事我幫你備註一下，不保證，但試試看。",
      5, "Flag_HelpedTA_Ch1", true, "玩家完成助教的跑腿請求後" },
    { "ta", 2, 4,
      "等一下——",
      "（助教丟下資料夾，往你的方向走過來）",
      -15, "", false, "玩家持 `ProfessorTrapUmbrella` 在場" },
    { "shop_auntie", 0, 4,
      "哎呀同學，淋這麼濕進來喔！",
      "今天下雨我多備了一點貨，你慢慢看。",
      0, "", false, "招呼" },
    { "shop_auntie", 1, 5,
      "傘？你問對地方了。",
      "要不要？不買也沒關係，但雨還在下喔。",
      0, "", false, "玩家詢問雨傘" },
    { "shop_auntie", 2, 3,
      "聰明！醜有醜的好處啦。",
      "哈，下次加退選記得早點來搶，加油喔。",
      0, "Flag_BoughtUglyUmbrella", true, "玩家購買醜綠傘後" },
    { "victim", 0, 5,
      "……我的傘不見了。",
      "你也是被偷的嗎？",
      0, "", false, "在雨中蹲著" },
    { "victim", 1, 3,
      "真的？謝謝你……",
      "你要小心那個學長，他看起來很會說話。",
      5, "Flag_PromisedVictim", true, "我去幫你追" },
    { "victim", 2, 3,
      "（那個同學看了你一眼，又低下頭）",
      "（雨在外頭繼續打）",
      -3, "", false, "玩家無視走過" },
};

}  // namespace

TEST_CASE("Ch1 dialog golden: exact content of every Chapter1_AddDrop entry") {
    std::vector<const nccu::dialog::Entry*> ch1;
    for (const auto& e : nccu::dialog::All())
        if (e.state == SemesterState::Chapter1_AddDrop)
            ch1.push_back(&e);

    const std::size_t expectedCount =
        sizeof(kGolden) / sizeof(kGolden[0]);

    INFO("Chapter1_AddDrop entry count: actual="
         << ch1.size() << " expected=" << expectedCount);
    REQUIRE(ch1.size() == expectedCount);

    for (std::size_t i = 0; i < expectedCount; ++i) {
        const GoldenEntry&         g = kGolden[i];
        const nccu::dialog::Entry& e = *ch1[i];

        INFO("Ch1 row " << i << " expected npcId="
             << std::string(g.npcId) << " sub=" << g.subState
             << " | actual npcId=" << std::string(e.npcId)
             << " sub=" << e.subState);

        // Row anchors: a reorder/replacement must trip the snapshot.
        CHECK(e.npcId == g.npcId);
        CHECK(e.subState == g.subState);

        // Line payload: count + first/last line text verbatim.
        REQUIRE(e.lineCount == g.lineCount);
        REQUIRE(e.lineCount >= 1);
        CHECK(e.lines[0] == g.firstLine);
        CHECK(e.lines[e.lineCount - 1] == g.lastLine);

        // Side-effect payload.
        CHECK(e.karmaDelta == g.karmaDelta);
        CHECK(e.setsFlag == g.setsFlag);
        CHECK(e.flagValue == g.flagValue);
        CHECK(e.choiceLabel == g.choiceLabel);
    }
}
