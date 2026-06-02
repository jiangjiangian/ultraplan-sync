---
id: file:src/game/entities/CursedUmbrella.cpp
type: source
path: src/game/entities/CursedUmbrella.cpp
domain: game
bucket: entities
loc: 27
classes: []
sources: ["src/game/entities/CursedUmbrella.cpp"]
---
# `CursedUmbrella.cpp`

> **一句定位**：詛咒傘的認領實作——以「污點計數器」取代即時業力懲罰，每章節邊界才兌現衰減，實現可累乘的道德咬噬機制。

## 職責

`CursedUmbrella::BeClaimed(Player*)` 是 `TransparentUmbrella` Template Method 鏈的末端覆寫，在玩家實際認領詛咒傘時執行。

**冪等守衛**：先以 `!isActive_` 短路，確保任何重複呼叫（如碰撞 + 互動在同一幀觸發）皆為無動作。

**污點機制**：不再一次性扣業力，而是呼叫 `player->IncCursedTaint()`（累加污點計數）。每章過場（`SceneRouter` Ch2/3/4 進場）呼叫 `ApplyCursedTaintDecay`，對 `cursedTaint_` 乘以 -5 扣業力。第一次拾取在三章過場各扣 -5（最多 -15）；第二次拾取後 -10/章，第三次後 -15/章，道德後果以再犯次數累乘。

**旗標設定**：無條件設下 `kFlagTookCursedUmbrella`（Ending B 前提）和 `HeldUmbrella::Cursed`（背包列記錄）。污點從不硬鎖結局 B——高業力玩家仍可抵達 Ending A（EndingGate 的 A→B 優先序保證）。

**事件順序**：發布 `UmbrellaClaimed("CursedUmbrella")` 在 `ShowMessage` 之後，使玩家看到「成為了你最討厭的人」的拾取字幕，而章節業力懲罰在後續過場才兌現。

## 關鍵內容（類別 / 函式 / 資料）

- `CursedUmbrella::BeClaimed(Player*)`：唯一實作，冪等守衛 + 污點計數 + 旗標設定 + 兩則事件。
- `Player::IncCursedTaint()`：累加詛咒污點計數器。
- `Player::SetHeldUmbrella(HeldUmbrella::Cursed)`：背包記錄並開啟遮蔽。
- `Player::SetFlag(kFlagTookCursedUmbrella)`：Ending B 前提旗標。
- `EventType::UmbrellaClaimed` / `EventType::ShowMessage`：發布順序：ShowMessage 先、UmbrellaClaimed 後。

## 相依與在架構中的位置

- **#include（往外）**：`CursedUmbrella.h`、`Player.h`（`SetHeldUmbrella` / `IncCursedTaint` / `SetFlag`）、`EventBus.h` / `EventSink.h`（發布事件）、`Flags.h`（`kFlagTookCursedUmbrella`）。
- **被誰使用（往內）**：—（葉節點；`TransparentUmbrella::BeClaimed` 的具體多型終端）。
- **繼承 / 實作 / 體現**：`CursedUmbrella` → `TransparentUmbrella` → `Item` → `GameObject`；`BeClaimed` 覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：在 Collision / E 互動階段被呼叫（`QuestGateOpen` 通過後），在 Sweep 階段清除實體。

## OO 概念與設計重點

本檔是傘家族 [Template Method](../concepts/pat-template.md) 的末端節點之一：`BeClaimed` 是抽象基底定義的「認領鉤子」，各傘子類別各自覆寫以實現不同的遊戲後果（詛咒傘的污點計數、真傘的旗標、脆傘的骨架破損等）。「污點而非即時業力」的設計是刻意的非線性道德系統：懲罰延遲到章節邊界，給予玩家犯錯後仍有機會補救的緩衝空間。[Observer 模式](../concepts/pat-observer.md) 體現於 `UmbrellaClaimed` 事件——由 `EventWiring` 接線到章節清關邏輯，`CursedUmbrella` 本身不感知。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/CursedUmbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/CursedUmbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [Observer](../concepts/pat-observer.md)
