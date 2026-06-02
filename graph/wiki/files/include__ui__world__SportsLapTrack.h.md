---
id: "file:include/ui/world/SportsLapTrack.h"
type: header
path: include/ui/world/SportsLapTrack.h
domain: ui
bucket: world
loc: 29
classes: []
sources: ["include/ui/world/SportsLapTrack.h"]
---
# `SportsLapTrack.h`

> **一句定位**：宣告繪製第三章操場校慶跑道地貼（動態消除虛線輪廓）的純渲染自由函式。

## 職責

此標頭宣告 `DrawSportsLapTrack` 自由函式，在第三章（Chapter3_SportsDay）的操場繞圈玩法進行期間，渲染一個虛線「體育場」輪廓（上下直道由左右半圓相接），並隨玩家圈速完成程度動態消除已通過的線段（「走完動態消除」）。

繪製必須在 CameraScope「之內」（世界座標）；在繪製順序掃描「之前」執行，確保綜合院館（覆蓋操場東緣）與跑者疊在跑道線條之上（分層順序：地圖 → 線條 → 綜院）。

反應式純函式：以 `World::SportsLapActive()` 與 `World::SportsLapProgress()` 讀取狀態，未進行圈速時提前返回；唯讀 `World`，絕不修改模型。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawSportsLapTrack(IRenderer& r, const World& world)` — 在 CameraScope 內繪製操場跑道虛線輪廓，根據 `SportsLapProgress()` 消除已走過的部分；`SportsLapActive()` 為假時提前返回。

## 相依與在架構中的位置
- **#include（往外）**：僅依賴前向宣告 `World` 與 `IRenderer`（無實際 include 解析）
- **被誰使用（往內）**：`src/ui/View.cpp`、`src/ui/world/SportsLapTrack.cpp`
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；於 `RenderWorld` 的早期（掃描之前）呼叫，在 CameraScope 內，純渲染，僅第三章 SportsDay 狀態下有視覺效果

## OO 概念與設計重點

從 View 主體抽出的單職責渲染函式，體現 SRP 與 [DIP](../concepts/arch-dip-renderer.md)。以純函式反應式讀取 World 狀態（`SportsLapActive` / `SportsLapProgress`），確保 View 不持有圈速計算邏輯（邏輯在 `SpawnSystem::Run` 呼叫的 `World::UpdateSportsLap`）。繪製分層順序（地圖 → 線條 → 綜院）的需求由呼叫端確保；此函式本身不涉及排序。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/world/SportsLapTrack.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/world/SportsLapTrack.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
