---
id: file:src/game/entities/NPC.cpp
type: source
path: src/game/entities/NPC.cpp
domain: game
bucket: entities
loc: 224
classes: []
sources: ["src/game/entities/NPC.cpp"]
---
# `NPC.cpp`

> **一句定位**：NPC 實體的完整實作——支援定點、隨機漫步（xorshift32）、圓形繞圈三種行動模式，以及帶 Pipoya 行走動畫的渲染和對話互動。

## 職責

`NPC` 繼承自 `WithRoles<NPC, Character>`，是遊戲中所有非玩家角色的通用基底實作，涵蓋行動、動畫、渲染、對話四個職責。

**建構與行動模式設定**：預設建構為定點（`wander_=false`，`circular_=false`）。`EnableWander(speed, seed)` 開啟隨機漫步，以 xorshift32（種子非零時用傳入值，零則用 `0x9E3779B9u`）每 1–3 秒重新抽取八方位或暫停。`EnableCircularRun(center, radius, angularSpeed, startAngle)` 開啟沿圓形軌道的連續滑行（校慶繞圈跑者）。

**Update 邏輯**：
- 圓形模式：每幀更新角度（`circleAngle_ += circleSpeed_ * dt`）、用 `cos/sin` 計算新位置、更新 `facing_`、推進動畫計時器（`kFrameDur = kPipoyaCell / WalkFrameDuration`，共用 `WalkCycle.h`）。
- 漫步模式：倒數 `retargetTimer_`，歸零時以 xorshift32 從 9 方向（含暫停）抽一個新向量，重設計時器。呼叫 `Character::Move` 積分位移，夾制到世界邊界，若有 `wanderMask_` 則呼叫 `physics::ResolveMove` 進行碰撞解算（撞牆時縮短下次重新定向的等待時間）。取「穩定朝向（`wanderDir_`）」而非「逐幀淨位移」更新 `facing_`，避免對角線行走時動畫抖動。`moving_` 閘控動畫，靜止時 `animStep_=0`（待機欄）。
- 定點：直接返回。

**Render**：無有效 sprite 時顯示綠色方塊；靜態整張貼圖（`staticSprite_=true`，自動販賣機）縮放至 48px 高置中；Pipoya 行走圖集模式呼叫 `CurrentRenderCell()`（`WalkColumn(animStep_)` + `WalkRowForFacing(facing_)`），繪製 32×32 sprite 底部置中於 24×24 碰撞盒。

**對話**：`Interact(Player*)` 先快照當前台詞並推進索引（EventBus 同步遞迴派發的防護），再發布 `ShowMessage`。`LoadDialog(npcId, state, subState)` 從 `nccu::dialog::Entries` 查對應子段後呼叫 `SetDialogLines`。

## 關鍵內容（類別 / 函式 / 資料）

- `NPC(Vec2, vector<string>, bool isQuestGiver, string_view npcId)`：建構，24×24 碰撞盒，定點初始化。
- `NPC& EnableWander(float speed, unsigned seed)`：開啟漫步，xorshift32 RNG，流式回傳。
- `NPC& EnableCircularRun(Vec2 center, float radius, float angularSpeed, float startAngle)`：開啟圓形繞圈。
- `void Update(float deltaTime)`：三模式邏輯 + 動畫推進。
- `void Render(IRenderer&) const`：三渲染路徑（fallback / staticSprite / Pipoya 圖集）。
- `NPC::RenderCell CurrentRenderCell() const noexcept`：回傳 `{col, row}` 的行走圖集欄列。
- `void Interact(Player*)`：先快照台詞再推進，發布 `ShowMessage`。
- `NPC& LoadDialog(string_view npcId, SemesterState, int subState)`：從對白倉儲載入指定子段台詞。
- `NPC& SetDialogLines(vector<string>)`：直接設置台詞並重置索引。
- `void LoadSprite(const string& path)`：載入 Texture。
- `wander_` / `circular_` / `rng_` / `retargetTimer_` / `wanderDir_`：漫步狀態。
- `circleCenter_` / `circleRadius_` / `circleSpeed_` / `circleAngle_`：圓形狀態。
- `staticSprite_`：是否使用整張靜態貼圖（自動販賣機模式）。
- `wanderMask_`：碰撞遮罩指標，用於漫步碰撞解算。

## 相依與在架構中的位置

- **#include（往外）**：`NPC.h`、`DialogSource.h`（`Entries` 查詢）、`EventBus.h` / `EventSink.h`、`Physics.h`（`ResolveMove`）、`WorldConfig.h`（`kSize` / `kPlayerWidth/Height`）、`IRenderer.h` / `Color.h` / `Rect.h`、`WalkCycle.h`（`kPipoyaCell` / `kWalkFrameDuration` / `WalkColumn` / `WalkRowForFacing`）。
- **被誰使用（往內）**：—（葉節點；由 `World::SpawnChapterNpcs` 建立，由 `GameController` 驅動 `Update`/`Render`/`Interact`）。
- **繼承 / 實作 / 體現**：`NPC` → `WithRoles<NPC, Character>` → `Character` → `GameObject`；實作 `IUpdatable`（`Update`）、`IDrawable`（`Render`）、`IInteractable`（`Interact`）。
- **每幀管線 / MVC 角色**：Model 層動態實體；在 Movement 階段呼叫 `Update`，在 E 互動階段呼叫 `Interact`，View 呼叫 `Render`。

## OO 概念與設計重點

`NPC` 以三個行動模式（`wander_` / `circular_` / 定點）在單一類別中支援全遊戲的 NPC 行為多樣性，而非用子類別繼承，體現了組合優於繼承的原則。`WalkCycle.h` 共用的 Pipoya 圖集邏輯（與 `Player.cpp` 一致）是單一真實來源原則。漫步方向取「穩定 `wanderDir_`」而非「逐幀淨位移」的設計，針對對角線行走動畫抖動做出了明確的工程決策，並在行內詳細說明了原因。[CRTP](../concepts/oo-crtp.md) 保證角色介面在編譯期靜態分派，[Observer](../concepts/pat-observer.md) 體現於 `ShowMessage` 事件。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/NPC.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/NPC.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [ISP 角色介面](../concepts/oo-isp-roles.md) · [Observer](../concepts/pat-observer.md)
