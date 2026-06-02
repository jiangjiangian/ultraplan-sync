---
id: file:include/app/IScene.h
type: header
path: include/app/IScene.h
domain: app
bucket: 
loc: 98
classes: [SceneCommand, IScene]
sources: ["include/app/IScene.h"]
---
# `IScene.h`

> **一句定位**：app 層所有畫面（Loading / Title / CharacterSelect / Gameplay）共用的場景契約，以及場景間切換的指令型別 `SceneCommand`。

## 職責

此標頭定義兩個核心型別，合力構成應用程式的場景切換機制。

`SceneCommand` 是每幀從場景傳回給 `SceneManager` 的指令值。它持有一個 `Kind` 列舉（`None / Replace / Push / Pop / Quit / Restart`）和一個工廠 thunk `std::function<std::unique_ptr<IScene>()> make`，以 closure 形式延後建構下一個場景。這個設計使具體場景類別的定義不必出現在 `IScene.h` 中，既避免循環 `#include`，也讓測試可以直接傳入 stub closure 而無須依賴正式場景實作。

`IScene` 是場景介面，規定了四個生命週期鉤子：`Enter()`（掛載時一次性接線，預設空實作）、純虛擬的 `Update(float dt)` 回傳 `SceneCommand`（讀輸入並推進模型）、純虛擬的 `Draw(IRenderer&)` 輸出畫面，以及 `Exit()`（解掛時清理，預設空實作）。`SceneManager` 在「該幀 Draw 之後」才套用 `Update` 回傳的指令，嚴格保障「絕不在幀中途抽換現役場景」的不變式。

`WorldForHarnessOrNull()` 是為自動遊玩錄製機制（`Harness::EndFrame`）開的接縫：唯有 `GameplayScene` 覆寫以回傳其 `World*`，其餘場景回傳 `nullptr`，`SceneManager::Run` 據此略過這些非遊玩幀的 `EndFrame` 呼叫。

輸入來源刻意不出現在介面上——所有場景透過程序內統一的 `nccu::engine::input::Input` seam 讀取按鍵，測試可呼叫 `Input::SetSource` 替換，而無需改動 `IScene` 的對外形貌。

## 關鍵內容（類別 / 函式 / 資料）

- **`SceneCommand`**：場景切換指令值型別。
  - `Kind`（enum）：`None / Replace / Push / Pop / Quit / Restart`，涵蓋堆疊所有操作語意。
  - `make`（`std::function<std::unique_ptr<IScene>()>`）：延後工廠 thunk，按需建立下一個場景。
- **`IScene`**：抽象場景介面。
  - `Enter()`：非純虛擬，掛載時一次性接線（訂閱事件等）。
  - `Update(float dt) → SceneCommand`：純虛擬，每幀推進模型並回傳切換指令。
  - `Draw(IRenderer&)`：純虛擬，在 `DrawScope` 的 `BeginDrawing / EndDrawing` 區間內輸出。
  - `Exit()`：非純虛擬，取消訂閱與清理。
  - `WorldForHarnessOrNull() const noexcept → const World*`：錄製接縫；僅 `GameplayScene` 覆寫以回傳非 null。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<functional>`, `<memory>`）；以前向宣告引入 `IRenderer` 和 `World`，不拉進任何 raylib 或遊戲層標頭——維持 app 層對 engine/game 的單向依賴。
- **被誰使用（往內）**：`SceneManager.h`、四個具體場景標頭（`CharacterSelectScene / GameplayScene / LoadingScene / TitleScene`）、`SceneBootstrap.cpp`。
- **繼承 / 實作 / 體現**：被 `CharacterSelectScene / GameplayScene / LoadingScene / TitleScene` 繼承實作。
- **每幀管線 / MVC 角色**：屬 app 層，為 `SceneManager::Run` 迴圈的介面合約。非 Model / View / Controller 任一，而是其組裝框架的協定邊界。

## OO 概念與設計重點

`IScene` 體現了介面（純虛擬）+ 非純虛擬預設實作（`Enter / Exit`）的混合設計，讓只需部分覆寫的葉場景不必提供空殼。

`SceneCommand::make` 採用 thunk closure，是 [Factory Method](../concepts/pat-factory.md) 的 lambda 變體：具體場景的構造被封裝在 closure 內，`IScene.h` 完全不知道 `CharacterSelectScene` 等型別的存在，實現了開放封閉原則（OCP）——新增場景型別無需修改本標頭。

[RAII](../concepts/oo-raii.md) 依賴場景的 `Enter / Exit` 明確接線與取消訂閱，而非仰賴建構 / 解構子——因為場景可能被推入堆疊後暫時不活躍（Push 語意），生命週期與活躍期不同。`WorldForHarnessOrNull` 接縫使錄製機制不需要下轉型（no `dynamic_cast`），符合 [DIP](../concepts/arch-dip-renderer.md) 精神。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/app/IScene.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/IScene.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md) · [RAII](../concepts/oo-raii.md)
