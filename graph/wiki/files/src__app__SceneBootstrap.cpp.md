---
id: "file:src/app/SceneBootstrap.cpp"
type: source
path: src/app/SceneBootstrap.cpp
domain: app
bucket: ""
loc: 64
classes: []
sources: ["src/app/SceneBootstrap.cpp"]
---
# `SceneBootstrap.cpp`

> **一句定位**：負責組裝完整場景鏈（Loading → Title → CharacterSelect → Gameplay）或錄製略過路徑的 composition root 輔助，讓 `main.cpp` 維持精簡。

## 職責

此檔案實作唯一的公開函式 `PushInitialScene`，決定首次進入哪條場景路徑。人類路徑中，內部的匿名函式 `MakeHumanInitialScene` 以值捕獲工廠 closure 遞迴地建構完整鏈：`LoadingScene` → `TitleScene` → `CharacterSelectScene` → `GameplayScene`；每個場景的建構子捕獲「下一段」工廠，形成一條自帶依賴的鏈結。

重新開始（`restartFactory`）被設計為再次呼叫 `MakeHumanInitialScene`，只捕獲 `audio` 與 `harness` 的借用參考（程式級壽命、main 擁有），刻意避開 `shared_ptr` 循環參考與前向宣告的 `std::function` 懸空問題。

錄製（harness）模式下，`PushInitialScene` 直接建構一個不帶 `restartFactory` 的 `GameplayScene`，略過 Loading/Title/Select 三個場景，以決定性角色（`harnessSel.spritePath = harness.SpritePath()`）直接進入遊玩，確保存檔逐位元一致。

## 關鍵內容（類別 / 函式 / 資料）

- `MakeHumanInitialScene(audio, harness, winW, winH)` — 內部匿名函式；以閉包鏈建構 LoadingScene 作回傳值，鏈內包含 Title、CharacterSelect、Gameplay 的工廠捕獲。
- `PushInitialScene(SceneManager& sm, Harness& harness, AudioDevice& audio, int winW, int winH)` — 公開入口；依 `harness.Active()` 分支，錄製路徑直接 push 裸 GameplayScene，人類路徑呼叫 `MakeHumanInitialScene` 後 push。

## 相依與在架構中的位置
- **#include（往外）**：`IScene.h`、`SceneManager.h`、`GameplayScene.h`、`TitleScene.h`、`CharacterSelectScene.h`、`LoadingScene.h`（整條場景鏈）；`CharacterSelect.h`（`CharacterSelectResult`）；`Harness.h`、`AudioDevice.h`（程式級資源）
- **被誰使用（往內）**：—（葉節點 / 組裝根，僅被 `main.cpp` 呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：app 層 composition root；與模擬管線無直接關係，僅決定遊戲啟動時推入哪條場景鏈

## OO 概念與設計重點

此檔案是 [MVC](../concepts/arch-mvc.md) 架構的 composition root 之一：它只組裝依賴，不持有任何遊戲狀態。工廠鏈（閉包捕獲）是 [Factory Method](../concepts/pat-factory.md) 精神的應用——每個場景以 `NextFactory` / `GameplayFactory` 抽象表示「下一個場景如何建立」，而非直接依賴具體類別。RAII 設計確保紋理與音訊資源在各自場景結束時自然釋放。錄製路徑的特化分支體現 [Harness](../concepts/arch-harness.md) 架構設計：錄製時完全繞過互動式 UI。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/SceneBootstrap.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/SceneBootstrap.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Factory](../concepts/pat-factory.md) · [Harness](../concepts/arch-harness.md)
