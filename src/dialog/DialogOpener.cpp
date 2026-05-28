#include "dialog/DialogOpener.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "entities/Player.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

namespace {

// Scaffold: which NPCs present the branch menu in this state. The Ch1
// choice-opener set is now {suit_senior, victim, shop_auntie}.
// 西裝學長 / 苦主 carry the genuine ripple A/B. 福利社阿姨's menu offers
// 詢問雨傘 / 購買醜綠傘 / 請阿姨喝一杯熱咖啡: the (c) 購買醜綠傘 choice does
// NOT set any flag on the DialogChoice (verified: chapter1.md (c) carries
// `// karma +0` and no `Flag_X = true`, so the parsed entry's setsFlag is "");
// B3 makes it a REAL buy whose money + held ugly umbrella are applied in
// GameController on confirm (TryBuyAuntieUglyUmbrella), NOT here, and it
// deliberately does NOT set Flag_BoughtUglyUmbrella — that Ending-C lock is
// the Ch4 集英樓 Vendor's (src/EndingGate.cpp). Everyone else stays line-only.
bool UsesChoiceOpener(std::string_view npcId, SemesterState s) {
    if (s != SemesterState::Chapter1_AddDrop) return false;
    return npcId == "suit_senior" || npcId == "victim" ||
           npcId == "shop_auntie";
}

// =============================================================================
// Per-(state, npcId) opener-substate resolvers — the table form of what used
// to be a 200-line switch in ResolveOpenerSubState. Each resolver answers ONE
// question: "given this player's flags/karma, which subState of this NPC's
// chapter*.md entries should the line-only / recap opener show?". Returning 0
// always means "the (a) baseline opener of OpenNpcDialog"; the caller short-
// circuits sub==0 back to the menu-vs-line-only path so a resolver returning
// 0 cannot accidentally drop a player into a non-existent recap.
//
// Routing is line-only recap. Any karma/flag the entry's prose carries is
// path-b (TryApplyChNRipple / quest hooks), NOT applied by the opener — the
// only place a routed (b)/(c)/(d) opener line writes player state is the
// Ch1-scoped 1b-3 reward-recap guard in OpenNpcDialog (申請書 / 承諾 reward).
// The dispatch lookup below is O(N) over ≤ ~20 rows — trivially fast.
// =============================================================================

// ---- Ch1 加退選之亂 ---------------------------------------------------------

// 助教 申請書 errand recap — once granted (Flag_HelpedTA_Ch1) OR after the
// player has the 申請書 in hand (Flag_FoundForm), her opener moves to (b)
// thank-you/idle. Otherwise (a) "請幫我撿一張表" plea.
int Ch1Ta(const Player& p) {
    if (p.HasFlag(kFlagHelpedTACh1)) return 1;
    if (p.HasFlag(kFlagFoundForm))   return 1;
    return 0;
}

// 苦主 善有善報 routing:
//   授予真傘後 (Flag_HasTrueUmbrella) → (d) 重逢致謝 recap [3]
//     (授予 + 清關已由 TryReturnVictimUmbrella 完成；(d) 純對白，loader
//      無旗標，once-apply 不重套——victim recap 先例)。
//   已承諾 (Flag_PromisedVictim)      → (b) 承諾後續 [1]
//     (「先幫他找傘」提示由 TryReturnVictimUmbrella 的 ShowMessage 呈現；
//      (b) 是 line-only recap)。
//   否則                              → (a) 在雨中初遇 [0]。
int Ch1Victim(const Player& p) {
    if (p.HasFlag(kFlagHasTrueUmbrella)) return 3;
    if (p.HasFlag(kFlagPromisedVictim))  return 1;
    return 0;
}

// ---- Ch2 期中考週 ----------------------------------------------------------
// S5c-3 ripple routing. Line-only recap; the once-per-Ch2 karma is landed by
// TryApplyCh2Ripple, not here. B5: 學霸/苦主/阿姨's reactive beats USED to be
// inline `*（若 Flag_X）*` lines the parser silently drops. They are now
// re-authored as genuine flag-gated SEPARATE subStates in chapter2.md and
// routed here so the line actually displays.

int Ch2SuitSenior(const Player& p) {
    if (p.HasFlag(kFlagHelpedSenior))  return 1;  // (b) +3
    if (p.HasFlag(kFlagScoldedSenior)) return 2;  // (c) -3
    return 0;                                       // (a) 路過
}

// Precedence (chapter2.md L225 「取代 (a)/(b) 段」 > L211 「取代 (a) 段」):
// ProfessorTrap outranks HelpedTA.
int Ch2Ta(const Player& p) {
    if (p.HasFlag(kFlagHasProfessorTrap)) return 2;  // (c) -10
    if (p.HasFlag(kFlagHelpedTACh1))      return 1;  // (b)
    return 0;                                          // (a)
}

// (a) 詢問線索：指向羅馬廣場雕像下的學霸 → (b) 喚醒學霸後的確認 recap. 純資訊
// 節點（chapter2.md：不給 karma、無 flag），故 OpenNpcDialog 的 once-apply 區
// 對 (b) 是 no-op，純 line-only recap（ta/victim recap 先例）。
int Ch2Librarian(const Player& p) {
    return p.HasFlag(kFlagBookworm) ? 1 : 0;
}

// 兩階段任務機（TryRescueBookworm，鑰匙 Flag_Bookworm）的對話映射：
//   Recovered          → (d) 致謝 recap [3]（line-only；+5 已在 rescue 套，
//     (d) blockquote 無 flag 註解，once-apply 不重套）。
//   喚醒後未換回傘      → (c) 已喚醒、等撿筆記 recap [2]（喚醒互動本身的提示
//     由 TryRescueBookworm 的 ShowMessage 呈現；(c) 子段現已重寫為純對白、可
//     路由）。
//   喚醒前持詛咒傘      → (b) 詛咒冷反應變體 [1]（chapter2.md (b)「取代 (a)」；
//     學霸直覺感應到那把寫著別人名字的傘——CursedUmbrella.cpp 註記的 Ch2 冷反應）。
//   否則                → (a) 常態遊魂初遇 [0]。
int Ch2Bookworm(const Player& p) {
    if (p.HasFlag(kFlagBookwormRecovered))  return 3;
    if (p.HasFlag(kFlagBookworm))           return 2;  // (c)
    if (p.HasFlag(kFlagTookCursedUmbrella)) return 1;  // (b)
    return 0;                                            // (a)
}

// B5: 買過集英樓螢光綠醜傘 → (b) 阿姨認出你（取代 (a)）。否則 (a) 考試週招呼。
// (c) 狼狽版仍由 rainMeter 機制觸發，非 opener 路由（沿用原狀，已知省略）。
int Ch2ShopAuntie(const Player& p) {
    return p.HasFlag(kFlagBoughtUglyUmbrella) ? 1 : 0;
}

// B5: Ch1 承諾過 → (c) 她記得承諾（取代 (a)）；買過醜傘 → (d) 她注意到那把
// 醜傘（取代 (b)）。兩旗標互不相斥，承諾的情感回扣優先於醜傘的辨識橋段。
// 否則 (a) 常態路過。
int Ch2Victim(const Player& p) {
    if (p.HasFlag(kFlagPromisedVictim))     return 2;  // (c)
    if (p.HasFlag(kFlagBoughtUglyUmbrella)) return 3;  // (d)
    return 0;                                            // (a)
}

// ---- Ch3 校慶運動會 --------------------------------------------------------
// 物物交換鏈三節點：route to (b)「交易完成 / 情報揭露」once this NPC's link
// has been done, else (a). The (a) sub-blocks「玩家尚未帶X / 玩家帶著X」are
// parser-flattened conditional lines (KNOWN OMISSION, same class as S5c-2
// (c)/(c-fail) and 學霸 (a) cursed line) — accepted, not routable without a
// chapter3.md edit. (b) is line-only recap; the +3/+3/+5 is landed by
// TryAdvanceCh3Trade, not the opener's once-apply.
//
// S5d-3 ripple routing (genuine flag-gated SEPARATE subStates; chapter3.md
// karma is `- \`// karma\`` bullet-doc, NOT a `>` blockquote, so nothing here
// is parser-applied — these routes are pure narrative recap, the only code-
// karma is ProfTrap -10 via TryApplyCh3Ripple).

// (a) Ch2 救回分支（Flag_BookwormRecovered=true）/ (b) 未救回分支（=false）.
int Ch3Bookworm(const Player& p) {
    return p.HasFlag(kFlagBookwormRecovered) ? 0 : 1;
}

// (c) Flag_HelpedTA_Ch1 分支；否則 (a) 登記桌初次接觸.
int Ch3Ta(const Player& p) {
    return p.HasFlag(kFlagHelpedTACh1) ? 2 : 0;
}

// (a) Flag_PromisedVictim=true 且傘尚未歸還 / (b) =false 或 Ch1 無承諾.
int Ch3Victim(const Player& p) {
    return p.HasFlag(kFlagPromisedVictim) ? 0 : 1;
}

// HelpedSenior → (b) 物物交換鏈提示（省一步）. Else (a), whose Helped=true/false
// condition lines are parser-flattened (KNOWN OMISSION); ScoldedSenior「不觸發
// 對話」 is likewise not expressible as a subState — accepted, would need a
// chapter3.md edit.
int Ch3SuitSenior(const Player& p) {
    return p.HasFlag(kFlagHelpedSenior) ? 1 : 0;
}

int Ch3VendorSausage(const Player& p) {
    if (p.HasFlag(kFlagHasSausage) ||
        p.HasFlag(kFlagHasLoudspeaker) ||
        p.HasFlag(kFlagKnowsUmbrellaLoc)) return 1;
    return 0;
}

int Ch3Loudspeaker(const Player& p) {
    if (p.HasFlag(kFlagHasLoudspeaker) ||
        p.HasFlag(kFlagKnowsUmbrellaLoc)) return 1;
    return 0;
}

int Ch3SeniorC(const Player& p) {
    return p.HasFlag(kFlagKnowsUmbrellaLoc) ? 1 : 0;
}

// ---- Ch4 期末考終焉 --------------------------------------------------------
// S5e-2a peak ripple routing (line-only recap; chapter4.md karma is `- \`//
// karma\`` bullet-doc, NOT a `>` blockquote, so nothing is parser-applied —
// the Ch4 karma that matters is landed by TryApplyCh4Ripple (S5e-2c) and the
// 助教 (d) 體諒 choice (S5e-2d)). 助教 (d) is NOT routed here — it is a
// code-constructed choice-opener (S5e-2d).

// chapter4.md L88: !HelpedSenior / ScoldedSenior → 學長 不出場. Spawn-
// suppression is a KNOWN OMISSION (roster keeps him); degrade to (a) 假笑面具.
// Otherwise karma splits the arc: >70 崩潰坦白 (b), <30 翻臉 (c), the 30..70
// middle stays (a).
int Ch4SuitSenior(const Player& p) {
    if (!p.HasFlag(kFlagHelpedSenior)) return 0;
    if (p.GetKarma() > 70) return 1;   // (b)
    if (p.GetKarma() < 30) return 2;   // (c)
    return 0;                           // (a)
}

// (b) Ch2 救他 callback / (c) 未救.
int Ch4Bookworm(const Player& p) {
    return p.HasFlag(kFlagBookwormRecovered) ? 1 : 2;
}

// (b)/(c) 互斥, HelpedTA_Ch1 優先 (chapter4.md L235); the (c) -15 still lands
// separately via TryApplyCh4Ripple. (a) 巡考慌張 default. (d) 體諒 is the
// S5e-2d choice.
int Ch4Ta(const Player& p) {
    if (p.HasFlag(kFlagHelpedTACh1))      return 1;  // (b)
    if (p.HasFlag(kFlagHasProfessorTrap)) return 2;  // (c)
    return 0;                                          // (a)
}

// (b) 淡漠：承諾過但傘到 Ch4 仍未在手 (chapter4.md L338). 否則 (a) 釋懷
// （已歸還 or Ch1 無承諾, L325). HasUmbrella() is set by TrueUmbrella::beClaimed.
int Ch4Victim(const Player& p) {
    if (p.HasFlag(kFlagPromisedVictim) && !p.HasUmbrella()) return 1;
    return 0;
}

// B3: the Ch1→Ch4 阿姨 ripple the GDD names (Flag_BoughtCoffeeForAuntie_Ch1)
// but engine never read. Ch1 請過咖啡情分 → (a) 直接情報（subState 0，主動說
// 助教往哪跑）；否則 → (d) 間接情報（subState 3，只說「那個常來的助教很趕」）.
// The +3 (a)-route callback is path-b via TryApplyCh4Ripple (chapter4.md karma
// is bullet-doc, not a `>` blockquote — nothing here is parser-applied). (b)/(c)
// 推銷綠傘/拒買 stay the Ending-C 集英樓 Vendor flavour beats.
int Ch4ShopAuntie(const Player& p) {
    return p.HasFlag(kFlagBoughtCoffeeForAuntie) ? 0 : 3;
}

// ---- Dispatch table --------------------------------------------------------

using OpenerResolver = int (*)(const Player&);

struct DispatchEntry {
    SemesterState state;
    std::string_view npcId;
    OpenerResolver resolver;
};

// One row per (state, npcId) that routes by subState. Any (state, npcId) not
// listed falls through to subState 0 (the (a) baseline opener of
// OpenNpcDialog). Order within a state is for readability only — lookup is
// linear and the keys are unique.
constexpr std::array<DispatchEntry, 20> kDispatch{{
    {SemesterState::Chapter1_AddDrop,   "ta",                Ch1Ta},
    {SemesterState::Chapter1_AddDrop,   "victim",            Ch1Victim},

    {SemesterState::Chapter2_Midterms,  "suit_senior",       Ch2SuitSenior},
    {SemesterState::Chapter2_Midterms,  "ta",                Ch2Ta},
    {SemesterState::Chapter2_Midterms,  "librarian",         Ch2Librarian},
    {SemesterState::Chapter2_Midterms,  "bookworm",          Ch2Bookworm},
    {SemesterState::Chapter2_Midterms,  "shop_auntie",       Ch2ShopAuntie},
    {SemesterState::Chapter2_Midterms,  "victim",            Ch2Victim},

    {SemesterState::Chapter3_SportsDay, "bookworm",          Ch3Bookworm},
    {SemesterState::Chapter3_SportsDay, "ta",                Ch3Ta},
    {SemesterState::Chapter3_SportsDay, "victim",            Ch3Victim},
    {SemesterState::Chapter3_SportsDay, "suit_senior",       Ch3SuitSenior},
    {SemesterState::Chapter3_SportsDay, "vendor_sausage_a",  Ch3VendorSausage},
    {SemesterState::Chapter3_SportsDay, "loudspeaker_b",     Ch3Loudspeaker},
    {SemesterState::Chapter3_SportsDay, "senior_c",          Ch3SeniorC},

    {SemesterState::Chapter4_Finals,    "suit_senior",       Ch4SuitSenior},
    {SemesterState::Chapter4_Finals,    "bookworm",          Ch4Bookworm},
    {SemesterState::Chapter4_Finals,    "ta",                Ch4Ta},
    {SemesterState::Chapter4_Finals,    "victim",            Ch4Victim},
    {SemesterState::Chapter4_Finals,    "shop_auntie",       Ch4ShopAuntie},
}};

}  // namespace

void OpenNpcDialogSub(DialogState& dlg, std::string_view npcId,
                      SemesterState state, int subState) {
    for (const auto& sub : nccu::dialog::Entries(npcId, state)) {
        if (sub.subState == subState) {
            dlg.Open(sub.lines);
            return;
        }
    }
    dlg.Open({});  // no match -> stays inactive
}

void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state) {
    const auto& subs = nccu::dialog::Entries(npcId, state);

    const nccu::dialog::SubEntry* opener = nullptr;
    for (const auto& sub : subs) {
        if (sub.subState == 0) {
            opener = &sub;
            break;
        }
    }
    if (opener == nullptr) { dlg.Open({}); return; }  // no opener -> inactive

    std::vector<std::string> openerLines = opener->lines;

    if (!UsesChoiceOpener(npcId, state)) {
        dlg.Open(std::move(openerLines));  // line-only
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // Highest substate that is a genuine BRANCH of this opener's decision
    // (vs a post-decision recap reached only by ResolveOpenerSubState's
    // routed line-only path). The Ch1 victim now has a (d) 重逢致謝 recap
    // (善有善報: shown AFTER the umbrella is returned, routed via
    // ResolveOpenerSubState==3); it must NOT appear as a premature menu
    // choice at the (a) plea, where the only real branches are (b) 承諾 /
    // (c) 無視. Capping at (c)=2 keeps the victim's plea menu to those two,
    // exactly as it was before the recap substate was added; every other
    // choice-opener NPC keeps every ≥1 substate (cap stays inclusive).
    const int maxChoiceSub =
        (state == SemesterState::Chapter1_AddDrop && npcId == "victim")
            ? 2 : 99;
    std::vector<DialogChoice> choices;
    for (const auto& sub : subs) {
        if (sub.subState >= 1 && sub.subState <= maxChoiceSub) {
            choices.push_back(DialogChoice{
                sub.choiceLabel, sub.karmaDelta,
                sub.setsFlag, sub.flagValue, sub.lines});
        }
    }
    dlg.Open(std::move(openerLines), std::move(choices));
    dlg.SetNpcContext(std::string(npcId));
}

int ResolveOpenerSubState(std::string_view npcId, SemesterState state,
                          const Player& player) {
    for (const auto& e : kDispatch) {
        if (e.state == state && e.npcId == npcId) return e.resolver(player);
    }
    return 0;
}

void OpenNpcDialog(DialogState& dlg, Player& player,
                   std::string_view npcId, SemesterState state) {
    // A1 (hard-gate the Ch1 spine): the 西裝學長 must NOT open his choice
    // menu before the player has even met the 苦主. The spine is
    // 苦主 → 學長 → (傘出現) → 苦主; the 學長's branch (要回傘 / 善意提醒 /
    // 接受取傘) only makes sense once the 苦主 has asked the player to chase
    // his stolen umbrella (Flag_PromisedVictim). Talked-to out of order, the
    // 學長 brushes the stranger off and points them back toward 綜合院館 —
    // a coherent in-fiction redirect, NOT the ripple-critical menu (which
    // would otherwise let a player commit the 學長 choice and even claim a
    // morality umbrella before the chapter's first beat). Line-only, sets
    // nothing, so the player can still go meet the 苦主 and return. The
    // morality umbrellas stay gated on Flag_PromisedVictim too (Transparent
    // Umbrella::QuestGateOpen), so this never dead-ends the Ending-B path.
    if (state == SemesterState::Chapter1_AddDrop && npcId == "suit_senior" &&
        !player.HasFlag(kFlagPromisedVictim) &&
        !player.HasFlag(kFlagSuitSeniorChoiceMade)) {
        dlg.Open({"（西裝學長上下打量你一眼，不太耐煩）",
                  "「你哪位？我趕時間，剛面試完還有事。」",
                  "「加退選沒搶到？那是你的事，別來煩我。」",
                  "（他別過頭——看來他並不想跟一個陌生人多談）"});
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // A2 (hard-gate the Ch2 spine): the 學霸 must NOT be approachable before
    // the player has met the 圖書館管理員. The spine is 管理員 → 學霸(喚醒) →
    // 撿筆記 → 學霸(換回); the librarian's (a) line is what points the player to
    // the 羅馬廣場 statue where the 學霸 is slumped. Talked-to out of order
    // (before Flag_MetLibrarian), the slumped 學霸 does not respond and a cue
    // redirects the player to the 櫃台. Line-only, sets nothing — the player
    // can go meet the librarian and return. Once she is met, the normal
    // (a)/(c)/(d) routing below takes over (TryRescueBookworm also nudges on
    // the E-interact path; this is the dialog-side mirror). Skipped once woken
    // (Flag_Bookworm implies the librarian was met — you cannot wake him
    // otherwise) so a re-talk after waking never wrongly redirects.
    if (state == SemesterState::Chapter2_Midterms && npcId == "bookworm" &&
        !player.HasFlag(kFlagMetLibrarian) &&
        !player.HasFlag(kFlagBookworm)) {
        dlg.Open({"（他整個人趴在雕像基座上，睡得不省人事）",
                  "（你叫了幾聲，他毫無反應，只是含糊地翻了個身）",
                  "（看來得先弄清楚這人是誰——"
                  "去圖書館櫃台問問那位管理員吧。）"});
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // C.3(b): 西裝學長 is the ripple-critical choice-opener. Once the
    // player has committed a choice with him (Flag_SuitSeniorChoiceMade
    // is set by GameController when a suit_senior choice is confirmed),
    // re-talking must NOT re-present the branch menu — otherwise the
    // player could stack mutually-exclusive ripple flags (pick (d)
    // Flag_HelpedSenior, then re-talk and pick (c) Flag_ScoldedSenior).
    // Recap = the subState-0 opener lines, line-only (no menu, no
    // re-applied karma/flag). shop_auntie / victim stay re-enterable
    // (low impact; after C.1 a shop_auntie re-entry is harmless).
    if (state == SemesterState::Chapter1_AddDrop && npcId == "suit_senior" &&
        player.HasFlag(kFlagSuitSeniorChoiceMade)) {
        OpenNpcDialogSub(dlg, npcId, state, 0);   // opener lines, NO choices
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // S5e-2d: 助教 (d) 結算 — the moral choice that gates Ending A.
    // chapter4.md (d)'s 「體諒」/「質問」are parser-flattened bold
    // sub-blocks (same class as S5c-2 (c)/(c-fail)), so the choice is
    // CODE-CONSTRUCTED (no 2nd content nod, not a fragile line-split —
    // payload is code, the .md owns flavour; path-b precedent). The
    // opener lines are the routed (a)/(b)/(c) reaction; the menu is the
    // 結算. One-shot like C.3(b): Flag_TaFinaleChoiceMade (set by
    // GameController on confirm) → line-only recap, never re-presented
    // (no double karma / no flipping the moral choice).
    if (state == SemesterState::Chapter4_Finals && npcId == "ta") {
        const int taSub = ResolveOpenerSubState(npcId, state, player);
        std::vector<std::string> openerLines;
        for (const auto& e : nccu::dialog::Entries(npcId, state))
            if (e.subState == taSub) { openerLines = e.lines; break; }

        if (player.HasFlag(kFlagTaFinaleChoiceMade)) {
            dlg.Open(std::move(openerLines));     // recap, NO menu
            dlg.SetNpcContext(std::string(npcId));
            return;
        }
        std::vector<DialogChoice> taChoices;
        // S5e-2d / T4: 體諒 is the gentle finale. It sets Flag_ConsoledTA
        // (Ending A's moral key) AND — because being kind makes the 助教
        // press YOUR umbrella back into your hands — GameController also
        // grants Flag_HasTrueUmbrella on this branch (the gentle reclaim
        // route to Ending A, parallel to the hidden Ch4 umbrella). The
        // "拿回你的傘" beat is spoken HERE so the return is on-screen, not
        // implicit. DialogChoice carries one flag (Flag_ConsoledTA); the
        // HasTrueUmbrella grant is wired in GameController on confirm.
        taChoices.push_back(DialogChoice{
            "體諒助教的辛勞", 15, kFlagConsoledTA, true,
            {"（你接過那把傘，順手替他把懷裡的考卷扶正）",
             "「辛苦了，先去睡一下吧。」",
             "（助教愣了一下）「……你不追究？」",
             "（他低聲）「你這學期……做了蠻多的。我有看到。」",
             "（你握緊了傘柄——這把手感紮實的傘，終於回到你手上了）",
             "（轉身前，他塞給你一顆糖）歐趴糖，之後再找你。"}});
        taChoices.push_back(DialogChoice{
            "質問／強硬索回", -5, std::string{}, false,
            {"（你伸手，語氣不軟）",
             "「規定就是規定，我配合。」",
             "「傘是你的，我還你。」",
             "（他不再說話，繼續巡考）"}});
        // 1c: the no-commit exit. Appended LAST so 體諒/質問 keep indices
        // 0/1 (test_ch4_finale pins them). Zero karmaDelta + empty setsFlag
        // and NO nextLines → Advance() Close()s immediately (vendor-decline
        // shape), and GameController skips Flag_TaFinaleChoiceMade for this
        // label, so the finale menu is NOT consumed — the player can walk
        // off and re-approach 助教 to decide later (no soft-lock, no
        // accidental Ending). The 助教 keeps巡考 in the background.
        taChoices.push_back(DialogChoice{
            kDialogExitLabel, 0, std::string{}, false, {}});
        dlg.Open(std::move(openerLines), std::move(taChoices));
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    const int sub = ResolveOpenerSubState(npcId, state, player);
    if (sub == 0) { OpenNpcDialog(dlg, npcId, state); return; }  // 1b-2 path

    const nccu::dialog::SubEntry* hit = nullptr;
    for (const auto& e : nccu::dialog::Entries(npcId, state))
        if (e.subState == sub) {
            hit = &e; break;
        }
    if (hit == nullptr) { OpenNpcDialog(dlg, npcId, state); return; }  // fallback

    dlg.Open(hit->lines);  // line-only consequence / recap
    dlg.SetNpcContext(std::string(npcId));

    // Apply the entry's own side-effects ONCE — the Ch1 1b-3 reward
    // recap (ta 申請書 / victim 承諾): only when it sets a true flag the
    // player doesn't have yet. Scoped to Chapter1_AddDrop on purpose:
    // this is the Ch1 reward-recap mechanism. Ch2/3/4 ripple karma is
    // path-b (TryRescueBookworm / TryApplyChNRipple / trade hooks), and
    // a ripple subState's parsed setsFlag is an ARTIFACT of the
    // chapter*.md prose, not an intended reward — e.g. chapter4.md 助教
    // (c) L235's precedence note carries `Flag_HelpedTA_Ch1 = true`, so
    // an unscoped auto-apply would spuriously grant HelpedTA_Ch1 to a
    // player routed to (c). Ch2/Ch3 only avoided this by content luck
    // (empty setsFlag); the guard makes safety structural.
    const std::string flag(hit->setsFlag);
    if (state == SemesterState::Chapter1_AddDrop &&
        !flag.empty() && hit->flagValue && !player.HasFlag(flag)) {
        player.AddKarma(hit->karmaDelta);
        player.SetFlag(flag);
    }
}

} // namespace nccu
