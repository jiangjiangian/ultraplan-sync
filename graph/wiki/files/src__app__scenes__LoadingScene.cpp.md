---
id: "file:src/app/scenes/LoadingScene.cpp"
type: source
path: src/app/scenes/LoadingScene.cpp
domain: app
bucket: scenes
loc: 75
classes: []
sources: ["src/app/scenes/LoadingScene.cpp"]
---
# `LoadingScene.cpp`

> **一句定位**：載入場景實作：在 `Enter()` 暖機全域紋理快取，停留固定 18 幀後切換至下一場景。

## 職責

`LoadingScene` 是場景鏈的第一個場景，主要職責是在進入 `TitleScene` 之前完成一次性的紋理磁碟讀取與 GPU 上傳（`PreloadGameTextures()`），避免首個遊玩幀發生卡頓。

`Enter()` 呼叫 `nccu::game::world::PreloadGameTextures()`，此為冪等操作：在乾淨 clone（缺少資產）或資產已上傳（快取命中）時皆安全。設計在 `Enter` 執行確保暖機只跑一次，早於任何 Update/Draw 配對。

`Update()` 以 `frameCount_` 累積幀數；達到 `kHoldFrames = 18`（約 0.3 秒於 60 fps）後回傳 `SceneCommand::Replace(next_)` 切換到下一場景。`18` 幀的停留讓玩家意識到「正在載入」，又短到在乾淨 clone 上不感覺卡住。

`Draw()` 渲染深色背景（`kBackdrop{14,16,22,255}`）、置中的「載入中…」大字（34pt）與「正在準備政大山下的雨天…」副標（16pt、`kHint` 灰色），以 `TextBuilder::Measure()` 計算文字寬高後置中。

## 關鍵內容（類別 / 函式 / 資料）

- `kHoldFrames = 18` — 暖機後停留幀數，約 0.3 秒。
- `kBackdrop{14,16,22,255}` — 深藍背景色。
- `LoadingScene::Enter()` — `PreloadGameTextures()` 一次性紋理暖機。
- `LoadingScene::Update(float)` — `++frameCount_` 計數，達 `kHoldFrames` 且 `next_` 有效時回傳 Replace。
- `LoadingScene::Draw(IRenderer&)` — 置中「載入中…」與副標，以 `TextBuilder::Measure()` 計算位置。

## 相依與在架構中的位置
- **#include（往外）**：`LoadingScene.h`；`Renderer.h` / `TextBuilder.h`（渲染）；`Color.h` / `Vec2.h`（數學）；`TexturePreload.h`（`PreloadGameTextures`）
- **被誰使用（往內）**：—（葉節點；場景鏈第一站，由 `SceneBootstrap::MakeHumanInitialScene` 建立）
- **繼承 / 實作 / 體現**：實作 `IScene`（Enter / Update / Draw）
- **每幀管線 / MVC 角色**：app / scenes 層；非遊玩場景（`WorldForHarnessOrNull()` 回傳 nullptr），不參與 Harness EndFrame 序列化

## OO 概念與設計重點

此場景是「暖機屏障」的設計模式：以一個短暫的過場隱藏一次性的資源載入，讓後續所有場景能即時存取紋理快取。RAII 設計——`next_` 工廠以值捕獲，場景解構時工廠 closure 自然釋放。`kHoldFrames` 的固定幀數（而非秒數）確保在 harness 固定步長（1/60 s）下行為與正常遊玩相同，屬於 [Harness](../concepts/arch-harness.md) 架構下的決定性設計。`WorldForHarnessOrNull()` 回傳 nullptr 使 SceneManager 不對此場景呼叫 `EndFrame`，不污染 state.jsonl。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/LoadingScene.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/LoadingScene.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [RAII](../concepts/oo-raii.md)
