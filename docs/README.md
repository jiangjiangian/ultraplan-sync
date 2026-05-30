# docs/ — 設計與內容文件

本資料夾收錄《尋傘記：政大山下篇》的設計文件與執行期載入的劇情內容。

```
docs/
├── UML/                                評分用 UML 設計文件（Mermaid 類別圖 / 狀態機 / 循序圖，已拆分為多檔）
├── Report.md                           期末專案口頭報告腳本
├── content/                            執行期由遊戲載入的劇情 Markdown（章節、結局、語音聖經）
└── README.md                           本檔
```

## UML/ — 評分用 UML 設計文件

`docs/UML/` 是繳交評分用的設計文件，內含本專案的 Mermaid 類別圖 / 狀態機 / 循序圖等
（已隨實作更新）。原本的單一 `UML.md` 已拆成一個資料夾：總覽與目錄見
[`docs/UML/README.md`](UML/README.md)，各章節（§0–§8）各自成檔以利渲染。若 GitHub
顯示「資訊過多無法渲染」，可把該圖的 Mermaid 原始碼貼到 mermaid.live 檢視。

## content/ — 執行期載入的劇情 Markdown

`docs/content/` 由遊戲在執行期讀取，內含四章劇情、結局與語音聖經（NPC 對白基準）。
所以它「不是」純文件——改它就等於改遊戲流程。請以各章 `.md` 內的旗標註記為準。
