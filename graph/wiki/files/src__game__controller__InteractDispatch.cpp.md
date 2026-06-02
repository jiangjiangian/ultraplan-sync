---
id: "file:src/game/controller/InteractDispatch.cpp"
type: source
path: src/game/controller/InteractDispatch.cpp
domain: game
bucket: controller
loc: 95
classes: []
sources: ["src/game/controller/InteractDispatch.cpp"]
---
# `InteractDispatch.cpp`

> **一句定位**：E 鍵互動分派的完整實作：以膨脹 AABB 探測鄰近物件，依物件類型路由至商販選單、閒談 NPC、任務 hook 表或通用對話框 / 拾取。

## 職責

`DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor)` 在玩家按下 E（`Input::IsPressed(Key::E)`）且 Player 存在時執行：

1. **互動觸及盒**：以 `kInteractReach`（`LargeTargets` 模式 16 px，否則 8 px）膨脹玩家 24x24 盒為 `pHit`，鏡像 GameController 的 E 探測幾何，確保與 ScriptResolver 的「現在可按 E」閘完全一致。

2. **`ForEachActiveExcept(world.Objects(), player, ...)` 遍歷**：對每個 `pHit.CheckCollision(o)` 為真的物件依序分派：
   - **商販（`o.IsVendor()`）**：對話框已開啟則跳過；否則呼叫 `OpenVendorMenu`，設 `pendingVendor`。
   - **有 NpcId 的物件**：
     - 若為第一章閒談型（`IsChapter1FlavorNpc(id)`）：呼叫 `o.AsInteractable()->Interact(player)` 後 return（不走主線 hook）。
     - 否則：呼叫 `RunInteractHooks`（Ch1-Ch4 任務 hook 表）；再呼叫 `OpenNpcDialog`（對話框開場）。
   - **無 NpcId 且有 `AsInteractable()`**：呼叫 `it->Interact(player)`（拾取 / Vendor 的 `IInteractable`）。

`kInteractReach` 的設計注解詳述：BlocksMovement NPC 的碰撞體阻止玩家穿過，若不膨脹 pHit 則探測盒永遠不與 NPC 重疊，導致「貼臉也按不到 E」的軟鎖——此 8 px（或 16 px）裕度正是修正該問題的核心。

`IsChapter1FlavorNpc` 的提前 return 確保閒談型 NPC（搶課同學 / 撐傘路人 / 揹書包學生）絕不觸及主線旗標 / 傳球 TryXxx hook，是任務進程不變式的守護。

## 關鍵內容（類別 / 函式 / 資料）

- `DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor)` — 完整分派邏輯；三路：Vendor / 有 NpcId / 無 NpcId 拾取。
- `kInteractReach` — `LargeTargets ? 16.0f : 8.0f`；膨脹 pHit 的互動觸及距離。
- `pHit` — 玩家位置外擴 kInteractReach 的 Rect（`{px - r, py - r, 24 + 2r, 24 + 2r}`）。
- `RunInteractHooks` — 依序跑 Ch1-Ch4 任務 hook 表，新章節只需 RegisterHook（OCP）。
- `OpenNpcDialog` — 讀取對話資料開啟對話框（`DialogState`）。
- `IsChapter1FlavorNpc` — 第一章閒談型 NPC 識別，提前 return 守護主線不變式。

## 相依與在架構中的位置
- **#include（往外）**：`InteractDispatch.h`；`World.h`、`Player.h`（Model）；`DialogOpener.h`（`OpenNpcDialog`）、`DialogState.h`；`Vendor.h`、`VendorMenu.h`（`OpenVendorMenu`）；`GameObjectQueries.h`（`ForEachActiveExcept`）；`NpcSpawns.h`（`IsChapter1FlavorNpc`）；`QuestHookTable.h`（`RunInteractHooks`）；`Input.h`、`Key.h`；`Rect.h`；`GameObject.h`
- **被誰使用（往內）**：—（由 `GameController::DispatchInteract()` 薄轉發呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；在 Survival→…→Spawn 管線之後、Sweep 之前執行（GameController::Update 的互動分派段）

## OO 概念與設計重點

`RunInteractHooks` 體現 [Command](../concepts/pat-command.md) 思維（任務 hook 是可登記的行為表）與 OCP：新增章節任務只需 `RegisterHook`，不需修改此分派函式。互動觸及盒的「鏡像 GameController 幾何」設計是 harness 逐位元一致性的要求——腳本的 `interact` 動詞與真人按 E 使用完全相同的幾何。[ISP / Roles](../concepts/oo-isp-roles.md) 體現在 `o.AsInteractable()` 的空值安全查詢，無需 `dynamic_cast`。商販 `IsVendor()` 的特化分支（在 NpcId 之前路由）確保 `Vendor` 的購買流程完整落地，不被通用對話覆蓋。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/InteractDispatch.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/InteractDispatch.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[ISP Roles](../concepts/oo-isp-roles.md) · [Command](../concepts/pat-command.md) · [MVC](../concepts/arch-mvc.md)
