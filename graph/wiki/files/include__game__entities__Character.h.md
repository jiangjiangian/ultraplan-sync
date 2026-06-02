---
id: file:include/game/entities/Character.h
type: header
path: include/game/entities/Character.h
domain: game
bucket: entities
loc: 65
classes: [Character]
sources: ["include/game/entities/Character.h"]
---
# `Character.h`

> **一句定位**：Player 與 NPC 的可移動角色共同基底，在 `GameObject` 之上封裝「方向正規化 × 速度 × 時間」的幀率無關位移語意。

## 職責

`Character` 繼承自 `GameObject`，新增了三個移動相關成員：`speed_`（像素／秒）、`direction_`（最近一次已正規化的移動方向）與 `currentFrame_`（動畫影格索引）。它集中了整個可移動物件層的兩個核心操作：

`Move(dir, deltaTime)` 將傳入的方向向量先呼叫 `dir.Normalized()` 正規化，再以 `speed_ × deltaTime` 推進 `position_`，同時將 `hitBox_` 的 xy 同步到 `position_`，保持碰撞盒與世界座標一致。斜向移動因此不會比正向快 √2 倍。`direction_` 記錄已正規化的向量，供子類別（如 NPC 朝向決定 Pipoya 列）讀取。

`SetPosition(p)` 允許外部修正（例如 `ClampToWorld` 的世界邊界夾制，或碰撞解析後的位移回推）直接覆寫座標，並同步更新 `hitBox_`，確保碰撞盒永不與位置脫鉤。

本類別不涉及輸入、渲染或任何 raylib 符號，屬於純資料中間層。

## 關鍵內容（類別 / 函式 / 資料）

- **`Character(position, hitBox, speed)`**：建構子，初始化 `speed_`、`direction_({0,0})`、`currentFrame_(0)`，並轉交 `GameObject(position, hitBox)`。
- **`void Move(dir, deltaTime)`**：正規化方向後以幀率無關公式推進座標並同步碰撞盒與 `direction_`。
- **`void SetPosition(p) noexcept`**：直接覆寫世界座標並同步碰撞盒，供外部邊界修正使用。
- **`float speed_`**（protected）：移動速度（像素／秒）。
- **`Vec2 direction_`**（protected）：最近一次的已正規化移動朝向。
- **`int currentFrame_`**（protected）：動畫影格索引（子類別驅動）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/core/GameObject.h`（提供位置、碰撞盒基底）、`include/engine/math/Vec2.h`（方向與位置運算）。
- **被誰使用（往內）**：`include/game/entities/NPC.h`、`include/game/entities/Player.h`（兩者皆繼承 `Character`）。
- **繼承 / 實作 / 體現**：繼承自 `GameObject`；被 `Player` 與 `NPC` 繼承。
- **每幀管線 / MVC 角色**：屬於 Model 層（`World` 擁有的 `GameObject` 繼承樹）。`Move` 由 Movement System 每幀呼叫；`SetPosition` 由 Collision System 碰撞解析後修正。本類別本身不在管線中持有行為，是資料結構。

## OO 概念與設計重點

`Character` 是繼承樹中的「可移動中間層」，在 `GameObject`（位置 + 碰撞盒）與具體角色（`Player`、`NPC`）之間插入速度語意，符合 DRY 原則。`Move` 內的方向正規化是必要的邊界條件處理：未正規化的對角輸入會讓速度達到 √2 倍，影響遊戲平衡。`SetPosition` 的存在是對外部修正的開放性設計——碰撞系統與邊界夾制無須知道 `Character` 的內部速度，直接注入修正後座標即可。

整體設計維持 Model 乾淨：不引入 raylib、不讀輸入、不發事件，完全符合 MVC 架構紅線（[MVC](../concepts/arch-mvc.md)）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/Character.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Character.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
