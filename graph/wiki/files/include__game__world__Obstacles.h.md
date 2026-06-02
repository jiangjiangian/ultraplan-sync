---
id: file:include/game/world/Obstacles.h
type: header
path: include/game/world/Obstacles.h
domain: game
bucket: world
loc: 27
classes: []
sources: ["include/game/world/Obstacles.h"]
---
# `Obstacles.h`

> **一句定位**：渲染期「不額外繪製 sprite」的建築白名單——列出像素已烘焙進底圖的開放地面，供 View 跳過 sprite 合成。

## 職責

本標頭只定義一個 `inline constexpr std::array<std::string_view, 2> kBuildingCollisionSkip`，但它扮演 View 與 TexturePreload 跳過特定建築 sprite 的唯一判斷依據。

目前名單包含「操場」與「羅馬廣場」兩項。這兩個建築的像素形狀已直接烘焙進 `worldmap_base.png` 的底圖層，若再另行繪製 3D sprite 疊圖會導致重複渲染。View 在遍歷 `buildings::kAll` 時對名單內的建築跳過 sprite 繪製；TexturePreload 同樣以此名單避免預熱這些不需要的貼圖。

值得注意的是，名稱雖為「CollisionSkip」，但注解已明確說明：**實體碰撞不再由此處控制**，碰撞改由像素級可走遮罩 PNG（`resources/assets/maps/collision_mask.png`）載入為 `CollisionMask` 管理。此名稱是歷史命名遺留，反映的是「sprite 略過」而非碰撞略過。

## 關鍵內容（類別 / 函式 / 資料）

- `obstacles::kBuildingCollisionSkip`（`inline constexpr std::array<std::string_view, 2>`）：像素已烘焙進 `worldmap_base.png` 的建築名稱白名單，當前為 `{"操場", "羅馬廣場"}`。View 與 TexturePreload 以 `std::find` 對此陣列查詢，命中者跳過 sprite 繪製或預熱。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<array>`、`<string_view>`）。
- **被誰使用（往內）**：`include/game/world/TexturePreload.h`（`PreloadGameTextures` 遍歷 `kAll` 時用此名單略過貼圖預熱）；`src/ui/View.cpp`（`RenderWorld` 繪製建築 sprite 時跳過名單內的建築）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純資料常數，無管線行為。被 View（渲染層）讀取，不影響模型或控制器。

## OO 概念與設計重點

本檔是典型的 **最小共用常數標頭**——兩個消費端（View.cpp 與 TexturePreload.h）若各自硬編「操場」與「羅馬廣場」，任一處遺漏更新都會造成漂移（預熱了不需要的貼圖、或畫出了不應出現的 sprite）。集中在此確保「略過清單」只有一份定義，符合 **DRY（Don't Repeat Yourself）** 原則。其命名雖帶「Collision」卻已說明為純渲染略過，是技術債的一個例子——若日後重命名為 `kBakedException` 或類似名稱會更準確。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/Obstacles.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Obstacles.h) · [← 全檔索引](../files-index.md)
