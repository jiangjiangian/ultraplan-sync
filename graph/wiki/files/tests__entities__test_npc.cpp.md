---
id: "file:tests/entities/test_npc.cpp"
type: test
path: tests/entities/test_npc.cpp
domain: tests
bucket: entities
loc: 80
classes: []
sources: ["tests/entities/test_npc.cpp"]
---
# `test_npc.cpp`

> **一句定位**：驗證 `NPC` 對話基本行為——初始行索引與行數、`Interact` 發出當前台詞並推進索引、末行後回繞、`SetDialogLines` 重設，以及無台詞時的安全空操作。

## 職責

本檔包含 5 個 `TEST_CASE`，驗證 `NPC` 的簡單循環對話機制（非 `DialogOpener` 的多子狀態系統，而是直接以字串向量驅動的舊式對話行為）。

**預設值**：`NPC n({100,100}, {"line 0","line 1","line 2"})` 後，`CurrentLineIndex() == 0`、`DialogLineCount() == 3`、`CurrentLineText() == "line 0"`、`IsQuestGiver() == false`。

**`Interact` 發出台詞並推進**：訂閱 `ShowMessage`，呼叫 `n.Interact(nullptr)` 後 `hits == 1`、`captured == "hello"`、`n.CurrentLineIndex() == 1`；再呼叫後 `hits == 2`、`captured == "world"`、索引 == 2。

**末行後回繞**：兩行 NPC 各 Interact 一次後，索引回到 0（循環）。

**`SetDialogLines` 重設**：先讓索引推進到 1，呼叫 `SetDialogLines({"new1","new2"})` 後索引歸 0、`DialogLineCount() == 2`、`CurrentLineText() == "new1"`。

**無台詞時空操作**：空向量 NPC `Interact(nullptr)` 後無 ShowMessage 事件（`hits == 0`）、`CurrentLineIndex() == 0`，不崩潰。

## 關鍵內容（類別 / 函式 / 資料）

- `NPC::CurrentLineIndex()`：當前行索引。
- `NPC::DialogLineCount()`：台詞行數。
- `NPC::CurrentLineText()`：當前行文字。
- `NPC::Interact(Player*)`：發出 `ShowMessage` 並推進（末行後回繞）。
- `NPC::SetDialogLines(vector<string>)`：替換台詞並重設索引。
- `NPC::IsQuestGiver()`：任務給予者旗標（本檔 case 預設為 false）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/NPC.h`、`include/engine/events/EventBus.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

本檔測試的是 `NPC` 最基礎的對話循環（`Interact` 逐行推進 + 回繞），這個機制供舊式 NPC 使用（與 `DialogOpener` 的多子狀態機制並存）。回繞 case 驗證了循環不變式，無台詞 case 驗證了防禦性空操作，這兩個都是「邊界條件測試」的最佳實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_npc.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_npc.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-observer](../concepts/pat-observer.md)
