---
id: "file:include/engine/render/Window.h"
type: header
path: include/engine/render/Window.h
domain: engine
bucket: render
loc: 76
classes: [Window, Builder]
sources: ["include/engine/render/Window.h"]
---
# `Window.h`

> **一句定位**：raylib 視窗的 move-only RAII 控制代碼——以內嵌 `Builder` 流暢設定標題/尺寸/FPS 後 `Open()`，解構時自動 `::CloseWindow()`。

## 職責

`Window.h` 在 engine render 層提供 raylib 視窗生命週期的 RAII 管理。它解決的問題是：`::InitWindow` 和 `::CloseWindow` 這對 C API 呼叫若配對錯誤會導致程序崩潰，將其封裝在帶有 move-only 所有權語意的類別中，確保視窗生命週期由單一擁有者掌控。

`Window::Builder` 是一個內嵌的建構器，以流暢介面設定 `Title`、`Size`（寬高）、`Fps` 後呼叫 `Open()` 建立視窗並回傳 `Window` 實例。`Open()` 做了一個重要的初始化決策：呼叫 `::SetExitKey(KEY_NULL)` 停用 raylib 預設的 ESC 離開鍵——因為本遊戲的「離開」流程必須透過首頁或暫停選單來走，不能讓 ESC 繞過。這個設計決策以注解明確記錄在程式碼中。

`Window` 以 `owns_` 旗標追蹤是否持有視窗，支援 move（轉移所有權後舊物件 `owns_=false`），禁止拷貝，解構時若仍持有則呼叫 `::CloseWindow()`。`ShouldClose()` 包裝 `::WindowShouldClose()`，供主迴圈輪詢關閉訊號（關閉鈕被按，而非 ESC）。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class Window` | Move-only RAII 視窗控制代碼；`owns_` bool；禁止拷貝；`~Window` 條件式 `::CloseWindow`。 |
| `class Window::Builder` | 流暢設定 `title_`/`width_`/`height_`/`fps_`；`Open()` 呼叫 `::InitWindow`+`::SetExitKey(KEY_NULL)`+`::SetTargetFPS`，回傳 `Window{true}`。 |
| `Builder::Title(string)` | 設定視窗標題；預設 `"Window"`。 |
| `Builder::Size(int w, int h)` | 設定視窗尺寸；預設 800×450。 |
| `Builder::Fps(int f)` | 設定目標 FPS；預設 60；`fps_<=0` 時跳過 `::SetTargetFPS`。 |
| `Window::ShouldClose()` | 回傳 `::WindowShouldClose()`；供主迴圈判斷是否終止。 |

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（直接呼叫 `::InitWindow`/`::CloseWindow`/`::SetExitKey`/`::SetTargetFPS`/`::WindowShouldClose`）、標準庫 `<string>`、`<utility>`。
- **被誰使用（往內）**：`src/app/SceneManager.cpp`（持有 `Window`，驅動主迴圈）、`src/app/main.cpp`（建立並傳遞 `Window`）。
- **繼承 / 實作 / 體現**：—（RAII 慣用法；無繼承）。
- **每幀管線 / MVC 角色**：Engine 基礎設施；不參與每幀管線。在 `main.cpp` 組裝階段建立，視窗的 `ShouldClose()` 驅動最外層的主迴圈。

## OO 概念與設計重點

`Window` 體現 [RAII（oo-raii）](../concepts/oo-raii.md)：`::InitWindow`/`::CloseWindow` 這對資源的取得/釋放封裝在物件生命週期中，解構自動釋放，消除了手動配對呼叫的風險。

`Window::Builder` 是**Builder 模式**的典型應用：把多個可選的視窗配置（標題、尺寸、FPS）收進建構器，解耦配置組裝與物件建立，讓 `main.cpp` 的初始化程式碼可讀性極高。

`::SetExitKey(KEY_NULL)` 這個初始化決策硬編碼在 `Builder::Open()` 中，體現了「在正確的時間點做正確的初始化」的設計哲學——視窗一旦開啟就不應允許 ESC 直接終止，這個約束在建立視窗的那一刻就被強制執行，無法被呼叫端繞過。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/Window.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Window.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
