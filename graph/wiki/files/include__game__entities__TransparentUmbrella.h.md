---
id: file:include/game/entities/TransparentUmbrella.h
type: header
path: include/game/entities/TransparentUmbrella.h
domain: game
bucket: entities
loc: 108
classes: [TransparentUmbrella]
sources: ["include/game/entities/TransparentUmbrella.h"]
---
# `TransparentUmbrella.h`

> **一句定位**：雨傘繼承樹的抽象中介（Template Method 骨架），定稿拾取與繪製流程，並以純虛擬 `BeClaimed()` 讓四個葉類別覆寫認領效果。

## 職責

`TransparentUmbrella` 是 `Item` 與四個具體雨傘（`TrueUmbrella`／`FragileUmbrella`／`ProfessorTrapUmbrella`／`CursedUmbrella`）之間的抽象中間層，是雨傘 Template Method 的核心。

**Template Method 骨架**：`Render`（依外形繪字符）與認領觸發（任務閘控邏輯）在本層定稿；可變的 `BeClaimed(player)` 延後到葉類別。`Interact` 與 `OnPickup` 兩條入口共用同一道任務閘（定義於 `.cpp`，可見 `Player`/`EventBus`）：唯有玩家已持 `Flag_PromisedVictim`（苦主承諾）後才能認領，否則發出引導提示。Ch3/Ch4 的 `Flag_PromisedVictim` 早已設立，故再認領不受影響。

**外形系統**：`UmbrellaStyle` 列舉（`Domed`/`Broken`/`Spiked`/`Drooping`）由葉類別在建構時傳入，`LookForStyle` 函式（`constexpr`，與列舉並排定義）將其映射到 `UmbrellaLook`（供 `UmbrellaGlyph::DrawUmbrellaGlyph` 使用），確保地圖內繪製與任何使用相同外觀的其他表面永不漂移。

**ISP 設計**：扮演 `IDrawable`（`Render` 繪字符）+ `IInteractable`（`Interact` 認領），捨棄 `IUpdatable`（傘不需逐幀更新）。`WithRoles<TransparentUmbrella, Item>` 為鍵，四個葉類別的 `static_cast<TransparentUmbrella*>` 均合法。

碰撞盒固定 20×20（建構子中硬編碼），使傘的可互動區域一致。

## 關鍵內容（類別 / 函式 / 資料）

- **`enum class UmbrellaStyle { Domed, Broken, Spiked, Drooping }`**：四種傘面外形，供 Render 切換剪影。
- **`constexpr UmbrellaLook LookForStyle(UmbrellaStyle) noexcept`**：外形到共用外觀列舉的映射，單一事實來源。
- **`TransparentUmbrella(position, name, tint, style=Domed)`**：建構子，碰撞盒固定 20×20。
- **`void Render(IRenderer&) const override`**：呼叫 `DrawUmbrellaGlyph`，依 `style_` 繪製字符。
- **`UmbrellaStyle Style() const noexcept`**：取外形（`[[nodiscard]]`）。
- **`void Interact(Player*) override`**：任務閘控認領，定義於 `.cpp`。
- **`void OnPickup(Player*) override`**：同 `Interact` 的任務閘，定義於 `.cpp`。
- **`virtual void BeClaimed(Player*) = 0`**：純虛擬的認領鉤子，葉類別覆寫。
- **`Color umbrellaTint_`**（protected）：傘的色調。
- **`UmbrellaStyle style_`**（protected）：外形輪廓。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/Item.h`（基底）、`include/engine/math/Color.h`（tint）、`include/game/gfx/UmbrellaGlyph.h`（繪製字符所需的 `UmbrellaLook`/`DrawUmbrellaGlyph`）。
- **被誰使用（往內）**：四個葉類別的 `.h`（直接繼承）；`src/game/entities/TransparentUmbrella.cpp`（任務閘邏輯）；`tests/quest/test_ch1_quest.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<TransparentUmbrella, Item>`，實作 `IDrawable`、`IInteractable`；被 `TrueUmbrella`/`FragileUmbrella`/`ProfessorTrapUmbrella`/`CursedUmbrella` 繼承。體現 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：Model 層 Item。View 呼叫 `Render`；Controller E 互動掃描呼叫 `Interact`；`BeClaimed` 後 `isActive_=false`，幀末 Sweep 移除。

## OO 概念與設計重點

`TransparentUmbrella` 是本遊戲中 [Template Method](../concepts/pat-template.md) 最典型的應用：不變的「任務閘驗證 → 觸發 `BeClaimed` → 設 `isActive_=false`」骨架封存於中間層，可變的「認領效果」留給四個葉類別。每個葉類別只需一個函式覆寫，極大降低實作負擔。

`LookForStyle` 與 `UmbrellaStyle` 並排定義，確保「剪影外觀映射」只有一個地方定義，消除剪影與色彩的漂移風險。[ISP](../concepts/oo-isp-roles.md) 的捨棄 `IUpdatable` 讓更新管線不為雨傘執行任何工作。`WithRoles<TransparentUmbrella, Item>` 的 [CRTP](../concepts/oo-crtp.md) 鍵位讓所有四個葉類別共享同一份角色集定義。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/TransparentUmbrella.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/TransparentUmbrella.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
