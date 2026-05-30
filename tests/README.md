# tests/ — 單元測試套件（doctest）

《尋傘記：政大山下篇》以 [doctest](https://github.com/doctest/doctest) 撰寫單元測試。
測試聚焦於純資料與邏輯層（狀態機、任務旗標、對白解析、碰撞、攤販、UI model…），
不依賴開窗或 GPU；圖形相依部分透過 `engine/platform` 的 Harness / ScriptInput 隔離。

本樹共 **112** 個 `.cpp` 檔，合計 **571** 個 `TEST_CASE`（另有 `fixtures/` 放共用測試資料，非測試案例）。
測試樹依與原始碼相同的 domain 子目錄組織。

## 如何執行

CMake 透過 `FetchContent` 取得 doctest，並以 `add_test(NAME unit_tests …)` 註冊到 CTest：

```bash
cmake -B build && cmake --build build       # 編譯遊戲與測試
ctest --test-dir build                       # 經由 CTest 執行（目標名 unit_tests）
ctest --test-dir build --output-on-failure   # 失敗時印出輸出
./build/OOP_Raylib_Lab_tests            # 或直接執行 doctest 二進位
./build/OOP_Raylib_Lab_tests -tc="*flag*"   # 直接執行並用 doctest 旗標篩選案例
```

## 各 domain 子目錄涵蓋範圍

| 子目錄 | 檔數 | TEST_CASE | 涵蓋內容 |
| --- | --- | --- | --- |
| `controller/` | 9 | 46 | GameController / InputHandler / SceneRouter / EventWiring / 工廠與 screens 接線 |
| `dialog/` | 10 | 56 | DialogLoader 的 Markdown 解析、對白分支、Opener / Source / Repository 行為 |
| `entities/` | 14 | 71 | Player / NPC / Item / 雨傘家族 / 消耗品 / 拾取物 的狀態與互動 |
| `gfx/` | 10 | 52 | 繪製輔助、版面與座標換算等純計算邏輯 |
| `harness/` | 3 | 11 | Harness / ScriptInput 自動遊玩感知層 |
| `quest/` | 31 | 132 | 章節任務、spawn、flags、objective、QuestHookTable（測試量最大） |
| `state/` | 6 | 35 | SemesterStateMachine 轉移、章節脊、EndingGate（**四結局** Ending_A/B/C/D） |
| `ui/` | 20 | 121 | View / HUD / overlay / 選單與各視圖 model |
| `vendor/` | 5 | 19 | 攤販 Vendor 設定、定價與訊息 |
| `world/` | 4 | 28 | World / Physics / CollisionMask / BuildingTracker |
| `fixtures/` | 1 | 0 | 共用測試資料（非 `TEST_CASE`） |

> 數字由實際樹推導；如新增測試，重新統計：
> `grep -rc TEST_CASE tests | awk -F: '{s+=$2} END{print s}'`。
