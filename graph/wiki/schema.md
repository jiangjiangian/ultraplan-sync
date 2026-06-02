---
id: schema
type: schema
title: 圖譜結構（節點 / 邊 / 產生方式）
---

# 圖譜結構：節點、邊、怎麼建的

這頁說明知識圖譜的「資料模型」——有哪些節點、哪些邊、以及 `graph/build_graph.py`
如何**純靜態地**從 repo 萃取它們。對應 `llm_wiki` 的 `schema.md`（規則層）。

## 節點種類（`data.kind`）

| kind | 是什麼 | 形狀 / 顏色 | 數量 |
|---|---|---|---|
| `file` | 一個版控檔案（**每一個**都在，500 個一個不漏） | 圓點，顏色＝領域 | 500 |
| `domain` | 領域分層 app / engine / game / ui / tests / docs / tools / resources / root | 大圓角矩形 | 9 |
| `bucket` | 領域底下的次層資料夾（如 `game/entities`） | 圓角矩形 | 33 |
| `pattern` | GoF 設計模式 | ★ 星形 | 7 |
| `principle` | OO 原則 / 技法（ISP / CRTP / RAII） | ◆ 菱形 | 3 |
| `architecture` | 架構元件（MVC / ISystem / IRenderer / Harness） | ⬡ 六邊形 | 4 |

檔案節點帶的中繼資料：`path`、`domain`、`bucket`、`ntype`
（header/source/test/doc/content/asset/tool/build/config）、`loc`、`classes`（解析出的類別名）、
`github`（原始碼連結）。

## 邊種類（`data.etype`）

| etype | 方向 | 怎麼來的 |
|---|---|---|
| `includes` | 檔案 → 檔案 | 解析 `#include "..."`，以 include 根（`include/`、`include/game/`）解析成實際檔案 |
| `inherits` | 子類別檔 → 基底類別檔 | 解析 `class X : public Base`，把 base 素名對應回宣告它的檔案 |
| `realizes` | 概念 → 落點檔案 | 概念地圖以**類別名**連結（對路徑差異穩健） |
| `in-bucket` | 檔案 → bucket | 由路徑推導 |
| `in-domain` | bucket / 檔案 → domain | 由路徑推導 |
| `depends` | domain → domain | 領域依賴方向（app→game/ui/engine、game→engine、ui→engine/game） |
| `tests` | 測試檔 → 同名受測 bucket | `tests/<bucket>` 對應 `game|engine|ui|app/<bucket>` |

## 完整性保證

`build_graph.py` 以 `git ls-files -z` 取得**每一個**版控檔案，逐一建成 `file` 節點，
最後硬性斷言：

```python
assert counts["file_nodes"] == counts["files"]   # 一個檔案都不能漏
```

可用 `python3 graph/build_graph.py --check` 只跑這個檢查。

## 怎麼重建

```bash
python3 graph/build_graph.py        # 重建 data/*.json + wiki/files-index.md + wiki/concepts|domains/*
python3 graph/build_graph.py --check  # 只驗證完整性，不寫檔
```

純標準函式庫、**無需 LLM、無外部相依套件**（與 `tools/docs_graph.py` 同精神）。
產物：

- `data/graph.json` — Cytoscape elements（nodes + edges）+ meta
- `data/graph-data.js` — 同一份資料包成 `window.GRAPH_DATA`（讓 `file://` 直接開也能用）
- `data/files.json` — 扁平的每檔索引
- `wiki/files-index.md`、`wiki/concepts/*.md`、`wiki/domains/*.md`

## 想要「每檔一頁」？

目前每個檔案都是**圖中的一個節點 + 全檔索引中的一列 + `files.json` 的一筆**（完整、可深連結）。
若你要 `llm_wiki` 那種「每個原始碼檔各一頁 markdown」，概念地圖已備好；擴充
`build_graph.py` 增加 `--file-pages` 即可逐檔輸出（預設關閉以免灌爆繳交分支）。

---
[← wiki 索引](index.md) · [📐 架構總覽](overview.md) · [🕸 互動圖譜](../index.html)
