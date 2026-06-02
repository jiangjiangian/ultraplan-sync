---
id: "file:src/app/scenes/CharacterSelectScene.cpp"
type: source
path: src/app/scenes/CharacterSelectScene.cpp
domain: app
bucket: scenes
loc: 144
classes: []
sources: ["src/app/scenes/CharacterSelectScene.cpp"]
---
# `CharacterSelectScene.cpp`

> **一句定位**：角色選擇場景的完整實作：載入五個角色的預覽紋理、處理左右游標與 Enter 確認，並繪製角色格與選取資訊面板。

## 職責

`CharacterSelectScene` 是 `IScene` 的具體子類別，處理玩家選擇五位政大角色（`kPersonas`）的互動流程。

`Enter()` 載入所有角色的 Pipoya 格式精靈紋理（32×32 一格；取待機欄 `kIdleCol=1`、朝下列 `kDownRow=0`），以 `vector::reserve` + `push_back` 建構 `previews_`（`Texture` 為 move-only，需此方式）。

`Update()` 以 `IsPressed(Key::Right/Left/D/A)` 移動 `cursor_`（循環取模），以 `confirm_.Fired(IsDown(Enter), IsPressed(Enter))` 長按閂確認選擇，確認後組裝 `CharacterSelectResult`（`spritePath`、`tint`、`closed=false`）並回傳 `SceneCommand::Replace`，thunk 以捕獲的 `gameplay_` 工廠建立 `GameplayScene`。

`Draw()` 繪製角色格（96×96 縮放、22 px 間距）、高亮邊框（`kHighlight` 橘色、3 px）、選取角色說明面板（`kPanel` 深色底色）與底部鍵盤提示。

## 關鍵內容（類別 / 函式 / 資料）

- `kWinW=800, kWinH=450, kFrameSize=32, kIdleCol=1, kDownRow=0` — 視窗與精靈採樣常數。
- `kTile=96, kGap=22` — 角色格大小與間距；`kRowW / kRowX / kRowY` 計算橫列置中位置。
- `kHighlight{255,153,0,255}` — 游標高亮橘色；`kPanel{20,22,30,210}` — 說明面板深色背景。
- `IdleSrc()` — 計算待機精靈的來源 Rect（`kIdleCol * kFrameSize`，`kDownRow * kFrameSize`）。
- `CharacterSelectScene::Enter()` — 載入 `kCount` 份預覽紋理，RAII 自動於場景解構時釋放。
- `CharacterSelectScene::Update(float)` — 游標移動 + `confirm_.Fired(...)` 確認邏輯，確認後回傳 Replace 指令。
- `CharacterSelectScene::Draw(IRenderer&)` — 繪製角色列、高亮邊框、說明面板（`label`、`blurb`）、底部提示。

## 相依與在架構中的位置
- **#include（往外）**：`CharacterSelectScene.h`；`Input.h` / `Key.h`（鍵盤輸入）；`Color.h` / `Rect.h` / `Vec2.h`（數學）；`Renderer.h` / `TextBuilder.h`（渲染）
- **被誰使用（往內）**：—（葉節點；由 `SceneBootstrap.cpp` 的工廠建立）
- **繼承 / 實作 / 體現**：實作 `IScene` 介面（Enter / Update / Draw）
- **每幀管線 / MVC 角色**：app / scenes 層；互動式 UI 場景，不在遊戲模擬管線中

## OO 概念與設計重點

此場景示範 [Factory Method](../concepts/pat-factory.md) 的場景鏈用法：`gameplay_` 是注入的工廠，場景確認後不直接建立 `GameplayScene`，而是呼叫工廠並以 thunk 回傳 `SceneCommand`，延後套用由 `SceneManager` 執行。紋理以 move-only RAII 管理，[RAII](../concepts/oo-raii.md) 確保 `previews_` 在 SceneManager 以 Replace 拆除本場景時（GL context 仍存活的邊界保證）正確釋放。`confirm_.Fired()` 長按閂防止同一次 Enter 鍵在「確認選角」與「進入遊玩」兩個場景各觸發一次。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/CharacterSelectScene.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/CharacterSelectScene.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Factory](../concepts/pat-factory.md) · [RAII](../concepts/oo-raii.md)
