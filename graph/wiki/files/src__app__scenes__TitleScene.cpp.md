---
id: "file:src/app/scenes/TitleScene.cpp"
type: source
path: src/app/scenes/TitleScene.cpp
domain: app
bucket: scenes
loc: 156
classes: [MenuItem]
sources: ["src/app/scenes/TitleScene.cpp"]
---
# `TitleScene.cpp`

> **一句定位**：標題場景實作：三項主選單（開始 / 說明 / 離開）的游標確認邏輯，以及標題內嵌的多頁「遊戲說明」子狀態輸入與繪製。

## 職責

`TitleScene` 持有一個主選單游標（`cursor_`）與一個「是否顯示說明」的子狀態布林（`showingHelp_`），兩者形成互斥的輸入路徑。

主選單（`showingHelp_` 為 false）：方向鍵上下 / W S 移動 `cursor_`；`confirm_.Fired(IsDown(Enter), IsPressed(Enter))` 長按閂確認，依 `MenuAction`（Start / Help / Quit）分派：Start 呼叫 `startGame_` 工廠回傳 Replace；Quit 回傳 Quit；Help 設定 `showingHelp_ = true`（`helpBack_` 在此重建，防止開啟 Help 的那次 Enter 同時關閉 Help）。

說明子狀態（`showingHelp_` 為 true）：左右方向鍵切換 `helpPage_`（`0 ~ kGameHelpPageCount-1` 循環）；E 或 `helpBack_.Fired(Enter)` 返回主選單並重設 `showingHelp_ = false`。

`Draw()` 分兩路：說明模式下呼叫 `nccu::ui::DrawHelpPage`（委託 HelpPageView）；主選單模式下繪製 banner 面板、「尋傘記 / 政大山下篇」標題、三列選單（高亮游標 ` > ` 前綴）與底部鍵盤提示。

## 關鍵內容（類別 / 函式 / 資料）

- `MenuItem` struct（匿名命名空間）— `label`（string_view）、`action`（MenuAction enum）。
- `MenuAction` enum — `Start / Help / Quit`，三個動作分類。
- `kItems` — `constexpr array<MenuItem, 3>`：「開始遊戲」「遊戲說明」「離開」。
- `kHighlight{255,153,0,255}` — 游標高亮橘色；`kPanel{18,20,28,200}` — banner 背景。
- `TitleScene::Update()` — 雙狀態輸入路徑（主選單 / 說明）；長按閂確認（`confirm_` / `helpBack_`）。
- `TitleScene::Draw()` — 說明模式呼叫 `DrawHelpPage`；主選單模式繪製 banner + 選單列 + 底部提示。

## 相依與在架構中的位置
- **#include（往外）**：`TitleScene.h`；`GameHelp.h`（`kGameHelpPageCount`）；`HelpPageView.h`（`DrawHelpPage`、`HelpPageStyle`）；`Input.h` / `Key.h`；`Color.h` / `Rect.h` / `Vec2.h`；`Renderer.h` / `TextBuilder.h`
- **被誰使用（往內）**：—（葉節點；由 SceneBootstrap `titleFactory` closure 建立）
- **繼承 / 實作 / 體現**：實作 `IScene`（Update / Draw）
- **每幀管線 / MVC 角色**：app / scenes 層；互動式標題畫面，不參與遊戲模擬管線，`WorldForHarnessOrNull()` 回傳 nullptr

## OO 概念與設計重點

`MenuItem` 與 `MenuAction` 是匿名命名空間內的局部型別，體現 C++ 的封裝（限制命名空間污染）。長按閂（`confirm_` / `helpBack_`）是防止「同一次 Enter 同時觸發兩個行為」的邊界條件設計：`helpBack_` 於開啟說明的同一幀被「重建」，使其等待那次 Enter 放開後才能觸發。說明子狀態是 [State](../concepts/pat-state.md) 模式的輕量版應用——兩個互斥布林路徑不用繼承即可表達簡單的內部子狀態機。`DrawHelpPage` 委託 `HelpPageView` 分離渲染實作，符合 SRP。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/TitleScene.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/TitleScene.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [RAII](../concepts/oo-raii.md)
