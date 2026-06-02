---
id: index
type: index
title: 《尋傘記》知識圖譜 · wiki 索引
---

# 《尋傘記》知識圖譜 · wiki 索引

這是整個 repo 的「知識地圖」。它把 **每一個版控檔案**（共 500 個，一個都不漏）、
**OO 概念 / 設計模式**、以及檔案之間的 **相依關係**（`#include`、類別繼承、模式落點、
領域依賴）整理成一張 [**可互動的知識圖譜**](https://jiangjiangian.github.io/ultraplan-sync/) 與這套互相連結的 wiki 頁面。

> 設計取自兩個參考專案：互動圖譜的形式學自李宏毅老師頻道地圖（Cytoscape 知識圖譜），
> 而「LLM 增量維護的持久 wiki、typed 頁面、`[[wikilink]]` 交叉參照、來源可追溯」
> 的概念取自 [`llm_wiki`](https://github.com/nashsu/llm_wiki)。

## 從哪裡開始

| 想做的事 | 去哪 |
|---|---|
| **互動探索整張圖**（搜尋 / 篩選 / 預設檢視） | [🕸 互動知識圖譜 `index.html`](https://jiangjiangian.github.io/ultraplan-sync/) |
| 讀一段架構總覽（這專案怎麼搭起來的） | [📐 架構總覽 overview.md](overview.md) |
| 想知道「圖怎麼建的、節點/邊是什麼意思」 | [🧬 圖譜結構 schema.md](schema.md) |
| 查**某一個檔案** | [🗂 全檔索引 files-index.md](files-index.md)（每檔一列、含深連結） |
| 機器讀取（你 / agent） | [`../data/graph.json`](../data/graph.json) · [`../data/files.json`](../data/files.json) |

## OO 概念 / 設計模式

### 設計模式（GoF）
- [Factory Method](concepts/pat-factory.md) — `GameObjectFactory` 由列舉生 12 種物件
- [Template Method](concepts/pat-template.md) — `BeClaimed` / `Consume` 純虛擬骨架
- [Observer](concepts/pat-observer.md) — `EventBus` 事件解耦
- [State](concepts/pat-state.md) — `SemesterStateMachine` 五章 + 四結局
- [Strategy / Pipeline](concepts/pat-strategy.md) — `ISystem` 五段模擬管線
- [Singleton](concepts/pat-singleton.md) — `EventBus::Instance()`
- [Command / Table](concepts/pat-command.md) — 資料化的 `QuestHookTable`

### OO 原則 / 技法
- [角色介面（ISP）](concepts/oo-isp-roles.md) — `Roles.h` 四個小介面
- [CRTP static mixin](concepts/oo-crtp.md) — `WithRoles<Derived,Base>` 編譯期能力查詢
- [RAII / 記憶體安全](concepts/oo-raii.md) — `unique_ptr` + mark-then-sweep

### 架構元件
- [MVC 核心](concepts/arch-mvc.md) — World / View / GameController
- [ISystem 模擬管線](concepts/arch-isystem.md) — 每幀 model 推進
- [DIP：IRenderer](concepts/arch-dip-renderer.md) — raylib 關在介面後
- [決定性 autoplay（Harness）](concepts/arch-harness.md) — 可重播

## 領域分層（檔案）

相依方向 **app → game / ui → engine**（engine 不反向相依）：

- [app · 組裝根](domains/app.md)（14）
- [engine · 引擎層](domains/engine.md)（35）
- [game · 遊戲邏輯 Model](domains/game.md)（139）
- [ui · 視圖層 View](domains/ui.md)（36）

> `tests`（114）、`docs`（21）、`tools`（9）、`resources`（123）、`root`（9）等其餘檔案
> 全數收在 [全檔索引](files-index.md) 與互動圖譜中。

## 權威來源（單一真相）

本 wiki 是**衍生視圖**；架構的單一真相是設計文件，請勿在此改變設計意圖：

- [`docs/UML/`](../../docs/UML/README.md) — 類別圖 / 狀態機 / 循序圖 / GoF / SOLID
- [`遊戲企劃與敘事架構.md`](../../遊戲企劃與敘事架構.md) — 章節 / karma / 經濟 / 結局
- [`docs/Report.md`](../../docs/Report.md) — 口頭報告腳本

---
本頁與所有 `concepts/`、`domains/`、`files-index.md` 由 `graph/build_graph.py` 產生並可重建。
