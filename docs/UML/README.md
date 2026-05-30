# 《尋傘記：政大山下篇》UML 設計文件

> 這裡是整個專案的「架構地圖」。想知道這個遊戲是怎麼搭起來的——有哪些類別、怎麼
> 分層、資料怎麼流動——從這份索引開始看就對了。

《尋傘記》是我用 C++20 + Raylib 5.5 寫的一款俯視角敘事 RPG。這份文件用 Mermaid 把
程式的骨架畫成一張張圖，對應 OOP 課程的「UML Class Diagram」評分項目，並且隨著我把
程式一路改到現在，持續更新到目前真實的實作（不是計畫，是跑得起來的版本）。

## 這份文件橫跨兩個階段

- **Assignment #5 — 2D 遊戲骨架。** 把一個原本沒有 OOP 架構的 Raylib 專案，親手搭成
  一套分層乾淨、責任分離的物件導向結構：`GameObject` 家族、MVC、工廠、狀態機。這就是
  現在遊戲跑起來的樣子——四章敘事、雨量表、業力系統、四種結局。

- **Assignment #6 — 吸血鬼倖存者（Vampire Survivors）生存模式。** 第六份作業要把這個
  骨架延伸成一個生存小遊戲：玩家在場上跑位，敵人定時生成並追著你跑，角色自動朝最近的
  敵人開火，子彈打中敵人就扣血、敵人血量歸零就消失，撐得越久越好，自己血量見底就
  Game Over。我在設計這套架構時，就先把它要用到的鉤子留好了——`IMortal` 這個角色介面
  （血量 / 受傷 / 死亡），以及把每幀邏輯拆成一條可插拔的 `ISystem` 管線（裡面已經有
  負責生成的 `SpawnSystem` 和負責清除的 `SweepSystem` 兩個 stage）。所以之後要長出
  敵人、子彈、自動攻擊，可以順著既有結構接上去，不必打掉重練。

> 換句話說，圖裡看到的 `IMortal` 與 `ISystem` 管線不是裝飾，而是我替生存模式預先鋪好
> 的路。

---

## 章節索引

文件依主題拆成多個檔案，每個檔案聚焦一層架構、各自放它該有的圖：

| 章節 | 主題 | 內容 |
| :--- | :--- | :--- |
| [§0 層次地圖](0-layer-map.md) | Layer Map | 頂層分層依賴 flowchart ＋ 層／章節對照表 |
| [§1 實體與道具繼承樹](1-entities.md) | Entities & Items | `GameObject` 繼承樹、角色介面、道具子樹、實作狀態總表 |
| [§2 狀態機與結局](2-state-machine.md) | State & Endings | `SemesterStateMachine`／`IChapterState` 類別圖、學期狀態圖（四結局） |
| [§3 MVC 核心 + ISystem 管線](3-mvc-isystem.md) | MVC + Pipeline | World／Controller／View、ISystem 模擬管線、EventBus／Vendor 周邊服務 |
| [§4 gfx 繪圖層](4-gfx.md) | Rendering | `IRenderer`、`RaylibRenderer`、材質快取、繪圖輔助 |
| [§5 autoplay 縫合層](5-harness.md) | Harness | `InputSource`／`ScriptInput`／`Time`／`Harness` 決定性重播 |
| [§6 系統互動：循序圖](6-sequence.md) | Sequence | E 互動 → BeClaimed → EventBus、每幀 ISystem 模擬管線 |
| [§7 設計模式對照](7-gof.md) | GoF | Factory／Template Method／Observer／State／Strategy 落點對照 |
| [§8 設計原則總結](8-solid.md) | SOLID | SOLID 體現位置 ＋ 架構鐵律（紅線） |

---

## 閱讀說明

**圖例**

| 標記 | 意義 |
| :--- | :--- |
| `<<Abstract>>` | 抽象基底 |
| `<<interface>>` | 角色／服務介面 |
| `<<enumeration>>` | 列舉 |
| `<<Singleton>>` | 單例 |
| `<<Factory>>` | 工廠 |
| 方法後綴 `*` | 純虛擬函式 |

**為什麼拆成好幾張圖？**
早期版本把約 40 個類別塞進「同一張」`classDiagram`，Mermaid／GitHub 會把整張圖等比
縮到版面寬度，類別一多就被縮到看不清字。所以我改成「先一張頂層層次地圖說明分層，再
每一層各畫一張小圖」（每張盡量 ≤ 約 12 個類別），這樣每張都能以可讀的尺寸渲染，需要
細看時再點圖放大即可。

**萬一某張圖 GitHub 顯示「資訊過多無法渲染」**，三個備援方法：

1. 把該圖的 ` ```mermaid ` 原始碼貼到 <https://mermaid.live> 直接檢視或匯出圖片。
2. 用 mermaid-cli 在本機預先渲染成圖片，例如單章：
   `npx -y @mermaid-js/mermaid-cli -i docs/UML/3-mvc-isystem.md -o docs/UML/3-mvc-isystem.svg`
   （或整個資料夾批次：`for f in docs/UML/*.md; do npx -y @mermaid-js/mermaid-cli -i "$f" -o "${f%.md}.svg"; done`）。
3. 用 VS Code 的 Markdown Preview Mermaid 外掛離線渲染，不受 GitHub 上限限制。
