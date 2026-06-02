---
id: "file:tests/entities/test_dlc_sign.cpp"
type: test
path: tests/entities/test_dlc_sign.cpp
domain: tests
bucket: entities
loc: 140
classes: [MessageCapture]
sources: ["tests/entities/test_dlc_sign.cpp"]
---
# `test_dlc_sign.cpp`

> **一句定位**：驗證 `DlcSign`（風雩走廊「?」彩蛋告示牌）的可重複互動語意、角色身分（IInteractable+IDrawable 非 NPC/Vendor）、`GameObject&` 分派路徑，以及僅在 `Chapter4_Finals` 生成並隨章節結束清除。

## 職責

本檔包含 4 個 `TEST_CASE`，完整測試 `DlcSign` 這個無玩法效果的彩蛋物件。

**可重複互動與無玩法效果**：`sign.Interact(&p)` 發出 `ShowMessage`（`"DLC開發中\n敬請期待"`，含換行字元），`IsActive()` 仍為 true（非一次性消耗）；玩家 karma/money 不動、無傘。再次互動發出第二次（`cap.hits == 2`），仍啟用。

**角色與身分**：以 `GameObject& asObj = sign` 驗證角色分派：`AsInteractable() != nullptr`、`AsDrawable() != nullptr`；`AsUpdatable() == nullptr`（不逐幀更新）；`NpcId().empty()`、`IsVendor() == false`、`IsQuestGiver() == false`、`BlocksMovement() == false`。

**`GameObject&` 分派 Interact（重現掃描路徑）**：模擬 `GameController` 的 E 互動分派路徑：`if (auto* it = asObj.AsInteractable()) it->Interact(&p)`；驗證事件發出、物件仍存活。

**章節生命週期**：`CountDlcSigns` helper 統計 `World::Objects()` 中 `DlcSign` 的數量。Ch1 下 `CountDlcSigns(w) == 0`；`RespawnChapterRoster(Chapter4_Finals)` 後 `== 1`（恰好一個）；`RespawnChapterRoster(Ending_C)` 後 `== 0`（隨章節清除）。

## 關鍵內容（類別 / 函式 / 資料）

- `struct MessageCapture`：raw Subscribe ShowMessage 捕捉器（`Attach()` 清再訂）。
- `CountDlcSigns(World&)`：統計 `dynamic_cast<DlcSign*>` 成功的物件數。
- `DlcSign::Interact(Player*)`：發出預告 ShowMessage，不停用自身。
- `DlcSign` 角色：`IInteractable + IDrawable`，無 `IUpdatable/IMortal`。
- `World::RespawnChapterRoster(SemesterState)`：手動觸發章節物件重生（用於無 GameController 的單元測試）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/DlcSign.h`、`include/engine/core/GameObject.h`、`include/engine/events/EventBus.h`、`include/game/entities/Player.h`、`include/game/world/World.h`、`include/game/state/SemesterState.h`、`include/engine/math/Vec2.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

`DlcSign` 體現了 [oo-isp-roles](../concepts/oo-isp-roles.md) 的角色最小化：只實作 `IInteractable + IDrawable`，不實作 `IUpdatable` 或 `IMortal`，確保 `GameController` 不對它呼叫 `Update` 或 sweep 語意。「角色與身分」case 以 `AsInteractable/AsDrawable/AsUpdatable/NpcId/IsVendor` 六項驗證，防止未來把 DlcSign 錯誤地加入 NPC 或 Vendor 的處理路徑。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_dlc_sign.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_dlc_sign.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[oo-isp-roles](../concepts/oo-isp-roles.md)
