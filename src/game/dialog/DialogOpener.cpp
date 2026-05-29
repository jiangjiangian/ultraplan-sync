#include "game/dialog/DialogOpener.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/entities/Player.h"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

namespace {

// 在此狀態下哪些 NPC 會呈現分支選單。第一章的選項開場集合為 {suit_senior、victim、
// shop_auntie}。西裝學長／苦主帶有真正的漣漪 A/B。福利社阿姨的選單提供 詢問雨傘 /
// 購買醜綠傘 / 請阿姨喝一杯熱咖啡：其中 (c) 購買醜綠傘 選項「不」在 DialogChoice 上設下
// 任何旗標（已驗證：該選項只帶 `// karma +0` 而無 `Flag_X = true`，故解析後的 setsFlag
// 為 ""）；它是一筆「真正」的購買，金錢 + 持有型醜傘在確認時由 GameController
//（TryBuyAuntieUglyUmbrella）套用，而非在此處，且刻意「不」設下 Flag_BoughtUglyUmbrella
// ——那個結局 C 鎖屬於第四章集英樓攤販。其餘所有人維持只有台詞。
bool UsesChoiceOpener(std::string_view npcId, SemesterState s) {
    if (s != SemesterState::Chapter1_AddDrop) return false;
    return npcId == "suit_senior" || npcId == "victim" ||
           npcId == "shop_auntie";
}

// =============================================================================
// 各 (state, npcId) 的開場子狀態解析器——把原本 ResolveOpenerSubState 裡 200 行的
// switch 改寫成表格形式。每個解析器只回答一個問題：「在此玩家的旗標／業力下，應顯示此
// NPC 章節條目中的哪個子狀態作為只有台詞／回顧的開場？」。回傳 0 永遠代表「OpenNpcDialog
// 的 (a) 基準開場」；呼叫端會在 sub==0 時短路回到「選單 vs 只有台詞」的路徑，故解析器
// 回傳 0 不會誤把玩家丟進不存在的回顧。
//
// 路由僅為只有台詞的回顧。條目散文所帶的任何業力／旗標都走另一條路徑
//（TryApplyChNRipple／任務鉤子），「不」由開場套用——被路由的 (b)/(c)/(d) 開場台詞唯一
// 會寫入玩家狀態之處，是 OpenNpcDialog 中限定於第一章的獎勵回顧防護（申請書／承諾獎勵）。
// 下方的分派查找對 ≤ 約 20 列為 O(N)——極快。
// =============================================================================

// ---- Ch1 加退選之亂 ---------------------------------------------------------

// 助教申請書跑腿回顧——一旦給予（Flag_HelpedTA_Ch1），或玩家已拿到申請書
//（Flag_FoundForm），她的開場便轉為 (b) 致謝／閒置。否則為 (a)「請幫我撿一張表」的請求。
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
// 第二章漣漪路由。只有台詞的回顧；每個第二章一次性的業力由 TryApplyCh2Ripple 落地，
// 而非在此。學霸／苦主／阿姨的反應節拍從前是內聯的 `*（若 Flag_X）*` 行、會被解析器
// 悄悄丟棄。它們如今在章節內容中改寫為真正受旗標閘控的「獨立」子狀態，並在此路由，
// 使該行真正顯示。

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
// 物物交換鏈三節點：一旦此 NPC 的環節完成，便路由到 (b)「交易完成 / 情報揭露」，否則
//（a）。(a) 的子區塊「玩家尚未帶X / 玩家帶著X」是被解析器壓平的條件行（已知省略，與
// 第二章 (c)/(c-fail) 及學霸 (a) 詛咒台詞同類）——接受現狀，未改章節內容前無法路由。
// (b) 是只有台詞的回顧；+3/+3/+5 由 TryAdvanceCh3Trade 落地，而非開場的一次性套用。
//
// 第三章漣漪路由（真正受旗標閘控的「獨立」子狀態；章節內容的業力是條列文件而非引用區塊，
// 故此處無一由解析器套用——這些路由純為敘事回顧，唯一的程式碼業力是經 TryApplyCh3Ripple
// 的教授陷阱 -10）。

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

// HelpedSenior → (b) 物物交換鏈提示（省一步）。否則為 (a)，其 Helped=true/false 的條件行
// 被解析器壓平（已知省略）；ScoldedSenior「不觸發對話」同樣無法表達為子狀態——接受現狀，
// 須改章節內容才能處理。
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
// 第四章高潮漣漪路由（只有台詞的回顧；章節內容的業力是條列文件而非引用區塊，故無一由
// 解析器套用——真正要緊的第四章業力由 TryApplyCh4Ripple 與助教 (d) 體諒 選項落地）。
// 助教 (d) 不在此路由——它是程式碼建構的選項開場。

// 章節內容規定：!HelpedSenior / ScoldedSenior → 學長不出場。但抑制生成是已知省略
//（名冊仍保留他）；故降級為 (a) 假笑面具。否則業力把劇情分流：>70 崩潰坦白 (b)、
// <30 翻臉 (c)，介於 30..70 的中段維持 (a)。
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

// (b)/(c) 互斥，HelpedTA_Ch1 優先；(c) 的 -15 仍另經 TryApplyCh4Ripple 落地。
// 預設為 (a) 巡考慌張。(d) 體諒 是程式碼建構的那個選項。
int Ch4Ta(const Player& p) {
    if (p.HasFlag(kFlagHelpedTACh1))      return 1;  // (b)
    if (p.HasFlag(kFlagHasProfessorTrap)) return 2;  // (c)
    return 0;                                          // (a)
}

// (b) 淡漠：承諾過但傘到第四章仍未在手。否則為 (a) 釋懷（已歸還，或第一章未承諾）。
// HasUmbrella() 由 TrueUmbrella::BeClaimed 設定。
int Ch4Victim(const Player& p) {
    if (p.HasFlag(kFlagPromisedVictim) && !p.HasUmbrella()) return 1;
    return 0;
}

// 企劃指名、但引擎從未讀取的第一章→第四章阿姨漣漪（Flag_BoughtCoffeeForAuntie_Ch1）。
// 第一章請過咖啡的情分 → (a) 直接情報（subState 0，主動說助教往哪跑）；否則 → (d)
// 間接情報（subState 3，只說「那個常來的助教很趕」）。+3 的 (a) 路線回扣經
// TryApplyCh4Ripple 走另一條路徑（章節內容的業力是條列文件而非引用區塊——此處無一由
// 解析器套用）。(b)/(c) 推銷綠傘/拒買 維持結局 C 集英樓攤販的風味節拍。
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

// 每個依子狀態路由的 (state, npcId) 各一列。任何未列出的 (state, npcId) 都落到子狀態 0
//（OpenNpcDialog 的 (a) 基準開場）。同一狀態內的順序僅為可讀性——查找是線性的、鍵皆唯一。
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
    dlg.Open({});  // 無相符 -> 維持未啟用
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
    if (opener == nullptr) { dlg.Open({}); return; }  // 無開場 -> 未啟用

    std::vector<std::string> openerLines = opener->lines;

    if (!UsesChoiceOpener(npcId, state)) {
        dlg.Open(std::move(openerLines));  // 只有台詞
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // 此開場決策中「真正屬於分支」的最高子狀態（相對於那些僅由 ResolveOpenerSubState
    // 路由、只有台詞之路徑才會到達的決策後回顧）。第一章苦主現在有一個 (d) 重逢致謝 回顧
    //（善有善報：在傘歸還「之後」才顯示，經 ResolveOpenerSubState==3 路由）；它「不得」在
    // (a) 請求處作為過早的選單選項出現，那裡真正的分支只有 (b) 承諾 / (c) 無視。把上限設在
    // (c)=2 使苦主的請求選單維持只有這兩項，與加入回顧子狀態之前完全一致；其餘所有選項開場
    // NPC 則保留每個 ≥1 的子狀態（上限為含括）。
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
    // 硬閘控第一章主線：西裝學長「不得」在玩家連苦主都還沒見到之前就開啟他的選項選單。
    // 主線是 苦主 → 學長 → (傘出現) → 苦主；學長的分支（要回傘 / 善意提醒 / 接受取傘）
    // 唯有在苦主已請玩家去追回他被拿走的傘（Flag_PromisedVictim）之後才合理。若順序錯亂
    // 就先找學長，他會把這個陌生人打發走、指引其回去綜合院館——一個劇情上連貫的轉向，而
    // 「非」攸關漣漪的選單（否則玩家可能在本章第一個節拍前就確認學長選項、甚至取得道德
    // 雨傘）。只有台詞、不設任何旗標，故玩家仍能去見苦主再回來。道德雨傘同樣以
    // Flag_PromisedVictim 為閘（TransparentUmbrella::QuestGateOpen），故此處絕不會使
    // 結局 B 路線走進死路。
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

    // 硬閘控第二章主線：學霸「不得」在玩家見到圖書館管理員之前就可接洽。主線是
    // 管理員 → 學霸(喚醒) → 撿筆記 → 學霸(換回)；管理員的 (a) 台詞會指引玩家前往
    // 羅馬廣場那座學霸癱倒的雕像。若順序錯亂（在 Flag_MetLibrarian 之前）就先找學霸，
    // 癱倒的學霸不會回應，並有提示把玩家導向櫃台。只有台詞、不設任何旗標——玩家可以去見
    // 管理員再回來。一旦見過她，下方正常的 (a)/(c)/(d) 路由便接手（TryRescueBookworm
    // 也會在按 E 互動路徑上推一把；這是對話側的鏡像）。一旦喚醒便略過（Flag_Bookworm
    // 意味著已見過管理員——否則無法喚醒他），故喚醒後重新對話絕不會錯誤轉向。
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

    // 西裝學長是攸關漣漪的選項開場。一旦玩家對他確認過某個選項（suit_senior 選項確認時，
    // GameController 會設下 Flag_SuitSeniorChoiceMade），重新對話便「不得」再次呈現分支
    // 選單——否則玩家可能堆疊互斥的漣漪旗標（先選 (d) Flag_HelpedSenior，再重新對話選 (c)
    // Flag_ScoldedSenior）。回顧 = 子狀態 0 的開場台詞，只有台詞（無選單、不重套業力／
    // 旗標）。shop_auntie / victim 維持可重新進入（影響低；其重新進入無害）。
    if (state == SemesterState::Chapter1_AddDrop && npcId == "suit_senior" &&
        player.HasFlag(kFlagSuitSeniorChoiceMade)) {
        OpenNpcDialogSub(dlg, npcId, state, 0);   // 開場台詞，「無」選項
        dlg.SetNpcContext(std::string(npcId));
        return;
    }

    // 助教 (d) 結算——閘控結局 A 的道德選擇。章節內容中 (d) 的「體諒」/「質問」是被解析器
    // 壓平的粗體子區塊，故此選項採「程式碼建構」（酬載為程式碼、由內容檔負責風味；走另一條
    // 路徑的先例）。開場台詞是被路由的 (a)/(b)/(c) 反應；選單則是結算。與西裝學長一樣為
    // 一次性：Flag_TaFinaleChoiceMade（確認時由 GameController 設下）→ 只有台詞的回顧、
    // 絕不再次呈現（不重複業力／不翻轉道德選擇）。
    if (state == SemesterState::Chapter4_Finals && npcId == "ta") {
        const int taSub = ResolveOpenerSubState(npcId, state, player);
        std::vector<std::string> openerLines;
        for (const auto& e : nccu::dialog::Entries(npcId, state))
            if (e.subState == taSub) { openerLines = e.lines; break; }

        if (player.HasFlag(kFlagTaFinaleChoiceMade)) {
            dlg.Open(std::move(openerLines));     // 回顧，「無」選單
            dlg.SetNpcContext(std::string(npcId));
            return;
        }
        std::vector<DialogChoice> taChoices;
        // 體諒 是溫柔的終局。它設下 Flag_ConsoledTA（結局 A 的道德鑰匙），「且」——因為
        // 善意會讓助教把「你的」傘塞回你手裡——GameController 在此分支也授予
        // Flag_HasTrueUmbrella（通往結局 A 的溫柔取回路線，與隱藏的第四章雨傘並行）。
        //「拿回你的傘」這一節拍在「此處」說出，使歸還呈現在畫面上、而非隱含。DialogChoice
        // 帶一個旗標（Flag_ConsoledTA）；HasTrueUmbrella 的授予在確認時由 GameController
        // 接線。
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
        // 不做承諾的退出。附加在「最後」，使 體諒/質問 維持索引 0/1（test_ch4_finale
        // 固定之）。零 karmaDelta + 空 setsFlag 且「無」後續台詞 → Advance() 會立刻
        // Close()（攤販不買的形狀），且 GameController 對此標籤略過 Flag_TaFinaleChoiceMade，
        // 故終局選單「不」被消費——玩家可以走開、稍後再接洽助教決定（無軟鎖、不會誤入結局）。
        // 助教則在背景繼續巡考。
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
