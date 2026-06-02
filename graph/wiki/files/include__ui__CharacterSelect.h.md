---
id: file:include/ui/CharacterSelect.h
type: header
path: include/ui/CharacterSelect.h
domain: ui
bucket: 
loc: 15
classes: []
sources: ["include/ui/CharacterSelect.h"]
---
# `CharacterSelect.h`

> **一句定位**：`Persona`、`kPersonas`、`CharacterSelectResult` 等型別的相容性轉出標頭——實體定義已移至 `game/entities/Personas.h`，本標頭僅做遞移 include 維持舊呼叫端的原始碼相容性。

## 職責

本標頭的功能極為單純：一行 `#include "game/entities/Personas.h"`，把角色選擇相關的型別（`Persona`、`kPersonas` 陣列、`CharacterSelectResult`）從 game 領域的模型標頭轉出，供仍沿用 `ui/CharacterSelect.h` 路徑的場景與測試繼續編譯。

存在的理由是歷史重構：這些型別屬於遊戲領域的模型資料（與 UI 無關），被從 `ui/CharacterSelect.h` 遷移至 `game/entities/Personas.h`。舊路徑的消費端（`CharacterSelectScene.h`、`GameplayScene.h`、`SceneBootstrap.cpp`）在遷移後不需立即修改 include 路徑，讓重構可以分批進行。注解也明確告知新程式碼應直接引入 `game/entities/Personas.h`。

## 關鍵內容（類別 / 函式 / 資料）

- `#include "game/entities/Personas.h"`：唯一的實質內容，把 `Persona` struct、`kPersonas` constexpr 陣列與 `CharacterSelectResult` enum 轉出給包含本標頭的編譯單元。

## 相依與在架構中的位置

- **#include（往外）**：`game/entities/Personas.h`（型別的實體定義）。
- **被誰使用（往內）**：`include/app/scenes/CharacterSelectScene.h`（選角畫面）；`include/app/scenes/GameplayScene.h`（遊戲場景，傳入 persona 給 World）；`src/app/SceneBootstrap.cpp`（場景啟動組裝）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純相容性轉出標頭，無任何邏輯）。

## OO 概念與設計重點

本檔是一個典型的 **相容性轉出（compatibility shim）** 標頭，是大型 C++ 重構中常見的過渡手段。它體現了 **開放封閉原則（OCP）** 在重構中的應用：型別遷移後不需立即修改所有消費端，而是透過薄薄的轉出層讓兩者並存，舊消費端繼續編譯直到有機會逐步遷移。注解中「新程式碼應直接 include game/entities/Personas.h」的說明是明確的技術債標記。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/CharacterSelect.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/CharacterSelect.h) · [← 全檔索引](../files-index.md)
