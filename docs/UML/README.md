# 《尋傘記：政大山下篇》UML 設計文件

本檔為《尋傘記》(C++20 / Raylib 5.5，俯視角敘事 RPG) 的系統架構與 UML 分析。它聚焦
Assignment 5 評分項目 #7「UML Class Diagram」，並隨程式碼演進更新到目前的實作
（含正在為 Assignment #6「吸血鬼倖存者」生存玩法鋪路的 ISystem 模擬管線與 `IMortal`
角色介面）。所有圖以 Mermaid 撰寫；每張圖刻意拆到 ≤ 8 個類別以符合 GitHub 的
Mermaid 渲染上限。**若你的 GitHub 仍對某張圖顯示「資訊過多無法渲染」**，有三個解法：
(1) 把該圖的 ` ```mermaid ` 原始碼貼到 <https://mermaid.live> 直接檢視／匯出圖片；
(2) 在本機以 mermaid-cli 預先渲染成圖片再內嵌（逐章節檔渲染，例如）：
`npx -y @mermaid-js/mermaid-cli -i docs/UML/3-mvc-isystem.md -o docs/UML/3-mvc-isystem.svg`
（或對整個資料夾批次：`for f in docs/UML/*.md; do npx -y @mermaid-js/mermaid-cli -i "$f" -o "${f%.md}.svg"; done`）；
(3) 用 VS Code 的 Markdown Preview Mermaid 外掛（離線即可渲染，不受 GitHub 上限限制）。

> **為什麼把類別圖拆成好幾張？**
> 早期版本把約 40 個類別塞進「一張」`classDiagram`。Mermaid／GitHub 會把整張圖
> 等比縮到版面寬度，類別一多就被縮到看不清字（這就是「拉遠後全部變超小」的問題）。
> 本檔改成 **先一張頂層「層次地圖」flowchart 說明分層依賴，再每一層各一張小類別圖**
> （每張 ≤ 約 12 個類別），如此每張圖都能以可讀的尺寸渲染。GitHub 對每張 Mermaid
> 圖都提供點擊放大（zoom），所以需要看細節時再點開即可。

> **圖例**：`<<Abstract>>` 抽象基底、`<<interface>>` 角色／服務介面、`<<enumeration>>`
> 列舉、`<<Singleton>>` 單例、`<<Factory>>` 工廠。方法後 `*` 代表純虛擬。

---

## 目錄（Table of Contents）

本文件已拆分為下列章節，每章一個檔案：

| 章節 | 標題 | 內容 |
|---|---|---|
| §0 | [層次地圖（Layer Map）](0-layer-map.md) | 頂層分層依賴 flowchart + 層／章節對照表 |
| §1 | [實體與道具繼承樹（Entities & Items）](1-entities.md) | `GameObject` 繼承樹、角色介面、道具子樹、實作狀態總表 |
| §2 | [狀態機與結局（State machine & Endings）](2-state-machine.md) | `SemesterStateMachine`／`IChapterState` 類別圖、學期狀態圖（4 結局） |
| §3 | [MVC 核心 + ISystem 模擬管線](3-mvc-isystem.md) | World／Controller／View、ISystem 管線、EventBus／Vendor 周邊服務 |
| §4 | [gfx 繪圖層（IRenderer + 角色卡片）](4-gfx.md) | `IRenderer`、`RaylibRenderer`、材質快取、繪圖輔助 |
| §5 | [autoplay 縫合層（Harness）](5-harness.md) | `InputSource`／`ScriptInput`／`Time`／`Harness` 決定性重播 |
| §6 | [系統互動：循序圖（Sequence Diagrams）](6-sequence.md) | E 互動 → BeClaimed → EventBus、每幀 ISystem 模擬管線 |
| §7 | [設計模式對照（GoF）](7-gof.md) | Factory／Template Method／Observer／State／Strategy 等落點對照 |
| §8 | [設計原則總結（SOLID / 其他）](8-solid.md) | SOLID 體現位置 + 架構鐵律（紅線） |
