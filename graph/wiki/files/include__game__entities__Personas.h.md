---
id: file:include/game/entities/Personas.h
type: header
path: include/game/entities/Personas.h
domain: game
bucket: entities
loc: 70
classes: [Persona, CharacterSelectResult]
sources: ["include/game/entities/Personas.h"]
---
# `Personas.h`

> **一句定位**：角色人設的領域資料，定義五個校園人設的 `Persona` 表與每場的選擇結果 `CharacterSelectResult`，屬 Model 而非 UI 呈現。

## 職責

此檔從 `ui/CharacterSelect.h` 中抽離角色人設的領域資料，使其可被 `TexturePreload`（資源預熱）、`GameplayScene`（套用人設色調）、及自動播放略過路徑等非 UI 程式碼使用，而不必引入整個 UI 標頭。

`kPersonas` 是一個 `inline constexpr std::array<Persona, 5>`，在編譯期固定五個人設（索引 0..4），確保自動播放可以確定性地由環境變數解析出 sprite 路徑與色調，而不需執行 UI 選擇器。

五個人設（夜貓子、social咖、邊緣人、卷王、佛系生）對應 `resources/assets/sprites/school_uniform_3/` 下既有的 Pipoya 圖集，以 `DrawTexturePro` 的色彩 tint（raylib 5.5 colour-modulate）在執行期拉開視覺差異，無須新增任何二進位檔。

`CharacterSelectResult` 是 `CharacterSelectScene` 確認後的傳遞值，攜帶 `spritePath`（sprite 資源路徑）、`tint`（色調，預設白 = 不調色）與 `closed`（是否取消/關閉）。

## 關鍵內容（類別 / 函式 / 資料）

- **`struct Persona`**：`label`（CJK 名稱）、`blurb`（風味說明）、`spritePath`（Pipoya 圖集路徑）、`tint`（RGBA 色調）。
- **`inline constexpr std::array<Persona, 5> kPersonas`**：五個人設的編譯期固定表，順序即選單順序（索引 0..4）。五個人設的色調分別為冷調靛藍（夜貓子）、暖橙（social咖）、柔和綠（邊緣人）、玫瑰紅（卷王）、琥珀（佛系生）。
- **`struct CharacterSelectResult`**：`spritePath`（所選 sprite 路徑）、`tint`（所選色調，預設白）、`closed`（取消旗標）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Color.h`（`Persona::tint` 與 `CharacterSelectResult::tint` 的型別）；`<array>`、`<string>`、`<string_view>`（STL 容器與字串）。
- **被誰使用（往內）**：`include/game/world/TexturePreload.h`（資源預熱讀取 spritePath）、`include/ui/CharacterSelect.h`（UI 層透過傳遞性 include 再匯出以維持原始碼相容）。
- **繼承 / 實作 / 體現**：純資料結構，無繼承。
- **每幀管線 / MVC 角色**：Model 層的常數資料。`CharacterSelectScene` 生成 `CharacterSelectResult` 並傳給 `GameplayScene`；後者設定 `Player::SetTint(result.tint)` 與載入 `result.spritePath` sprite。

## OO 概念與設計重點

本檔體現「領域資料歸 Model，視覺呈現歸 View」的 [MVC](../concepts/arch-mvc.md) 架構紅線。將人設資料從 UI 標頭中抽離，避免 `TexturePreload` 等非 UI 程式碼必須引入整個 `ui/CharacterSelect.h`，降低編譯相依。

`inline constexpr` 的人設表讓自動播放略過路徑能在編譯期確定性地解析人設，無須執行 UI 選擇器，是「測試友善」設計的體現（harness 可確定性重播）。五個人設共用同一張 Pipoya 底圖加色調調變，是「資產效率」（無新二進位檔）與「視覺差異化」的平衡方案。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/Personas.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Personas.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Harness](../concepts/arch-harness.md)
