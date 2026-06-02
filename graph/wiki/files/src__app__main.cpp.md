---
id: "file:src/app/main.cpp"
type: source
path: src/app/main.cpp
domain: app
bucket: ""
loc: 84
classes: []
sources: ["src/app/main.cpp"]
---
# `main.cpp`

> **一句定位**：整個程式的 composition root：開窗、初始化共用資源、組裝場景鏈後交給 `SceneManager::Run`，再有序拆除。

## 職責

`main()` 極度精簡，只做四件事：（1）建立 800×450 的 raylib 視窗（`Window::Builder`，`SetExitKey(KEY_NULL)` 讓 ESC 惰性）；（2）初始化共用資源（工作目錄正規化 `EnsureAssetWorkingDir`、中日韓字型 `EnsureFont`、`AudioDevice` RAII 物件）；（3）用 `MaybeAttach()` 建立 harness（預設關閉，由環境變數 `UMBRELLA_SCRIPT` 啟用）；（4）呼叫 `SceneBootstrap::PushInitialScene` 組裝場景後執行 `SceneManager::Run`。

拆除次序精心排列：`SetSink(nullptr)` 先卸下事件發布接縫，再呼叫 `ShutdownTextureCache()` / `ShutdownFont()` 釋放 GPU 資源，`Window` 解構最後才執行 `::CloseWindow()`，確保釋放 GPU 物件時 GL context 仍有效。

視窗固定 800×450、60 fps；`constexpr int kWinW = 800, kWinH = 450` 傳遞給 SceneBootstrap，使場景鏈中的 UI 元件能正確置中。title 字串為 "Lost Umbrella - MVP"。

## 關鍵內容（類別 / 函式 / 資料）

- `main()` — 組裝 `Window`、`EnsureFont`、`AudioDevice`、`Harness`、`SceneManager`、`RaylibRenderer`，呼叫 `PushInitialScene` 再 `Run`，最後有序拆除。
- `kWinW = 800, kWinH = 450` — 全域視窗尺寸常數，貫穿場景鏈。
- `EnsureAssetWorkingDir()` — 正規化工作目錄，確保 `resources/` 相對路徑在任何啟動方式下可解析。
- `EnsureFont()` — 於 GL context 存在後立即載入中日韓字型，避免 ASCII 後備字型的問題。
- `ShutdownTextureCache()` / `ShutdownFont()` — 在 Window 解構前釋放 GPU 資源。
- `SetSink(nullptr)` — 卸下實體層發布接縫，確保重啟循環（下次若有）能重新綁定。

## 相依與在架構中的位置
- **#include（往外）**：`SceneManager.h`、`SceneBootstrap.h`、`Harness.h`（MaybeAttach）、`WorkingDir.h`、`Window.h`、`Font.h`、`RaylibRenderer.h`、`Texture.h`（ShutdownTextureCache）、`AudioDevice.h`、`EventSink.h`（SetSink）
- **被誰使用（往內）**：—（葉節點 / 程式進入點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：程式入口 / composition root；`SceneManager::Run` 才是主迴圈，main 本身不參與每幀邏輯

## OO 概念與設計重點

此檔案是 [MVC](../concepts/arch-mvc.md) 架構的最頂層 composition root，體現 DIP：所有依賴（Renderer、Harness、AudioDevice）都在此注入，子系統不反向依賴 main。RAII 貫穿整個資源生命期（`Window`、`AudioDevice`、`Harness` 皆以值持有）。拆除次序的精心設計（先 Sink → 紋理/字型 → Window）防止 GPU 資源在 GL context 消失後存取，是 C++ RAII 複合物件解構次序意識的具體示範。`SetExitKey(KEY_NULL)` 讓 ESC 受遊戲邏輯控制而非 raylib 預設行為，是邊界條件設計。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/main.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/main.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [RAII](../concepts/oo-raii.md) · [Harness](../concepts/arch-harness.md)
