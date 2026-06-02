---
id: file:include/game/entities/NPC.h
type: header
path: include/game/entities/NPC.h
domain: game
bucket: entities
loc: 198
classes: [NPC, RenderCell]
sources: ["include/game/entities/NPC.h"]
---
# `NPC.h`

> **一句定位**：非玩家角色的完整宣告，支援定點對話、隨機漫步與校慶繞圈三種行為模式，為 `Vendor` 的基底。

## 職責

`NPC` 繼承自 `Character`，扮演全部三個動態角色：`IUpdatable`（漫步與繞圈動畫）、`IDrawable`（Pipoya sprite）、`IInteractable`（循環對話）。每個實作都是真實的——三個角色皆非空殼，故滿足 ISP。

**三種行為模式**透過成員旗標切換，而非子類別繼承：
- **定點原型 NPC**（預設）：不移動，`BlocksMovement()` 返回 `true`（玩家會被推開），扮演劇情核心 NPC（苦主、管理員、學霸等）。
- **漫步環境學生**：`EnableWander(speed, seed)` 開啟，每 1-3 秒以 PRNG 重選朝向，`BlocksMovement()` 返回 `false`（不阻擋玩家，為裝飾性人群）。
- **校慶繞圈跑者**：`EnableCircularRun(center, radius, angularSpeed, startAngle)` 開啟，以角速度繞圓形跑道移動，附行走動畫。

**對話系統**：持有 `dialogLines_` 向量，`Interact` 每次呼叫推進 `currentLineIndex_`（循環）。`LoadDialog(npcId, state, subState)` 允許章節狀態機在換章時推入新台詞。`NpcId()` 覆寫 `GameObject` 的虛擬函式，供 `GameController` 以 `(npcId, SemesterState)` 組出對話分流，無須 `dynamic_cast`。

**動畫系統**：`animTimer_`、`animStep_`、`facing_` 驅動 Pipoya 96×128 圖集的四向行走循環；`CurrentRenderCell()` 暴露本幀應繪的 `{col, row}`，供無頭測試直接讀取（不依賴 GL context）。`SetStaticSprite(true)` 允許美術為自動販賣機等靜態 NPC 繪整張圖而非切格。

## 關鍵內容（類別 / 函式 / 資料）

- **`NPC(position, dialogLines, isQuestGiver, npcId)`**：主建構子。
- **`void Update(float deltaTime) override`**：推進漫步方向、繞圈角度、行走動畫計時器與 `animStep_`。
- **`void Render(IRenderer&) const override`**：依 `CurrentRenderCell()` 切出 Pipoya 格繪製 sprite；缺 texture 時以備援矩形代替。
- **`void Interact(Player*) override`**：發布當前 `dialogLines_[currentLineIndex_]` 的 `ShowMessage` 並推進索引（循環）。
- **`std::string_view NpcId() const noexcept override`**：對話查找鍵。
- **`bool BlocksMovement() const noexcept override`**：定點 NPC 為 true；漫步 / 繞圈跑者為 false。
- **`NPC& EnableWander(speed, seed) noexcept`**：轉為漫步環境學生（回傳 *this 串接）。
- **`NPC& EnableCircularRun(center, radius, angularSpeed, startAngle) noexcept`**：轉為繞圈跑者（回傳 *this 串接）。
- **`void SetWanderMask(const CollisionMask&) noexcept`**：注入地形遮罩，使漫步 NPC 自行避開建築。
- **`void LoadSprite(const std::string& path)`**：載入 Pipoya 圖集。
- **`void SetStaticSprite(bool v) noexcept`**：整張 sprite 模式（供自販機等靜態 NPC）。
- **`NPC& SetDialogLines(vector<string>)`**：替換台詞並重置索引（供狀態機換章更新）。
- **`NPC& LoadDialog(npcId, state, subState)`**：依執行期章節內容替換對話。
- **`RenderCell CurrentRenderCell() const noexcept`**：本幀應繪 Pipoya `{col, row}`（測試用）。
- **`struct RenderCell { int col; int row; }`**：Pipoya 格位查詢結構。
- 私有成員：`dialogLines_`、`currentLineIndex_`、`npcId_`、`wander_`、`circular_`、`rng_`、`wanderMask_`、`animTimer_`、`animStep_`、`moving_`、`facing_`、圓形跑道四個參數。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/Character.h`（可移動基底）、`include/game/world/CollisionMask.h`（漫步避牆）、`include/game/state/SemesterState.h`（`LoadDialog` 章節參數）、`include/engine/render/Texture.h`（sprite 圖集）、`include/engine/math/Rect.h`（碰撞盒）。
- **被誰使用（往內）**：`include/game/vendor/Vendor.h`（唯一的 NPC 子類別）；`World.cpp`、`WorldSpawn.cpp`（持有與生成）；大量測試檔。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<NPC, Character>`，實作 `IUpdatable`、`IDrawable`、`IInteractable`（`Roles.h`）；被 `Vendor` 繼承。
- **每幀管線 / MVC 角色**：Controller 每幀呼叫 `Update`（Movement System 管線）；View 每幀呼叫 `Render`；Controller 的 E 互動掃描呼叫 `Interact`。`WithRoles<NPC, Character>` 為鍵，`Vendor` 的 `static_cast<NPC*>` 合法。

## OO 概念與設計重點

`NPC` 是 [ISP（介面隔離）](../concepts/oo-isp-roles.md) 與 [CRTP mixin](../concepts/oo-crtp.md) 的代表案例：三個角色皆有實質實作，`WithRoles<NPC, Character>` 讓控制器以 `AsUpdatable()`、`AsDrawable()`、`AsInteractable()` 取得各介面指標而無須 `dynamic_cast`。`EnableWander` 和 `EnableCircularRun` 以流式 API（回傳 `*this`）設計，讓 `WorldSpawn` 生成 NPC 時可以鏈式呼叫（與 `Player` 的 `AddKarma(10).AddMoney(50)` 同一風格）。`BlocksMovement()` 根據行為模式而非子類別多型決定，是「行為旗標」替代「繼承爆炸」的設計取捨。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/NPC.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/NPC.h) · [← 全檔索引](../files-index.md) · 相關概念：[ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md) · [MVC](../concepts/arch-mvc.md)
