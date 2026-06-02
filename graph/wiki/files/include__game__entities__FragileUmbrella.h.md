---
id: file:include/game/entities/FragileUmbrella.h
type: header
path: include/game/entities/FragileUmbrella.h
domain: game
bucket: entities
loc: 38
classes: [FragileUmbrella]
sources: ["include/game/entities/FragileUmbrella.h"]
---
# `FragileUmbrella.h`

> **一句定位**：雨傘 Template Method 樹的「破傘」葉類別，`BeClaimed` 設立持傘狀態，並持有漏雨速率供雨量系統讀取。

## 職責

`FragileUmbrella` 是廉價、瀕臨失效的借來雨傘，以泛白骨灰色（210,205,190,255）搭配 `UmbrellaStyle::Broken`（殘破傘骨外形）視覺呈現「快壞掉的借傘」。

它是四把劇情傘中唯一攜帶額外數值屬性的葉類別：`leakRate_`（漏雨速率，0.5f），表示持此傘時每秒仍有 0.5 的雨量滲入比例。雨量系統（`ApplyRainSheltered` 路徑）可讀取 `GetLeakRate()` 來計算持破傘時的加速淋溼。此設計使玩家在 Ch1 選擇撿起破傘時，即便有「傘」可用，雨量仍會以較高速率累積，給出「破傘比真傘差但比沒傘好」的遊戲性差異。

`BeClaimed(player)` 覆寫設立持傘狀態（`SetHeldUmbrella(HeldUmbrella::Fragile)`），但不設任何影響結局的旗標——破傘是一條「平庸」的選擇，不特別懲罰也不特別獎勵。

## 關鍵內容（類別 / 函式 / 資料）

- **`FragileUmbrella(position)`**：建構子，硬編碼骨灰色 + `Broken` 樣式，名稱 `"FragileUmbrella"`；初始化 `leakRate_(0.5f)`。
- **`void BeClaimed(Player* player) override`**：設立持傘狀態，定義於 `.cpp`。
- **`float GetLeakRate() const noexcept`**：取漏雨速率（0.5f）。
- **`float leakRate_`**（private）：漏雨速率。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/TransparentUmbrella.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`、`src/game/entities/FragileUmbrella.cpp`、`tests/controller/test_factory.cpp`、`tests/entities/test_umbrella_render.cpp`、`tests/quest/test_ripple_seed_flags.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `TransparentUmbrella`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層 Item，地圖上由 `WorldSpawn` 生成（Ch1 四把傘之一）；`BeClaimed` 後 `isActive_=false`，幀末 Sweep 移除；雨量系統透過 `GetLeakRate()` 調整淋溼速率。

## OO 概念與設計重點

`FragileUmbrella` 是 [Template Method](../concepts/pat-template.md) 的「最小覆寫」葉類別：只覆寫 `BeClaimed`，並附加一個 `leakRate_` 屬性以承載遊戲性差異。這個額外屬性使傘的「品質」概念在純資料層（Model）中表達，不滲入 View 或控制邏輯。`GetLeakRate()` 的公開讓雨量系統可以在不知道傘的具體型別的情況下（透過 `HeldUmbrella::Fragile` 枚舉檢查）取得其特性，維持解耦。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/FragileUmbrella.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/FragileUmbrella.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
