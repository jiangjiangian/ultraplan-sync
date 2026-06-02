---
id: file:tests/README.md
type: doc
path: tests/README.md
domain: tests
bucket: 
loc: 59
classes: []
sources: ["tests/README.md"]
---
# `README.md`（tests/ 套件說明）

> **一句定位**：doctest 單元測試套件的總覽——112 個 `.cpp`、571 個 `TEST_CASE`，依 domain 子目錄組織。

## 職責
`tests/` 樹的門面文件，說明測試以 [doctest](https://github.com/doctest/doctest) 撰寫，聚焦純資料與邏輯層
（狀態機、任務旗標、對白解析、碰撞、攤販、UI model…），不依賴開窗或 GPU；圖形相依透過
`engine/platform` 的 Harness / ScriptInput 隔離。測試樹依與原始碼相同的 domain 子目錄組織。

## 內容（這份說明涵蓋什麼）
- 目錄樹與各 domain 子目錄的檔數 / `TEST_CASE` 計數總表：controller(9/46)、dialog(10/56)、entities(14/71)、
  gfx(10/52)、harness(3/11)、quest(31/132，量最大)、state(6/35，含四結局 EndingGate)、ui(20/121)、
  vendor(5/19)、world(4/28)、fixtures(1/0，共用測試資料非案例)。
- 如何執行：`cmake -B build && cmake --build build` 後以 `ctest --test-dir build`（目標 `unit_tests`）或
  直接跑 `./build/umbrella_test`（可用 `-tc="*flag*"` 篩選案例）。
- 重新統計指令：`grep -rc TEST_CASE tests | awk -F: '{s+=$2} END{print s}'`。

## 相依與在架構中的位置
純文件，無程式相依。是整個 `tests/` domain 的索引；與 [`include/`](include__README.md.md) / [`src/`](src__README.md.md) 樹說明同層級。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/README.md) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/README.md) · [← 全檔索引](../files-index.md) · 相關：[ISystem 管線](../concepts/arch-isystem.md) · [決定性 Harness](../concepts/arch-harness.md)
