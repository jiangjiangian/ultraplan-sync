---
id: file:tests/entities/test_umbrella_render.cpp
type: test
path: tests/entities/test_umbrella_render.cpp
domain: tests
bucket: entities
loc: 115
classes: [CountingRenderer, RectCall]
sources: ["tests/entities/test_umbrella_render.cpp"]
---
# `test_umbrella_render.cpp`

> **一句定位**：透過間諜 `IRenderer` 驗證四種雨傘（TrueUmbrella / FragileUmbrella / ProfessorTrapUmbrella / CursedUmbrella）各自算繪出明顯不同的圖形，且傘面色調兩兩曼哈頓距離 ≥ 120，在地圖上不會被混淆。

## 職責

此測試確保每種 `UmbrellaStyle` 的 `Render()` 實作都畫出獨特且可辨識的外觀。兩個 TEST_CASE 從兩個角度驗證這個不變式：

1. **完整指紋相異**：把四個算繪結果（矩形的 (x,y,w,h,r,g,b,a) 序列）放入 `std::set`，要求集合大小為 4；同時確認矩形數量（輪廓複雜度）也不全相同（至少 3 種不同數量），排除「同形狀換色」的情況。
2. **色彩距離足夠**：取每把傘 `rects[0]`（最上方的傘面色塊）作為標誌色，計算所有 C(4,2)=6 對的曼哈頓距離（|Δr|+|Δg|+|Δb|），全部要求 ≥ 120。

兩個測試都使用同一個 `DrawOf<U>()` 模板輔助函式，以 `Vec2{100, 200}` 為固定位置建構各款式並算繪。

## 關鍵內容（類別 / 函式 / 資料）

- `CountingRenderer`：實作 `IRenderer` 的間諜，記錄 `std::vector<RectCall> rects`、`int sprites`、`int texts`。與 `test_quest_pickup_render.cpp` 中的同名類別結構相同（同為本批次測試的慣用模式）。
- `RectCall`：單次 `DrawRect` 呼叫的幾何與顏色記錄。
- `Fingerprint()`：把 `CountingRenderer` 的矩形列表轉為 `vector<tuple<...>>` 供 `std::set` 比較。
- `DrawOf<U>()`：對型別 `U` 在固定位置建構並呼叫 `Render(spy)` 的模板輔助。
- `TEST_CASE("四種雨傘各自算繪出明顯不同的圖形")`：指紋集合大小 == 4；矩形數量集合大小 ≥ 3；所有款式 sprites==0、texts==0、rects.size()≥3。
- `TEST_CASE("各把傘的傘面色調彼此相距夠遠")`：逐對計算曼哈頓色距，斷言全部 ≥ 120。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/IRenderer.h`（間諜基底），`game/entities/TrueUmbrella.h`、`FragileUmbrella.h`、`ProfessorTrapUmbrella.h`、`CursedUmbrella.h`（四個受測類別）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`CountingRenderer` 繼承 `IRenderer`。
- **每幀管線 / MVC 角色**：—（純視覺行為測試，不在執行期管線內）

## OO 概念與設計重點

本測試體現 [Template Method 模式](../concepts/pat-template.md)：四種雨傘繼承自 `TransparentUmbrella` 並在 `BeClaimed` / `Render` 覆寫差異行為；此測試從 Render 的輸出端驗證子類確實提供了各自的外觀差異。`CountingRenderer` 作為 [DIP Renderer](../concepts/arch-dip-renderer.md) 的測試替代品，讓視覺邏輯可在無 GL context 下完整測試。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_umbrella_render.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_umbrella_render.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
