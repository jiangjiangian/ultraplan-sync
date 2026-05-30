# docs/ — 設計文件與執行期劇情

本資料夾只含兩類內容：

```text
.
├── UML.md                              評分用 UML 設計（Mermaid 類別圖 / 狀態機 / 循序圖）
│
└── content/                            執行期載入並解析的劇情 Markdown（可 Reload 熱重載）
    ├── chapter1.md                         第一章 NPC 對白與分支
    ├── chapter2.md                         第二章 NPC 對白與分支
    ├── chapter3.md                         第三章 NPC 對白與分支
    ├── chapter4.md                         第四章 NPC 對白與分支
    ├── interlude_market.md                 市集過場（interlude）對白
    ├── ending_a.md                         結局 A 劇情文字
    ├── ending_b.md                         結局 B 劇情文字
    ├── ending_c.md                         結局 C 劇情文字（Ending_D 為無名冊變體，沿用既有文本）
    └── voice_bible.md                      角色口吻 / 語氣設定（撰稿一致性參考）
```

## UML.md — 評分用 UML 設計文件

`docs/UML.md` 是繳交評分用的設計文件，內含本專案的 Mermaid 類別圖 / 狀態機 / 循序圖等
UML 設計說明，描述領域分層（app / engine / game / ui）與核心子系統的結構與互動。

## content/ — 執行期載入的劇情 Markdown

`docs/content/` 放的是**作者撰寫、遊戲在執行期載入並解析**的劇情文字（非建置產物）。
`game/dialog` 的 `DialogLoader` / `DialogSource` 在執行期（相對工作目錄）讀取這些 `*.md`，
解析成 NPC 對白資料模型；編輯後可透過 `Reload()` 熱重載，無需重新編譯。

收錄檔案見上方目錄樹：`chapter1.md` ～ `chapter4.md` 為四章 NPC 對白與分支，
`interlude_market.md` 為市集過場對白，`ending_a.md` / `ending_b.md` / `ending_c.md` 為結局劇情文字
（**狀態機共四結局** Ending_A/B/C/D，其中 Ending_D 為「無名冊」變體，沿用既有結局文本與場景配置），
`voice_bible.md` 為角色口吻 / 語氣設定（voice bible），供撰稿一致性參考。
