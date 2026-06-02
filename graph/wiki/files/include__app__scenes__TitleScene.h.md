---
id: file:include/app/scenes/TitleScene.h
type: header
path: include/app/scenes/TitleScene.h
domain: app
bucket: scenes
loc: 50
classes: [TitleScene]
sources: ["include/app/scenes/TitleScene.h"]
---
# `TitleScene.h`

> **一句定位**：標題畫面場景，提供「開始遊戲 / 遊戲說明 / 離開」三項主選單，並在確認開始時透過工廠 closure 切換到下一個場景。

## 職責

`TitleScene final : public IScene` 是玩家看到的第一個互動畫面。它負責三件事：顯示主選單（「開始遊戲 / 遊戲說明 / 離開」）、顯示內建的分頁式遊戲說明（`showingHelp_` 旗標控制），以及在確認「開始遊戲」後發出 `SceneCommand{Replace, startGame_()}`。

「遊戲說明」刻意維持為標題內部的子頁面（`showingHelp_ = true` + `helpPage_` 分頁），不跨出場景邊界成為獨立場景——這樣確認開始遊戲和離開才是唯一的場景邊界事件。

`cursor_` 記錄目前選取的選單項目；`PressLatch confirm_` 攔下從 `LoadingScene` 延續的 Enter 長按；`PressLatch helpBack_` 攔下從選單進入說明頁時延續的 Enter 長按，防止說明頁立刻被誤觸確認。

`startGame_`（`NextSceneFactory` closure）由 composition root 提供，封裝了 `CharacterSelectScene` 的建構介面（包含它所需的捕獲參考）——`TitleScene` 不知道 `CharacterSelectScene` 的任何細節，可獨立測試。

## 關鍵內容（類別 / 函式 / 資料）

- **`TitleScene`**（`final : IScene`）：標題畫面場景。
  - `NextSceneFactory`（型別別名）：`std::function<std::unique_ptr<IScene>()>`，確認開始遊戲後建構下一個場景。
  - `TitleScene(NextSceneFactory startGame)`：接收「開始遊戲」工廠 closure。
  - `Update(float dt) → SceneCommand override`：讀方向鍵 / Enter / Escape，更新 `cursor_` / `showingHelp_` / `helpPage_`；確認「離開」回傳 `{Quit}`；確認「開始遊戲」回傳 `{Replace, startGame_()}`。
  - `Draw(IRenderer&) override`：視 `showingHelp_` 繪製主選單或說明頁。
  - `startGame_`（`NextSceneFactory`）：「開始遊戲」工廠 closure。
  - `cursor_`（`int`，初值 0）：目前選單游標（0..kCount-1）。
  - `showingHelp_`（`bool`，初值 false）：是否正顯示「遊戲說明」子頁面。
  - `helpPage_`（`int`，初值 0）：說明頁的目前頁碼（分頁式）。
  - `confirm_`（`PressLatch`）：Enter 攔截，防止從前一畫面延續的長按。
  - `helpBack_`（`PressLatch`）：從選單進入說明頁時延續 Enter 的攔截器。

## 相依與在架構中的位置

- **#include（往外）**：`include/app/IScene.h`（場景介面）、`include/ui/PressLatch.h`（長按攔截）；標準庫 `<functional>`, `<memory>`。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`（由工廠鏈建構）、`src/app/scenes/TitleScene.cpp`（實作）。
- **繼承 / 實作 / 體現**：繼承並實作 `IScene`。
- **每幀管線 / MVC 角色**：app 層；Loading → **Title** → CharacterSelect → Gameplay 場景鏈的第二站，不參與遊戲每幀模擬管線。

## OO 概念與設計重點

`NextSceneFactory` closure 是 [Factory Method](../concepts/pat-factory.md) 的 lambda 形式，使 `TitleScene` 對 `CharacterSelectScene` 保持無知。兩個 `PressLatch` 解決跨場景鍵盤邊緣偵測問題，是實務上常見的 edge detection debounce。`showingHelp_` 子狀態旗標是 `TitleScene` 內的輕量有限狀態，避免引入獨立場景類別，保持了場景邊界的清晰。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/TitleScene.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/TitleScene.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md)
