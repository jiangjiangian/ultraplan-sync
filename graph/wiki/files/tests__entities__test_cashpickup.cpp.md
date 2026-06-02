---
id: "file:tests/entities/test_cashpickup.cpp"
type: test
path: tests/entities/test_cashpickup.cpp
domain: tests
bucket: entities
loc: 107
classes: [MessageCapture]
sources: ["tests/entities/test_cashpickup.cpp"]
---
# `test_cashpickup.cpp`

> **一句定位**：驗證 `CashPickup` 的撿取語意（增加玩家金錢、自我停用、發出 `ShowMessage`）以及 `GameObjectFactory` 產生各面額（5/10/20 元）硬幣的正確性。

## 職責

本檔包含 5 個 `TEST_CASE`，覆蓋 `CashPickup` 的全部功能契約。

**`MessageCapture` fixture**：訂閱 `ShowMessage` 事件，記錄 `hits` 次數與 `lastText`，在各 case 中驗證 EventBus 輸出。使用 `EventBus::Instance().Clear()` + 裸 `Subscribe`（注意：此檔採 raw Subscribe，不使用 ScopedSubscribe）。

**核心撿取語意**：`CashPickup coin({10,20}, 7)` + `coin.OnPickup(&p)`；驗證：`p.GetMoney() == before + 7`（金錢增加）、`coin.IsActive() == false`（物件停用）、`cap.hits == 1`（事件一次）、`cap.lastText == "撿到 7 元"`（正確文字）。前置檢查：`coin.IsActive() == true`、`coin.Value() == 7`、`coin.GetName() == "Cash"`。

**null 玩家安全**：`coin.OnPickup(nullptr)` 後物件維持啟用、不發事件（`cap.hits == 0`），確認不崩潰。

**Factory 面額 5/10/20**：三個 case 各自以 `GameObjectFactory::Create(ObjectType::CashPickup5/10/20, {0,0})` 建立，`dynamic_cast<CashPickup*>` 驗證型別，`coin->Value() == 5/10/20`，撿取後玩家金錢加正確面額。

## 關鍵內容（類別 / 函式 / 資料）

- `struct MessageCapture`：EventBus ShowMessage 捕捉器（`hits`、`lastText`、`Attach()`）。
- `CashPickup::OnPickup(Player*)`：撿取的核心動作。
- `CashPickup::Value()`：面額查詢。
- `CashPickup::GetName()`：物件名稱（`"Cash"`）。
- `GameObjectFactory::Create(ObjectType::CashPickup5/10/20, Vec2)`：各面額的工廠路徑。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/CashPickup.h`、`include/engine/events/EventBus.h`、`include/game/controller/GameObjectFactory.h`、`include/game/entities/Player.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

本檔測試 [Factory Method](../concepts/pat-factory.md) 的三個 `CashPickup` 路徑，以及 [Observer](../concepts/pat-observer.md) 的 `ShowMessage` 事件發布。null 玩家安全 case 是防禦性程式設計的標準驗證，確保 `CollisionSystem` 在邊界情況下呼叫 `OnPickup(nullptr)` 不導致 undefined behavior。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_cashpickup.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_cashpickup.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-factory](../concepts/pat-factory.md) · [Observer](../concepts/pat-observer.md)
