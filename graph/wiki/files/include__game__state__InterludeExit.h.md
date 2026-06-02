---
id: file:include/game/state/InterludeExit.h
type: header
path: include/game/state/InterludeExit.h
domain: game
bucket: state
loc: 44
classes: []
sources: ["include/game/state/InterludeExit.h"]
---
# `InterludeExit.h`

> **一句定位**：幕間市集的玩家進入座標與南側出口觸發矩形幾何常數，以及純幾何的 `InInterludeExitZone` 判定述詞。

## 職責

`InterludeExit.h` 定義幕間市集的兩個空間關鍵點：玩家進入時被重置的座標 `kInterludeEntry{500, 1500}`（避開南側出口帶，免得一進來就被彈出去），以及南側出口帶的四邊界常數（`kInterludeExitMinX/MaxX = 150/1950`、`kInterludeExitMinY/MaxY = 1900/2048`）。

出口帶設計說明：`y >= 1900` 是玩家出生點（y=1860）再往南的一條已知可步行地面帶，玩家出生時不會落在其中；路人在此路上約於 y~1880 行走，故帶設在 1900 以下。`kInterludeExitMaxY = 2048` 等同 `world::kSize`，覆蓋地圖南緣。

`InInterludeExitZone(Vec2 centre)` 是純幾何述詞，以矩形 AABB 判定玩家碰撞盒中心是否落在出口帶內，由 `GameController` 每幀呼叫以觸發離場旗標。函式是 inline 純函式，可在無 GL 環境下單元測試，因此幾何驗證完全獨立於渲染。

出口採「走出觸發區」設計而非看板 NPC 選單：純資料驅動、無硬編對白、不需動到作者管理的內容檔。

## 關鍵內容（類別 / 函式 / 資料）

- `kInterludeEntry{500.0f, 1500.0f}`（`constexpr Vec2`）：幕間進入時玩家重置座標。
- `kInterludeExitMinX/MaxX/MinY/MaxY`（`constexpr float`）：南側出口帶四邊界。
- `InInterludeExitZone(Vec2 centre) noexcept → bool`：點落在出口帶矩形內回傳 true，inline 純函式。

## 相依與在架構中的位置

- **#include（往外）**：`Vec2.h`（`kInterludeEntry` 型別與 `centre` 參數）
- **被誰使用（往內）**：`include/game/state/InterludeExitMarker.h`（視覺標記幾何用這些常數）、`src/game/controller/GameController.cpp`（每幀呼叫 `InInterludeExitZone`）、`src/game/controller/SceneRouter.cpp`；測試（`test_scene_router.cpp`、`test_interlude_exit.cpp`、`test_interlude_exit_feedback.cpp`、`test_interlude_exit_marker.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 管線的幕間出口判定——每幀 GameController 呼叫 `InInterludeExitZone(player.centre)` 以確定是否應設 `Flag_LeaveInterlude` 並觸發章節轉移。

## OO 概念與設計重點

「純幾何可測試」的設計：所有幾何常數與判定函式都是 inline 且無副作用，使 `test_interlude_exit.cpp` 能在完全不依賴遊戲狀態或渲染的情況下驗證出口判定邏輯。常數的分組（入口、出口帶四邊界）讓 `InterludeExitMarker.h` 可以直接複用，保持幾何的單一真實來源，避免標記與觸發帶不對齊。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/InterludeExit.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeExit.h) · [← 全檔索引](../files-index.md)
