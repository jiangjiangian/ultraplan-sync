---
id: file:include/ui/View.h
type: header
path: include/ui/View.h
domain: ui
bucket: 
loc: 126
classes: [View, BuildingSprite, DrawRef, DecorationSprite]
sources: ["include/ui/View.h"]
---
# `View.h`

> **一句定位**：MVC 的 View——持有具體 RaylibRenderer、跟隨相機與世界地圖貼圖，把唯讀的 World 模型轉成每幀像素；`Draw(const World&)` 是遊戲狀態唯一被轉成像素的入口。

## 職責

`View` 是渲染層的最頂層封裝，對應 MVC 的「V」角。它持有具體的 `RaylibRenderer`（不透過介面抽象，因為它是組裝根唯一一個明確依賴 raylib 的地方），以及 `Camera2D`（玩家追蹤相機）、世界底圖貼圖、建築貼圖集合、環境裝飾條集合等視覺資源。

`Draw(const World& world)` 是唯一的公開方法，是「遊戲狀態唯一被轉成像素」的地方。它以 `const World&` 唯讀存取模型，**絕不修改 World**（MVC 紅線），並按序呼叫五個私有 helper：
1. `UpdateChapterCardTransition(st)`：偵測 FSM 邊界，武裝「傘又掉了」/「找到傘了」書檔字卡。每幀都跑（在結局提前返回之前），讓字卡狀態保持最新。
2. `RenderEnding(world, st)`：當 FSM 處於結局狀態時接管整幀，清為黑底並呼叫 `DrawEndingCard`，呼叫端隨即返回。
3. `RenderWorld(world, st)`：在 `CameraScope` 內繪製世界底圖、跑道地貼、建築/裝飾/物件的深度排序（by Y）、任務給予者「!」浮標、過場出口標記。
4. `RenderHud(world, st)`：螢幕座標，繪製圈速環（右上）、左上狀態面板（業力/金幣/雨量/章節）、雨壓 vignette、任務目標面板。
5. `RenderOverlays(world)`：HUD 訊息（雙通道）、對話框、物品欄、M 選單提示、暫停選單、說明疊層、章節書檔字卡（最後畫，疊在最上層）。

View 維護三個純 View 側狀態（不進 World）：`endingAlpha_`（結局淡入進度）、`decorationClock_`（裝飾動畫時鐘）、`interludeMarkerPhase_`（過場出口標記掃動相位）以及 `lastSemester_`（用於 FSM 邊界偵測）與 `chapterCard_`（章節書檔字卡狀態機）。

## 關鍵內容（類別 / 函式 / 資料）

- `View(windowWidth, windowHeight)`：建構子，載入世界底圖與建築貼圖集。
- `Draw(const World& world)`：唯一公開方法，按序呼叫五個 helper。
- `BuildingSprite`（私有 struct）：已擺放的建築記錄——`texIndex`、`dest`（`Rect`）、`baseY`（深度排序 Y）、`flipX/flipY`。
- `DrawKind`（私有 `enum class`）：`Object`、`Building`、`Decoration`——每幀繪製順序清單的項目類型。
- `DrawRef`（私有 struct）：深度排序清單的一筆——`y`（排序鍵）、`kind`、`obj`（Object 時有效）、`index`（Building/Decoration 時的索引）。裝飾物併入同一次排序，使環境裝飾（如石像）能對 NPC/建築正確 walk-behind。
- `DecorationSprite`（私有 struct）：一條已載入的環境裝飾條——`defIndex`（指向 `kDecorations`）、`texture`（已載入貼圖，move-only）。
- 私有 helper：`UpdateChapterCardTransition`、`RenderEnding`、`RenderWorld`、`RenderHud`、`RenderOverlays`。
- 私有狀態：`renderer_`（`RaylibRenderer`）、`camera_`（`Camera2D`）、`worldmap_`（底圖貼圖）、`buildingTextures_`（建築貼圖向量）、`buildings_`（BuildingSprite 向量）、`decorations_`（DecorationSprite 向量）、`drawOrder_`（每幀暫存的深度排序清單）、`endingAlpha_`、`decorationClock_`、`interludeMarkerPhase_`、`lastSemester_`（`std::optional<SemesterState>`）、`chapterCard_`（`ChapterCardState`）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/RaylibRenderer.h`（具體渲染器）；`engine/render/Camera2D.h`、`Texture.h`；`engine/math/Vec2.h`、`Rect.h`；`game/state/SemesterState.h`（FSM 邊界偵測）；`ui/ChapterCard.h`（書檔字卡狀態機與渲染）；`<optional>`、`<vector>`、`<cstddef>`。
- **被誰使用（往內）**：`include/app/scenes/GameplayScene.h`（GameplayScene 持有 `View view_` 成員）；`src/ui/View.cpp`（`Draw` 與五個 helper 的實作）。
- **繼承 / 實作 / 體現**：`realizes_concepts: MVC 核心 (arch-mvc)`。
- **每幀管線 / MVC 角色**：MVC 的 View。每幀在 GameController 完成所有系統更新後，`GameplayScene` 呼叫 `view_.Draw(world_)`，把最新的 World 狀態轉成像素。

## OO 概念與設計重點

`View` 是 [MVC](../concepts/arch-mvc.md) 的「V」角的完整體現：`Draw(const World&)` 是其唯一公開方法，強制所有渲染以唯讀模型為輸入；任何「View 試圖寫入 World」的嘗試都無法通過編譯。深度排序（`DrawRef` 按 Y 排序）使遊戲呈現正確的 Z 順序（walk-behind），是俯視角 RPG 必備的渲染技術。View 側狀態（`endingAlpha_`、`decorationClock_`、`chapterCard_` 等）與模型狀態完全分離——這些僅影響視覺呈現，不影響遊戲邏輯，自動跑流程的存檔因此逐位元不變。私有 helper 的拆分把一個可能龐大的 `Draw` 方法分成五個關注點清晰的片段，是可讀性設計的良好實踐。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/View.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/View.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
