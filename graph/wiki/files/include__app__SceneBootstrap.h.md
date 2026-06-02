---
id: file:include/app/SceneBootstrap.h
type: header
path: include/app/SceneBootstrap.h
domain: app
bucket: 
loc: 39
classes: []
sources: ["include/app/SceneBootstrap.h"]
---
# `SceneBootstrap.h`

> **一句定位**：composition root 的「畫面流程」組裝點，將測試錄製分支與人類遊玩的場景工廠鏈封裝於單一函式 `PushInitialScene`，讓 `main()` 保持精簡。

## 職責

此標頭聲明單一自由函式 `PushInitialScene(SceneManager&, Harness&, AudioDevice&, int, int)`，由 `main.cpp` 和 `SceneBootstrap.cpp` 共同使用。

`PushInitialScene` 負責決定場景鏈的起點，並推入 `SceneManager`。它封裝了兩條路徑：
- **自動錄製路徑**（`harness.Active()` 為真）：直接建構並推入 `GameplayScene`，略過 Loading → Title → CharacterSelect 的人類互動流程；`RestartFactory` 留空，對應「腳本化執行永不重啟」的合約。
- **人類遊玩路徑**：推入 `LoadingScene`，其內部工廠鏈依序接入 `TitleScene → CharacterSelectScene → GameplayScene`；「重新開始」工廠從全新的 `LoadingScene` 重建整條鏈，使每回合的 World / View / GameController 都經過 RAII 完整重建。

將這段組裝邏輯抽離 `main.cpp` 的好處：`main()` 只需管理視窗、字型、音訊、錄製器的建構 / 拆除（最小 composition root），而整條場景工廠鏈及分支判斷集中在 `SceneBootstrap.cpp`，可以不影響 `main.cpp` 地單獨修改場景鏈。

## 關鍵內容（類別 / 函式 / 資料）

- **`PushInitialScene(SceneManager& sm, Harness& harness, AudioDevice& audio, int winW, int winH) → void`**：讀取 `harness.Active()` 決定分支，建立 closure 鏈並呼叫 `sm.Push(...)` 推入首個場景。此函式不擁有任何資源，借用 `Harness` 和 `AudioDevice` 的參考（生命週期由 `main()` 把關）。

## 相依與在架構中的位置

- **#include（往外）**：無任何 `#include`（純宣告頭，使用前向宣告 `nccu::Harness` / `nccu::audio::AudioDevice`）——刻意不拉進場景類別定義，讓 `main.cpp` 不需直接依賴每個具體場景標頭。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`（實作）、`src/app/main.cpp`（呼叫點）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：app 層組裝根；在 `main.cpp` 的 `Run()` 迴圈「之前」執行一次，不參與每幀管線。

## OO 概念與設計重點

此檔是 composition root 模式（Dependency Injection 的一種）的體現：將所有「知道怎麼組裝整個物件圖」的知識收斂到一處，讓其餘類別保持可測試的無知（`GameplayScene` 不知道誰呼叫了它的建構子）。

`PushInitialScene` 的場景工廠鏈大量使用 lambda closure 層層捕獲，是 [Factory Method](../concepts/pat-factory.md) 的匿名 lambda 版：每個工廠只建構自己負責的場景，並把「下一層工廠」捕捉進去，形成延遲實體化的工廠鏈。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/app/SceneBootstrap.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/SceneBootstrap.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md) · [Harness](../concepts/arch-harness.md)
