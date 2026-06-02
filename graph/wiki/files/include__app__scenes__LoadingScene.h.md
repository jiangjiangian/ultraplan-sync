---
id: file:include/app/scenes/LoadingScene.h
type: header
path: include/app/scenes/LoadingScene.h
domain: app
bucket: scenes
loc: 42
classes: [LoadingScene]
sources: ["include/app/scenes/LoadingScene.h"]
---
# `LoadingScene.h`

> **一句定位**：人類遊玩場景鏈的第一個場景，在 Enter() 暖機紋理快取後，待數幀顯示「載入中…」畫面再切換到 TitleScene。

## 職責

`LoadingScene final : public IScene` 位於人類遊玩路徑的最前端（`main.cpp` → `PushInitialScene` → 此場景）。它的設計目標有兩個：一是讓紋理快取在首個遊玩幀前完成暖機，避免首幀卡頓；二是提供短暫的載入畫面讓玩家感知「進入遊戲中」。

`Enter()` 負責同步暖機紋理快取。`Update` 計數 `frameCount_`，等待若干幀後（讓「載入中」畫面可被察覺）回傳 `SceneCommand{Replace, next_()}`，將控制權交給 `TitleScene`。`Draw` 在等待期間繪製載入畫面，即便只是簡單的清屏，也確保 `DrawScope` 不空白。

`next_` 是 composition root 交付的 `NextSceneFactory` closure（即 `TitleScene` 的工廠，其內部又捕獲 `CharacterSelectScene → GameplayScene` 鏈）。`LoadingScene` 本身不知道後續場景的型別，完全透過 closure 解耦——測試可換入任意 stub。

`frameCount_`（`int`）是幀計數器，唯一的狀態。

## 關鍵內容（類別 / 函式 / 資料）

- **`LoadingScene`**（`final : IScene`）：載入畫面場景。
  - `NextSceneFactory`（型別別名）：`std::function<std::unique_ptr<IScene>()>`，觸發後建構下一個場景。
  - `LoadingScene(NextSceneFactory next)`：接收下一場景工廠 closure。
  - `Enter() override`：同步暖機紋理快取（確保首個遊玩幀不卡頓）。
  - `Update(float dt) → SceneCommand override`：每幀遞增 `frameCount_`，達到門檻後回傳 `{Replace, next_()}`。
  - `Draw(IRenderer&) override`：繪製「載入中…」畫面。
  - `next_`（`NextSceneFactory`）：下一場景工廠 closure。
  - `frameCount_`（`int`，初值 0）：幀計數器，決定停留時長。

## 相依與在架構中的位置

- **#include（往外）**：僅 `include/app/IScene.h` 與標準庫（`<functional>`, `<memory>`）——刻意最小依賴，不引入任何 UI / 紋理 / 遊戲標頭。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`（建構實例作為場景鏈入口）、`src/app/scenes/LoadingScene.cpp`（實作）。
- **繼承 / 實作 / 體現**：繼承並實作 `IScene`。
- **每幀管線 / MVC 角色**：app 層；Loading → Title → CharacterSelect → Gameplay 場景鏈的起點，不參與遊戲每幀模擬。

## OO 概念與設計重點

`NextSceneFactory` closure 是 [Factory Method](../concepts/pat-factory.md) 的 lambda 形式，讓 `LoadingScene` 對後續場景的具體型別一無所知，符合開放封閉原則。

簡單設計的體現：整個場景只有一個狀態變數 `frameCount_`，沒有多餘成員；Enter 暖機 + 短暫停留 + Replace 切換三件事，職責清晰。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/LoadingScene.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/LoadingScene.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md)
