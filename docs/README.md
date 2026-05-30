# docs/ — 設計文件與執行期劇情

本資料夾只含兩類內容：

## UML.md — 評分用 UML 設計文件

`docs/UML.md` 是繳交評分用的設計文件，內含本專案的 Mermaid 類別圖 / 狀態機 / 循序圖等
UML 設計說明，描述領域分層（app / engine / game / ui）與核心子系統的結構與互動。

## content/ — 執行期載入的劇情 Markdown

`docs/content/` 放的是**作者撰寫、遊戲在執行期載入並解析**的劇情文字（非建置產物）。
`game/dialog` 的 `DialogLoader` / `DialogSource` 在執行期（相對工作目錄）讀取這些 `*.md`，
解析成 NPC 對白資料模型；編輯後可透過 `Reload()` 熱重載，無需重新編譯。

收錄檔案：

- `chapter1.md` ～ `chapter4.md` — 四個章節的 NPC 對白與分支
- `interlude_market.md` — 市集過場（interlude）對白
- `ending_a.md` / `ending_b.md` / `ending_c.md` — 結局劇情文字（**狀態機共四結局** Ending_A/B/C/D，
  其中 Ending_D 為「無名冊」變體，沿用既有結局文本與場景配置）
- `voice_bible.md` — 角色口吻 / 語氣設定（voice bible），供撰稿一致性參考
