# `graph/` — 《尋傘記》互動知識圖譜

把整個 repo 的 **每一個版控檔案**、**OO 概念 / 設計模式**、以及檔案之間的
**相依關係**（`#include`、類別繼承、模式落點、領域依賴）整理成一張可互動的知識圖譜
＋一套互相連結的 wiki，讓人與 AI agent 都能快速看懂這個專案、並能精準連到任一檔案。

> 形式參考李宏毅老師頻道地圖（Cytoscape 互動圖譜），知識組織概念參考
> [`llm_wiki`](https://github.com/nashsu/llm_wiki)（typed 頁面、`[[wikilink]]`、來源可追溯）。

## 怎麼看

- **線上公開網站（推薦）**：<https://jiangjiangian.github.io/ultraplan-sync/>
  —— 由 `.github/workflows/pages.yml` 自動部署（push 到 `main` 觸發；`configure-pages`
  的 `enablement` 會自動開啟 Pages，免手動設定）。任何人都能直接開，不需下載或 clone。
- **本機**：直接用瀏覽器開 `graph/index.html`（已是單一自足檔，CSS/JS/資料全內嵌，
  不需架站、不需網路）。
- **純讀文字**：從 [`wiki/index.md`](wiki/index.md) 進入（GitHub 會渲染）。

### 操作
拖曳平移、滾輪縮放、點節點看細節（含 `#include`、被誰 include、繼承、原始碼與 wiki 連結）、
雙擊檔案節點開 GitHub 原始碼。左側可切換**預設檢視**（架構骨架 / 繼承樹 / 設計模式 / 完整全圖）、
排版、依種類/領域/邊篩選、搜尋。每個節點都有深連結，例如
`index.html#node=file:include/game/entities/Player.h`。

## 重建（資料與 repo 同步）

```bash
python3 graph/build_graph.py          # 重建 data/*.json + wiki/{files-index,concepts,domains}
python3 graph/build_graph.py --check  # 只驗證「節點數 == 檔案數」（完整性），不寫檔
```

純標準函式庫、無需 LLM、無外部相依。

## 結構

```
graph/
├── index.html              互動圖譜（Cytoscape.js）
├── build_graph.py          萃取器：掃 git 檔案 → 圖資料 + wiki（可重建）
├── assets/                 style.css · app.js（前端邏輯）
├── vendor/cytoscape.min.js Cytoscape（vendored，離線可用）
├── data/
│   ├── graph.json          nodes + edges + meta（機器可讀）
│   ├── graph-data.js        window.GRAPH_DATA（給 file:// 用）
│   └── files.json          扁平的每檔索引
└── wiki/                   llm_wiki 風格知識頁
    ├── index.md            入口 / 目錄
    ├── overview.md         架構總覽
    ├── schema.md           節點 / 邊 / 產生方式
    ├── files-index.md      全檔索引（每檔一列，自動產生）
    ├── concepts/*.md       14 個 OO 概念 / 設計模式頁（自動產生）
    └── domains/*.md        4 個領域頁（自動產生）
```

## 資料一覽

500 檔案 · 556 節點（500 檔 + 9 領域 + 33 bucket + 14 概念）· 2092 邊
（`includes` / `inherits` / `realizes` / `in-bucket` / `in-domain` / `depends` / `tests`）。

> 這是**衍生視圖**。架構的單一真相在 [`docs/UML/`](../docs/UML/README.md) 與
> [`遊戲企劃與敘事架構.md`](../遊戲企劃與敘事架構.md)；改設計請改那邊，再 `build_graph.py` 重建。
