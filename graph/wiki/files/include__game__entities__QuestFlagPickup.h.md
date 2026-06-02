---
id: file:include/game/entities/QuestFlagPickup.h
type: header
path: include/game/entities/QuestFlagPickup.h
domain: game
bucket: entities
loc: 71
classes: [QuestFlagPickup]
sources: ["include/game/entities/QuestFlagPickup.h"]
---
# `QuestFlagPickup.h`

> **一句定位**：任務旗標拾取物——拾取時設立具名玩家旗標的一次性地面道具，支援整組完成獎勵與依數量切換訊息。

## 職責

`QuestFlagPickup` 是地圖上的任務道具，拾取後立即設立 `Player::SetFlag(flagName_)` 並失效（`isActive_ = false`）。典型用例包括 Ch1 加退選申請書、Ch2 三頁散落筆記、以及 Ch1 苦主被吹走的傘（由 `QuestFlagPickup` 設 `Flag_HasVictimUmbrella`）。

**整組完成獎勵**：攜帶 `completionFlags_`（姊妹旗標清單）與 `completionKarma_`（業力加成）。拾取後若「全部」姊妹旗標都已設立（包含剛設的這個），觸發業力加成。由於只有「最後」一個被撿起時才能看到所有旗標齊全，加成恰好觸發一次，無須守衛旗標——此為精妙的反應式設計。

**依數量切換訊息**：`countMessages_` 非空時，畫面訊息依玩家「已持有 `completionFlags_` 中幾個」決定（0個→[0]、1個→[1]、2個→[2]），而非依具體道具 ID。這讓 Ch2 三頁筆記不論以任何順序撿起，都能正確印出「找到第一張／第二張／最後一張」。

繪製（`Render`）依道具種類選擇地面標記：傘旗標（`Flag_HasVictimUmbrella`）畫真傘字符（呼叫 `UmbrellaGlyph`），紙張類（申請書、筆記）畫白色紙頁矩形。

ISP 設計：扮演 `IDrawable`（地面標記）+ `IInteractable`（設旗標），捨棄 `IUpdatable`（道具不需逐幀更新）。

## 關鍵內容（類別 / 函式 / 資料）

- **`QuestFlagPickup(position, flagName, message, completionFlags, completionKarma, countMessages)`**：完整建構子；`completionFlags`、`completionKarma`、`countMessages` 皆可選（預設值）。
- **`void Render(IRenderer&) const override`**：依道具種類選擇地面標記（傘字符或紙頁矩形）。
- **`void Interact(Player*) override`**：轉呼叫 `OnPickup(initiator)`。
- **`void OnPickup(Player*) override`**：設旗標、依數量選訊息、整組集齊時加業力。
- **`std::string flagName_`**（private）：拾取時設立的旗標名稱。
- **`std::string message_`**（private）：單則訊息（`countMessages_` 為空時採用）。
- **`std::vector<std::string> completionFlags_`**（private）：整組完成所需的姊妹旗標。
- **`int completionKarma_`**（private）：整組集齊時的業力加成。
- **`std::vector<std::string> countMessages_`**（private）：依已持有旗標數量挑選的訊息清單。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`（世界座標）、`include/game/entities/Item.h`（基底）；`<string>`、`<vector>`。
- **被誰使用（往內）**：`src/game/entities/QuestFlagPickup.cpp`（實作，含 `UmbrellaGlyph` 呼叫）、`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（生成）；多個測試檔。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<QuestFlagPickup, Item>`，實作 `IDrawable`、`IInteractable`（`Roles.h`）。葉類別，`WithRoles` 以 `QuestFlagPickup` 自身為鍵。
- **每幀管線 / MVC 角色**：Model 層一次性道具。View 呼叫 `Render` 繪地面標記；Controller E 互動掃描呼叫 `Interact`；拾取後 `isActive_=false`，幀末 Sweep 移除。

## OO 概念與設計重點

`QuestFlagPickup` 的「整組完成獎勵」是一個精妙的反應式設計：每個拾取物自帶知道「我是哪組的」所需資訊（`completionFlags_`），無需中央協調器。最後被撿起者自然看到所有旗標滿足，業力恰好觸發一次。此設計避免了「需要維護一個全局筆記收集狀態」的複雜性，是輕量級事件聚合的實踐。

`countMessages_` 的依序號切換訊息設計讓「以任意順序探索」的開放式玩法感覺自然（訊息不依特定道具 ID，而依「已有幾個」），同時保持資料驅動而非特殊案例程式碼。[ISP](../concepts/oo-isp-roles.md) 的精確扮演（捨棄 `IUpdatable`）確保更新管線不執行空覆寫。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/QuestFlagPickup.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/QuestFlagPickup.h) · [← 全檔索引](../files-index.md) · 相關概念：[ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
