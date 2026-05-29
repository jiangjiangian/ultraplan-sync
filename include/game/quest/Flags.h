#ifndef NCCU_QUEST_FLAGS_H_
#define NCCU_QUEST_FLAGS_H_

// =============================================================================
// nccu::kFlag* — single source of truth for every Flag_* string the engine
// reads/writes on Player. Replaces the per-chapter-header sprinkling and the
// raw "Flag_*" literals that used to drift between chapter quests, dialog
// openers, the SceneRouter and View.
//
// House rules
//   • Every Player::HasFlag / SetFlag / ClearFlag and every Vendor
//     `setFlagOnBuy` MUST pass one of these constants (no raw "Flag_*"
//     literals in src/ or include/). `DialogLoader.cpp`'s "Flag_" prefix
//     parser is the one allowed exception — it parses dialog Markdown.
//   • The string value is the on-the-wire identifier (Player::flags_ key,
//     state.jsonl key, dialog_lint --list-flags entry). Never rename the
//     string after it ships — rename only the C++ identifier.
//   • dialog_lint.engine_flags() scans src+include for `Flag_<word>` tokens
//     and auto-whitelists them, so a new flag the engine reads is one line
//     here + a content callsite.
//   • Semantic narration of HOW a flag is used by a chapter stays in that
//     chapter's quest header (Chapter{1,2,3,4}Quest.h) — this file is the
//     bare registry, grouped by chapter for navigability.
// =============================================================================

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
