---
id: file:include/game/quest/PipoyaRoster.h
type: header
path: include/game/quest/PipoyaRoster.h
domain: game
bucket: quest
loc: 82
classes: []
sources: ["include/game/quest/PipoyaRoster.h"]
---
# `PipoyaRoster.h`

> **一句定位**：掃描 PIPOYA 32×32 角色素材包並為每個 NPC 生成點確定性地指派多樣 sprite，在素材缺席時優雅降級（no-op）。

## 職責

`PipoyaRoster.h` 解決「校園 NPC 看起來全是同一張臉」的問題：透過掃描可選的 PIPOYA FREE RPG Character Sprites 素材包（目錄在 gitignore 下），為每個生成點挑選多樣的外觀，並保證確定性（同一生成點每次重啟外觀不變）與優雅降級（素材不存在時原封不動回傳後備路徑）。

`PipoyaRoster()` 函式以區域靜態惰性初始化掃描 `resources/assets/PIPOYA FREE RPG Character Sprites 32x32/Male` 與 `Female` 兩個子目錄，收集所有 `.png` 路徑、排序後快取（排序確保跨次執行順序穩定）。首次呼叫時掃描，後續呼叫直接回傳快取向量。

`PickNpcSprite(npcId, pos, fallbackPath)` 以 `npcId` + 整數化後的 `pos.x`/`pos.y` 計算雜湊（以 0x9e3779b9 為混合常數的 XOR 折疊，相當於一種輕量 FNV/boost 風格 hash_combine），再取模到名冊大小，確保同 `(npcId, position)` 對固定對應同一張外觀，不同生成點各異。素材包缺席（名冊為空）時直接回傳 `fallbackPath`。

PIPOYA 素材每張 96×128（3×4 排版），與手工挑選的 NPC 美術格式完全一致，`NPC::LoadSprite` 以同樣切片方式載入，無需更改任何渲染邏輯。

## 關鍵內容（類別 / 函式 / 資料）

- `PipoyaRoster() → const vector<string>&`：掃描 Male/Female 子目錄收集 .png 路徑，排序並惰性快取；素材包不存在回傳空向量。
- `PickNpcSprite(const string& npcId, Vec2 pos, const string& fallbackPath) → string`：hash(npcId, pos.x, pos.y) 對名冊取模，選出確定性 sprite 或退回後備路徑。

## 相依與在架構中的位置

- **#include（往外）**：`Vec2.h`（`pos` 型別）；標準庫 `<algorithm>`、`<cstddef>`、`<filesystem>`、`<functional>`、`<string>`、`<vector>`
- **被誰使用（往內）**：`include/game/vendor/VendorSprite.h`（攤販多樣化 sprite）、`include/game/world/TexturePreload.h`（預載貼圖）、`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（NPC 生成時呼叫 `PickNpcSprite`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：建構期工具函式——在 World 初始化生成 NPC 時呼叫一次，對每幀無影響。

## OO 概念與設計重點

本檔的核心設計模式是**優雅降級（Graceful Degradation）**：整個功能對遊戲正確性沒有硬性要求，素材存在時增添視覺多樣性，素材缺席時完全透明。函式區域靜態惰性初始化（Lazy Initialization）確保目錄掃描只執行一次，且 `std::sort` 後的固定排序消除了檔案系統列舉順序的不確定性，保持確定性 sprite 指派。雜湊合並方式（0x9e3779b9 常數 + 位元移位）是 Boost 中廣泛使用的 `hash_combine` 模式，既輕量又散列效果良好。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/PipoyaRoster.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/PipoyaRoster.h) · [← 全檔索引](../files-index.md)
