---
id: file:include/app/scenes/CharacterSelectScene.h
type: header
path: include/app/scenes/CharacterSelectScene.h
domain: app
bucket: scenes
loc: 45
classes: [CharacterSelectScene]
sources: ["include/app/scenes/CharacterSelectScene.h"]
---
# `CharacterSelectScene.h`

> **一句定位**：五角色選擇畫面的場景實作，讀方向鍵 / Enter 移動確認游標，並以工廠 closure 將選取結果傳入下一個場景（GameplayScene）。

## 職責

`CharacterSelectScene final : public IScene` 是角色選擇畫面，負責讓玩家在五位角色（由 `kPersonas` 提供）中挑選一位，並在確認後發出 `SceneCommand{Replace, gameplayFactory_(selection)}`，以選取的 `CharacterSelectResult` 實體化下一個場景。

每幀 `Update` 讀取方向鍵（`←→` / `AD`）移動 `cursor_`，Enter 鍵確認選取；`Draw` 繪製五格角色與選取面板。透過 `previews_`（`vector<Texture>`）持有各角色的預覽紋理，場景解構時紋理隨之 RAII 釋放。

`PressLatch confirm_` 用以攔下從 `TitleScene` 延續的 Enter 長按，避免確認鍵在場景剛切入時就被誤觸。`Enter()` 負責載入預覽紋理（`previews_`），使紋理與場景的活躍期嚴格綁定。

`GameplayFactory` 是由 composition root（`SceneBootstrap.cpp`）產生的 closure：它捕獲 `GameplayScene` 所需的 `audioDevice / harness / windowSize` 等參考，`CharacterSelectScene` 本身不持有也不知道這些參考的細節——這讓 `CharacterSelectScene` 可以在不依賴 `GameplayScene` 標頭的情況下保持可測試。

## 關鍵內容（類別 / 函式 / 資料）

- **`CharacterSelectScene`**（`final : IScene`）：角色選擇場景。
  - `GameplayFactory`（型別別名）：`std::function<std::unique_ptr<IScene>(nccu::CharacterSelectResult)>`，接收選取結果並建構下一場景的工廠。
  - `CharacterSelectScene(GameplayFactory gameplay)`：建構子，接收工廠 closure。
  - `Enter() override`：載入五個角色的預覽紋理到 `previews_`。
  - `Update(float dt) → SceneCommand override`：處理方向鍵游標移動與 Enter 確認，確認時回傳 `{Replace, gameplayFactory_(selection)}`。
  - `Draw(IRenderer&) override`：繪製五格選取 UI。
  - `gameplay_`（`GameplayFactory`）：gameplay 工廠 closure。
  - `cursor_`（`int`）：目前選取的角色索引（0 到 kPersonas 上限）。
  - `previews_`（`vector<Texture>`）：各角色預覽紋理，Enter() 時載入，解構時 RAII 釋放。
  - `confirm_`（`PressLatch`）：Enter 長按攔截器，防止從 TitleScene 跨場景的誤觸。

## 相依與在架構中的位置

- **#include（往外）**：`include/app/IScene.h`（場景介面）、`include/engine/render/Texture.h`（預覽紋理型別）、`include/ui/CharacterSelect.h`（`CharacterSelectResult` 與 `kPersonas`）、`include/ui/PressLatch.h`（長按攔截）。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`（建構實例）、`src/app/scenes/CharacterSelectScene.cpp`（實作）。
- **繼承 / 實作 / 體現**：繼承並實作 `IScene`（來自 `include/app/IScene.h`）。
- **每幀管線 / MVC 角色**：app 層；位於 Loading → Title → **CharacterSelect** → Gameplay 的場景鏈中，不參與遊戲每幀模擬管線。

## OO 概念與設計重點

`GameplayFactory` closure 是 [Factory Method](../concepts/pat-factory.md) 的 lambda 形式，由 composition root 注入，讓 `CharacterSelectScene` 對 `GameplayScene` 的具體型別保持無知（依賴倒置）。

`PressLatch` 解決跨場景按鍵延續問題，是一種邊緣偵測防抖機制。`previews_` 以 `vector<Texture>` 持有預覽紋理並在場景解構時自動釋放，是 [RAII](../concepts/oo-raii.md) 的應用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/CharacterSelectScene.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/CharacterSelectScene.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md) · [RAII](../concepts/oo-raii.md)
