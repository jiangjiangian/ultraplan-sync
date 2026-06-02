---
id: file:include/game/quest/Flags.h
type: header
path: include/game/quest/Flags.h
domain: game
bucket: quest
loc: 81
classes: []
sources: ["include/game/quest/Flags.h"]
---
# `Flags.h`

> **一句定位**：引擎在 `Player` 上讀寫的每一個 `Flag_*` 字串的單一事實來源（Single Source of Truth），以 `inline constexpr const char*` 常數集中宣告所有任務旗標。

## 職責

`Flags.h` 統一管理分散在各章節標頭、對話系統、SceneRouter 與 View 之間的裸 `"Flag_*"` 字面值。所有 `Player::HasFlag`、`SetFlag`、`ClearFlag` 呼叫及 Vendor 的 `setsFlag` 欄位都必須引用此處的常數（src/ 與 include/ 內不得出現裸 `"Flag_*"` 字面值，唯一例外是 `DialogLoader` 的 `"Flag_"` 前綴解析器）。

旗標依章節分組：Ch1 加退選含 8 個旗標（`kFlagClearChapter1`、`kFlagHasVictimUmbrella`、`kFlagPromisedVictim`、`kFlagFoundForm`、`kFlagHelpedTACh1`、`kFlagSuitSeniorChoiceMade`、`kFlagHelpedSenior`、`kFlagScoldedSenior`）；Ch2 期中考 9 個；Ch3 運動會 8 個；幕間市集 3 個；Ch4 期末考＋結局 13 個，共 41 個常數。

字串值是 on-the-wire 識別碼（Player 旗標表的鍵、存檔鍵、旗標檢查清單條目），一旦上線「不可」更改字串，只能改 C++ 識別字名稱。旗標的語意敘述（某章如何使用某旗標）留在各章任務標頭（`Chapter{1,2,3,4}Quest.h`），本檔僅作為裸登錄表。

## 關鍵內容（類別 / 函式 / 資料）

- **Ch1 旗標**：`kFlagHasVictimUmbrella`（找到苦主傘）、`kFlagPromisedVictim`（答應幫忙）、`kFlagHelpedSenior` / `kFlagScoldedSenior`（西裝學長選擇，影響 Ch4 生成過濾）、`kFlagFoundForm`（申請書）等。
- **Ch2 旗標**：`kFlagFoundNote1/2/3`（三頁筆記）、`kFlagBookworm`（學霸已被喚醒）、`kFlagBookwormRecovered`、`kFlagLibrarianUmbrella`/`kFlagLibrarianUmbrellaReturned`（管理員借傘）、`kFlagCh2RippledSuitSenior`/`kFlagCh2RippledTA`（Ch2 漣漪）等。
- **Ch3 旗標**：`kFlagSportsLapDone`（操場跑一圈）、`kFlagHasSausage`/`kFlagHasLoudspeaker`（物物交換鏈）、`kFlagKnowsUmbrellaLoc`（知道傘所在）、`kFlagHasProfessorTrap`（陷阱傘）等。
- **幕間旗標**：`kFlagLeaveInterlude`（離開市集）、`kFlagBoughtUglyUmbrella`（買了醜傘，Ending_C 觸發條件）、`kFlagTookCursedUmbrella`（拿了詛咒傘）。
- **Ch4/結局旗標**：`kFlagHasTrueUmbrella`、`kFlagTaFinaleChoiceMade`（助教終局選擇），以及各結局漣漪旗標（`kFlagCh4RippledSenior` 等）、告白旗標（`kFlagCh4ConfessedCursed/Ugly/True`）。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 #include）
- **被誰使用（往內）**：極廣——被 Ch1–Ch4 任務標頭、SceneRouter、DialogScreen、各 Umbrella 實體、ChapterGate、EndingGate、WorldSpawn、WorldSportsLap 以及 20 餘個測試檔案引用（如 `test_cursed_taint.cpp`、`test_ch1_quest.cpp`、`test_ending_gate.cpp` 等）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純常數定義（無程式邏輯），屬 Model 層的配置字典，為全系統旗標操作的靜態基礎；不直接出現在幀管線中，但其常數貫穿所有任務邏輯與結局判定。

## OO 概念與設計重點

本檔體現了**單一事實來源（Single Source of Truth）**原則：所有對 Player 旗標的讀寫必須流經此處定義的常數，使得任何旗標名稱的搜索與重命名皆可在一處完成。`inline constexpr const char*` 確保零執行期開銷（編譯期常數），不產生靜態初始化問題。規範中明確禁止直接字串字面值的使用，並要求旗標工具（flag check tool）掃描這些常數，形成**開放封閉**的旗標擴充機制：新增旗標只需在此加一行，且工具自動白名單化。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/Flags.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Flags.h) · [← 全檔索引](../files-index.md)
