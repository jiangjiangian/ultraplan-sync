#ifndef NCCU_QUEST_FLAGS_H_
#define NCCU_QUEST_FLAGS_H_

/**
 * @file Flags.h
 * @brief nccu::kFlag* — 引擎在 Player 上讀寫的每一個 Flag_* 字串的單一事實來源。
 *
 * 集中管理旗標常數，取代過去散落在各章標頭、且容易在章節任務／對話開場／
 * SceneRouter／View 之間漂移的裸 "Flag_*" 字面值。
 *
 * 規範：
 *   - 每個 Player::HasFlag / SetFlag / ClearFlag 與每個 Vendor 的 setsFlag
 *     都「必須」傳入此處的常數（src/ 與 include/ 內不得出現裸 "Flag_*"
 *     字面值）。唯一例外是 DialogLoader 的 "Flag_" 前綴解析器——它解析的是
 *     對話 Markdown。
 *   - 字串值是對外（on-the-wire）識別碼（Player 旗標表的鍵、存檔鍵、
 *     旗標檢查清單條目）。一旦上線就「不可」改字串，只改 C++ 識別字。
 *   - 旗標檢查工具會掃描 src+include 內的 Flag_<word> token 自動加白名單，
 *     故新增一個引擎讀取的旗標只需這裡一行加上一處內容呼叫點。
 *   - 「某章如何使用」某旗標的語意敘述，留在該章的任務標頭
 *     （Chapter{1,2,3,4}Quest.h）；本檔僅是依章節分組以利導覽的裸登錄表。
 */

namespace nccu {

// ---- Ch1 加退選之亂 / 撞傘 -------------------------------------------------
inline constexpr const char* kFlagClearChapter1        = "Flag_ClearChapter1";
inline constexpr const char* kFlagHasVictimUmbrella    = "Flag_HasVictimUmbrella";
inline constexpr const char* kFlagPromisedVictim       = "Flag_PromisedVictim";
inline constexpr const char* kFlagFoundForm            = "Flag_FoundForm";
inline constexpr const char* kFlagHelpedTACh1          = "Flag_HelpedTA_Ch1";
inline constexpr const char* kFlagSuitSeniorChoiceMade = "Flag_SuitSeniorChoiceMade";
inline constexpr const char* kFlagHelpedSenior         = "Flag_HelpedSenior";
inline constexpr const char* kFlagScoldedSenior        = "Flag_ScoldedSenior";

// ---- Ch2 期中考週 ----------------------------------------------------------
inline constexpr const char* kFlagCh2Cleared                = "Flag_Ch2Cleared";
inline constexpr const char* kFlagFoundNote1                = "Flag_FoundNote1";
inline constexpr const char* kFlagFoundNote2                = "Flag_FoundNote2";
inline constexpr const char* kFlagFoundNote3                = "Flag_FoundNote3";
inline constexpr const char* kFlagBookworm                  = "Flag_Bookworm";
inline constexpr const char* kFlagBookwormRecovered         = "Flag_BookwormRecovered";
inline constexpr const char* kFlagMetLibrarian              = "Flag_MetLibrarian";
inline constexpr const char* kFlagLibrarianUmbrella         = "Flag_LibrarianUmbrella";
inline constexpr const char* kFlagLibrarianUmbrellaReturned = "Flag_LibrarianUmbrellaReturned";
inline constexpr const char* kFlagCh2RippledSuitSenior      = "Flag_Ch2Rippled_SuitSenior";
inline constexpr const char* kFlagCh2RippledTA              = "Flag_Ch2Rippled_TA";

// ---- Ch3 校慶運動會 --------------------------------------------------------
inline constexpr const char* kFlagCh3Cleared           = "Flag_Ch3Cleared";
inline constexpr const char* kFlagHasSausage           = "Flag_HasSausage";
inline constexpr const char* kFlagHasLoudspeaker       = "Flag_HasLoudspeaker";
inline constexpr const char* kFlagKnowsUmbrellaLoc     = "Flag_KnowsUmbrellaLoc";
inline constexpr const char* kFlagSportsLapDone        = "Flag_SportsLapDone";
inline constexpr const char* kFlagHasProfessorTrap     = "Flag_HasProfessorTrap";
inline constexpr const char* kFlagCh3RippledProfTrap   = "Flag_Ch3Rippled_ProfTrap";

// ---- Interlude / 二手市場 --------------------------------------------------
inline constexpr const char* kFlagLeaveInterlude       = "Flag_LeaveInterlude";
inline constexpr const char* kFlagBoughtUglyUmbrella   = "Flag_BoughtUglyUmbrella";
inline constexpr const char* kFlagTookCursedUmbrella   = "Flag_TookCursedUmbrella";

// ---- Ch4 期末考終焉 + Endings ----------------------------------------------
inline constexpr const char* kFlagHasTrueUmbrella       = "Flag_HasTrueUmbrella";
inline constexpr const char* kFlagTaFinaleChoiceMade    = "Flag_TaFinaleChoiceMade";
inline constexpr const char* kFlagConsoledTA            = "Flag_ConsoledTA";
inline constexpr const char* kFlagBoughtCoffeeForAuntie = "Flag_BoughtCoffeeForAuntie_Ch1";
inline constexpr const char* kFlagCh4RippledSenior      = "Flag_Ch4Rippled_Senior";
inline constexpr const char* kFlagCh4RippledBookworm    = "Flag_Ch4Rippled_Bookworm";
inline constexpr const char* kFlagCh4RippledTAHelped    = "Flag_Ch4Rippled_TAHelped";
inline constexpr const char* kFlagCh4RippledProfTrap    = "Flag_Ch4Rippled_ProfTrap";
inline constexpr const char* kFlagCh4RippledAuntie      = "Flag_Ch4Rippled_Auntie";
inline constexpr const char* kFlagCh4ConfessedCursed    = "Flag_Ch4Confessed_Cursed";
inline constexpr const char* kFlagCh4ConfessedUgly      = "Flag_Ch4Confessed_Ugly";
inline constexpr const char* kFlagCh4ConfessedTrue      = "Flag_Ch4Confessed_True";

} // namespace nccu

#endif // NCCU_QUEST_FLAGS_H_
