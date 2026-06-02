---
id: file:include/game/entities/ProfessorTrapUmbrella.h
type: header
path: include/game/entities/ProfessorTrapUmbrella.h
domain: game
bucket: entities
loc: 38
classes: [ProfessorTrapUmbrella]
sources: ["include/game/entities/ProfessorTrapUmbrella.h"]
---
# `ProfessorTrapUmbrella.h`

> **一句定位**：雨傘 Template Method 樹的「教授陷阱傘」葉類別，`BeClaimed` 覆寫播下後續章節的負面漣漪旗標並追蹤模擬生成的追兵數。

## 職責

`ProfessorTrapUmbrella` 是偷了教授的傘所觸發的陷阱傘，以警示琥珀橙（255,140,30,255）搭配 `UmbrellaStyle::Spiked`（尖角稜角階梯狀傘面）呈現「武裝化、這是陷阱」的剪影。

覆寫 `BeClaimed(player)` 設立持傘狀態（`SetHeldUmbrella(HeldUmbrella::ProfessorTrap)`）與 `Flag_HasProfessorTrap` 旗標，並「播下」後續章節的負面漣漪因子——`Flag_HasProfessorTrap` 在 Ch2 的 `TryApplyCh2Ripple`（-10 業力）與 Ch3 的 `TryApplyCh3Ripple`（-10 業力）、Ch4 的 `TryApplyCh4Ripple`（-15 業力）中各自落地一次，形成跨章節的多次業力懲罰。

額外成員 `spawnedEnemiesCount_` 模擬生成的助教（追兵）數量，供測試驗證（`GetSpawnedEnemiesCount()`）。此計數反映「教授派助教追來」的敘事，在 `.cpp` 中 `BeClaimed` 遞增。

## 關鍵內容（類別 / 函式 / 資料）

- **`ProfessorTrapUmbrella(position)`**：建構子，硬編碼橙色 + `Spiked` 樣式，名稱 `"ProfessorTrapUmbrella"`；初始化 `spawnedEnemiesCount_(0)`。
- **`void BeClaimed(Player* player) override`**：設立持傘狀態 + `Flag_HasProfessorTrap` + 追兵計數，定義於 `.cpp`。
- **`int GetSpawnedEnemiesCount() const noexcept`**：取模擬生成的追兵數（測試用）。
- **`int spawnedEnemiesCount_`**（private）：模擬追兵計數。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/TransparentUmbrella.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`、`src/game/entities/ProfessorTrapUmbrella.cpp`、`tests/controller/test_factory.cpp`、`tests/entities/test_umbrella_render.cpp`、`tests/quest/test_ripple_seed_flags.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `TransparentUmbrella`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層 Item，地圖上由 `WorldSpawn` 生成（Ch1 四把傘之一）；`BeClaimed` 後幀末 Sweep 移除；`Flag_HasProfessorTrap` 在後續每章的 E 互動鉤子中落地漣漪業力。

## OO 概念與設計重點

`ProfessorTrapUmbrella` 是四把劇情傘中後果跨章節最長、最分散的葉類別——它不在拾取時一次性結算，而是在 Ch2/Ch3/Ch4 三個章節各自扣除業力，讓玩家感受到跨越整場遊戲的「選擇後果」。這是 [Template Method](../concepts/pat-template.md) 葉類別覆寫 `BeClaimed` 的設計給予每把傘獨特後果的具體體現。

`spawnedEnemiesCount_` 的設計說明了「可測試性」：追兵生成邏輯無需在測試中執行完整場景，只要驗證計數正確增加即可。這是「暴露內部狀態供測試讀取」而非「為測試修改生產邏輯」的取捨。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/ProfessorTrapUmbrella.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/ProfessorTrapUmbrella.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
