---
id: file:include/game/gfx/UmbrellaGlyph.h
type: header
path: include/game/gfx/UmbrellaGlyph.h
domain: game
bucket: gfx
loc: 148
classes: []
sources: ["include/game/gfx/UmbrellaGlyph.h"]
---
# `UmbrellaGlyph.h`

> **一句定位**：所有雨傘外觀繪製的唯一真實來源，以純矩形向量字符（rect-only glyph）實現五種傘的視覺辨識，不依賴 raylib texture，任何出現處視覺一致。

## 職責

`UmbrellaGlyph.h` 定義了兩件事：`UmbrellaLook` 列舉（五種外觀）與 `DrawUmbrellaGlyph` 函式（繪製對應字符），以及 `UmbrellaLookColor`（取各外觀的標誌色）。

**五種外觀**（`UmbrellaLook`）：`TrueBlue`（藍、寬圓頂、「乾淨」）、`FragileBroken`（灰白、只剩手柄與斷骨）、`CursedPurple`（暗紫、下垂傘面 + 黑手柄）、`UglyGreen`（螢光綠、凹凸不對稱）、`ProfessorTrap`（危險紅、稜角尖刺）。

**`DrawUmbrellaGlyph(r, look, bounds, alpha=255)`**：所有幾何以方框比例（0..1 的 `fx,fy,fw,fh`）表示，再以 `bounds` 縮放到實際螢幕座標。這讓同一輪廓在 20×20 世界佔地、16×16 地面拾取物與大張結局卡上都讀得出來。`alpha` 等比縮放每個矩形的不透明度，供淡入效果（結局卡）使用。純呼叫 `IRenderer::DrawRect`，不呼叫 DrawText/DrawTexture——符合「resources/ 可能為空，傘必須在任何情況下都能畫」的架構規定。

每種外觀的方框矩形集精心設計：True 三層漸縮傘面（給「完整」感）；Fragile 只剩樞紐+斷骨（給「壞掉」感）；Cursed 下垂骨架（給「壓抑」感）；Ugly 偏心不對稱（給「醜得全校最好認」感）；ProfessorTrap 金字塔尖刺（給「陷阱/武裝化」感）。

## 關鍵內容（類別 / 函式 / 資料）

- **`enum class UmbrellaLook { TrueBlue, FragileBroken, CursedPurple, UglyGreen, ProfessorTrap }`**：五種外觀列舉。
- **`constexpr Color UmbrellaLookColor(UmbrellaLook look) noexcept`**：取各外觀的標誌色（`[[nodiscard]]`）；公開供 HUD 等呼叫端保持一致配色。
- **`inline void DrawUmbrellaGlyph(IRenderer& r, UmbrellaLook look, Rect bounds, unsigned char alpha=255)`**：繪製 glyph；方框比例幾何、alpha 縮放；switch 分支每個外觀各有獨立的矩形集。
- 內部 `lambda fade(col)`：以 `alpha` 等比縮放顏色透明度。
- 內部 `lambda rc(fx,fy,fw,fh,col)`：方框比例定位並呼叫 `r.DrawRect`。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Color.h`（`Color` 型別與 `Colors::DarkGray`/`Black`）、`include/engine/render/IRenderer.h`（`DrawRect` 介面）、`include/engine/math/Rect.h`（方框矩形型別）。不引入任何 raylib 符號。
- **被誰使用（往內）**：`include/game/entities/TransparentUmbrella.h`（建構子引用 `UmbrellaLook` 決定外觀）；`src/game/entities/QuestFlagPickup.cpp`、`src/game/entities/TransparentUmbrella.cpp`（Render 路徑）；`src/ui/ChapterCard.cpp`、`src/ui/EndingView.cpp`、`src/ui/InventoryView.cpp`（UI 表面）；測試。
- **繼承 / 實作 / 體現**：—（純函式 + 列舉）。
- **每幀管線 / MVC 角色**：View 繪製工具，被多個 View 表面呼叫。整個 glyph 繪製鏈（Model 持 `UmbrellaStyle` → 映射為 `UmbrellaLook` → 傳給此處繪製）完全符合 [DIP 渲染器](../concepts/arch-dip-renderer.md) 架構：Model 不知道 raylib、View 透過 IRenderer 間接繪圖。

## OO 概念與設計重點

`UmbrellaGlyph.h` 是「架構紅線嚴格執行」的最佳例證：「Item 不得呼叫 DrawText/DrawTexture」的規定在此以純矩形向量字符滿足。任何表面（地圖、地面、UI）只要傳入 `IRenderer` 和 `UmbrellaLook`，就能得到視覺一致的傘 glyph，而不需關心是哪個表面——這正是 [DIP 渲染器抽象](../concepts/arch-dip-renderer.md) 的好處。

方框比例（0..1）的幾何設計是「可縮放字符」的核心：同一個 `fx,fy,fw,fh` 集合在任意 `bounds` 下都能正確縮放，避免為不同尺寸維護多份像素座標。`alpha` 縮放讓結局卡的淡入動畫無需修改 glyph 邏輯，是 [Strategy](../concepts/pat-strategy.md) 風格的正交性（淡入策略與繪製邏輯完全分離）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/UmbrellaGlyph.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/UmbrellaGlyph.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP 渲染器](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
