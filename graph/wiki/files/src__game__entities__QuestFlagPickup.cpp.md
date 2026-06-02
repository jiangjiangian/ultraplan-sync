---
id: file:src/game/entities/QuestFlagPickup.cpp
type: source
path: src/game/entities/QuestFlagPickup.cpp
domain: game
bucket: entities
loc: 104
classes: []
sources: ["src/game/entities/QuestFlagPickup.cpp"]
---
# `QuestFlagPickup.cpp`

> **一句定位**：任務道具拾取物的通用實作——設旗標、依已撿數量選訊息，集齊時給予業力獎勵，並以資料驅動方式分別繪製傘或紙張外觀。

## 職責

`QuestFlagPickup` 是地面任務道具的通用實體，涵蓋申請書（`Flag_FoundForm`）、散落筆記（`Flag_FoundNote1/2/3`）、苦主的透明傘（`Flag_HasVictimUmbrella`）等。

**建構**：接受 `position`、`flagName`（要設立的旗標）、`message`（拾取訊息）、`completionFlags`（整組完成的旗標清單）、`completionKarma`（完成獎勵）、`countMessages`（依撿到幾個選訊息的陣列）；16×16 碰撞盒，類型字串 `"QuestItem"`。

**Render（資料驅動外觀）**：呼叫 `IsUmbrellaFlag(flagName_)` 判斷是否為傘旗標（以子字串 `"Umbrella"` 比對），傘類使用 `DrawUmbrellaGlyph(renderer, UmbrellaLook::TrueBlue, hitBox_)` 繪製藍色真傘字符；其餘任務紙張以 7 個 `DrawRect` 繪出白色紙頁 + 折角 + 兩道文字線條。純矩形，不含 raylib。

**OnPickup（拾取邏輯）**：設 `player->SetFlag(flagName_)`，設 `isActive_=false`，再計算要顯示哪則訊息：若有 `countMessages_` 且 `completionFlags_` 非空，則統計玩家此刻持有 completionFlags 中的個數 `held`（旗標剛才已設立，故含自己）；以 `held-1` 為索引（夾限至陣列尾端），選出對應的「第一/第二/最後一張」句子。整組完成獎勵：當所有 `completionFlags_` 旗標皆已滿足（收尾那次 `held==completionFlags_.size()`）且 `completionKarma_!=0`，呼叫 `player->AddKarma(completionKarma_)`。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestFlagPickup(Vec2, string flagName, string message, vector<string> completionFlags, int completionKarma, vector<string> countMessages)`：建構，16×16 碰撞盒。
- `void Render(IRenderer&) const`：依旗標名稱決定外觀（傘字符 vs 紙張矩形）。
- `void OnPickup(Player*)`：設旗標 + 失活 + 依數量選訊息 + 整組完成業力獎勵。
- `IsUmbrellaFlag(string_view)` → `bool`：子字串比對 `"Umbrella"`（匿名命名空間）。
- `flagName_` / `message_` / `completionFlags_` / `completionKarma_` / `countMessages_`：成員。

## 相依與在架構中的位置

- **#include（往外）**：`QuestFlagPickup.h`、`EventBus.h` / `EventSink.h`（ShowMessage）、`Player.h`（`SetFlag` / `HasFlag` / `AddKarma`）、`IRenderer.h` / `Color.h` / `Rect.h`、`UmbrellaGlyph.h`（`DrawUmbrellaGlyph`）。
- **被誰使用（往內）**：—（葉節點；由 `World::SpawnChapterNpcs` 或 Factory 建立）。
- **繼承 / 實作 / 體現**：繼承 `WithRoles<QuestFlagPickup, Item>`；實作 `IDrawable`（`Render`）、`IInteractable`（`OnPickup`）。
- **每幀管線 / MVC 角色**：在 Collision 拾取掃描時觸發 `OnPickup`，Sweep 清除。

## OO 概念與設計重點

**資料驅動**：外觀和訊息完全由建構時傳入的參數決定，使同一個類別能服務不同類型的任務道具，而無需為每種道具建子類別。[CRTP](../concepts/oo-crtp.md) 的 `WithRoles` 在編譯期掛載角色，[Observer](../concepts/pat-observer.md) 體現於 `ShowMessage`。子字串 `"Umbrella"` 比對的外觀判斷是一種輕量的旗標命名慣例，使未來新增傘旗標時外觀自動套用正確樣式（開放/封閉原則的弱形式）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/QuestFlagPickup.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/QuestFlagPickup.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [Observer](../concepts/pat-observer.md)
