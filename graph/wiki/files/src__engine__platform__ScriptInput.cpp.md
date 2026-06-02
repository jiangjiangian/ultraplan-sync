---
id: "file:src/engine/platform/ScriptInput.cpp"
type: source
path: src/engine/platform/ScriptInput.cpp
domain: engine
bucket: platform
loc: 171
classes: []
sources: ["src/engine/platform/ScriptInput.cpp"]
---
# `ScriptInput.cpp`

> **一句定位**：腳本輸入的解析與傳統「定時指令」執行：載入 `.script` 檔案（高階動詞與 `<幀號> verb key` 定時指令混合），每幀 `Advance()` 套用合成按鍵邊緣。

## 職責

此檔案實作 `ScriptInput` 的解析與定時指令執行部分（高階動詞計畫解析 `ResolvePlan` 單獨在 `ScriptResolver.cpp`）。

`Load(istream&)` 逐行解析腳本：以 `LooksLikeVerb(line)` 區分「高階動詞行」（第一個非空白字元非數字且非 `-`）與傳統「`<幀號> verb key`」行。高階動詞行解析 `goto`、`interact`（兩種形式：純 NpcId 或 `label x y` 座標）、`choose`、`advance`、`wait`、`quit` 等，加入 `plan_` 向量。傳統行解析 `down`、`up`、`press` 動作及 `KeyCode` 映射，存入 `byFrame_[frame]`。

`LoadFile(path)` 開啟檔案後委託 `Load`。

`Advance()` 每幀呼叫：遞增 `frame_`、清除 `pressed_` / `released_`、從 `autoUp_` 放開上一幀的 press 按鍵、套用 `byFrame_[frame_]` 的本幀指令（`SynthDown/SynthUp/SynthPress`）。

`SynthDown(key)` — 加入 `down_`，若新插入則加 `pressed_`。
`SynthUp(key)` — 從 `down_` 移除，若成功則加 `released_`。
`SynthPress(key)` — 加入 `down_` + `pressed_` + `autoUp_`（下幀自動放開）。

`IsDown / IsPressed / IsReleased` 查詢對應集合。

`KeyCode(string_view)` — 將腳本符記（單字母 A-Z、Enter、Escape、Tab、Space、Backspace、Up/Down/Left/Right）映射到 raylib 鍵碼。

## 關鍵內容（類別 / 函式 / 資料）

- `LooksLikeVerb(line)` — 判定行是否為高階動詞（非數字開頭、非註解）。
- `KeyCode(string_view)` — 腳本符記 → raylib 鍵碼映射。
- `ScriptInput::Load(istream&)` — 解析高階動詞行（加入 `plan_`）與傳統定時行（加入 `byFrame_`）。
- `ScriptInput::Advance()` — 每幀套用 `byFrame_[frame_]` 指令；清除並重建按鍵狀態集合。
- `SynthDown / SynthUp / SynthPress` — 合成按鍵邊緣的三個輔助（傳統與計畫共用）。
- `IsDown / IsPressed / IsReleased` — 查詢 `down_` / `pressed_` / `released_` 集合。

## 相依與在架構中的位置
- **#include（往外）**：`ScriptInput.h`；`World.h`、`Player.h`、`GameObject.h`、`DialogState.h`（`ResolvePlan` 共用，此 TU 也納入）；`Key.h`（`ToRaylibKey`）；`Vec2.h`、`Rect.h`；raylib
- **被誰使用（往內）**：—（由 `Harness.cpp` 建立並持有）
- **繼承 / 實作 / 體現**：實作 `InputSource` 介面（`IsDown / IsPressed / IsReleased`）
- **每幀管線 / MVC 角色**：engine / platform 層；`Advance()` 在 `Harness::BeginFrame()` 內、在 `Input::SetSource` 介入後、讀取輸入前執行；屬於 [Harness](../concepts/arch-harness.md) 架構

## OO 概念與設計重點

`ScriptInput` 實作 [Strategy](../concepts/pat-strategy.md) 模式（`InputSource` 介面），讓遊戲的輸入讀取點（`Input::IsDown/IsPressed`）完全不知道來源是人類鍵盤還是腳本，實現逐位元重播。`autoUp_` 的設計確保 `SynthPress` 的單幀按鍵語意與 raylib 真實鍵盤一致（按下後自動放開），防止「按住 press 鍵」的副作用。「高階動詞行 / 定時行」的共存格式（純增添、永不歧義）是設計刻意的向後相容保證——既有腳本完全不受高階動詞加入影響。`ResolvePlan` 分拆到 `ScriptResolver.cpp` 減少此 TU 的行數，屬於大型 TU 的 SRP 拆分。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/platform/ScriptInput.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/ScriptInput.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [Harness](../concepts/arch-harness.md)
