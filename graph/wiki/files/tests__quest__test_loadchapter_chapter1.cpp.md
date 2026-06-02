---
id: file:tests/quest/test_loadchapter_chapter1.cpp
type: test
path: tests/quest/test_loadchapter_chapter1.cpp
domain: tests
bucket: quest
loc: 147
classes: []
sources: ["tests/quest/test_loadchapter_chapter1.cpp"]
---
# `test_loadchapter_chapter1.cpp`

> **一句定位**：對照網測試——以 `LoadChapter` 解析器讀取真實的 `chapter1.md`，逐一驗證每個 NPC 各子狀態的台詞數、首行文字、`karmaDelta`、`setsFlag`/`flagValue` 與 `choiceLabel` 推導規則。

## 職責

此測試檔是「parser 契約釘住」的典型用例：所有預期值都是手動從 `docs/content/chapter1.md` 謄錄的，形成一份永久對照表。只要解析器行為有任何靜默回歸（如旗標解析順序變更、choiceLabel 截取邏輯改變），此測試就會失敗。

重點驗證項目：
- **NPC 段落過濾**：`chapter.npcs` 必須有 `西裝學長`、`學霸`、`苦主`，而 `場景旁白`、`章節結尾分支提示` 必須被排除。
- **西裝學長 4 個子狀態**：
  - (a) sub-state 0：5 行，首行「欸，加退選也沒搶到嗎？」，karma 0，無旗標。
  - (b) sub-state 1：`karmaDelta==3`，`setsFlag==kFlagScoldedSenior`，`flagValue==true`，`choiceLabel=="理性指出他品行不該，要回雨傘"`。
  - (c) sub-state 2：5 行，`karmaDelta==0`，`setsFlag==kFlagScoldedSenior`，`flagValue==false`，`choiceLabel=="接受，取傘後交給學長"`。
  - (d) sub-state 3：5 行，首行「……怪怪的？你什麼意思？」，`karmaDelta==5`，`setsFlag==kFlagHelpedSenior`，`flagValue==true`，`choiceLabel=="點破傘的疑點，轉而提供正規協助"`。
- **學霸 3 個子狀態**：sub-state 0 無 karma 無旗標；sub-state 1 karma==3 無旗標。
- **苦主 choiceLabel 截取規則**：(b) 標題中的「」內的文字優先（`"我去幫你追"`），(a) 去除結尾（…）（`"在雨中蹲著"`）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::dialog::LoadChapter(path)`：解析 markdown 並回傳章節結構體（含 `npcs` map）。
- `Find(subs, subState)`：從 `std::vector<SubEntry>` 中找對應 `subState` 的 helper。
- `ContentPath(name)`：組合 `TEST_CONTENT_DIR + "/" + name` 的路徑 helper。
- `TEST_CASE("LoadChapter：chapter1 實際內容與 codegen 一致")`：單一大 TEST_CASE 包含所有斷言（NPC 段落過濾 + 西裝學長四分支 + 學霸 + choiceLabel 截取規則）。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`（旗標常數比對）、`DialogLoader.h`（`LoadChapter`）。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試對話資料載入管線（`DialogLoader`），這是 `OpenNpcDialog` 在初始化時依賴的底層解析層。

## OO 概念與設計重點

「手謄預期值」是此測試的核心特點：它不依賴任何中間層——直接從實際內容檔的格式規範導出。這使它同時是「解析器回歸測試」和「內容檔格式契約文檔」。若日後修改 choiceLabel 截取規則（如將「」改為`（）`），此測試會精確指出哪個子狀態的哪個欄位變了。`kFlagScoldedSenior` 在 (b) 為 true、(c) 為 false 的對稱性是一個設計上值得注意的細節：同一個旗標名，但語義方向相反。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_loadchapter_chapter1.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_loadchapter_chapter1.cpp) · [← 全檔索引](../files-index.md)
