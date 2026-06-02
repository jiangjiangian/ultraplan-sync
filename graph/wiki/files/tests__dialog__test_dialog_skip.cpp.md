---
id: "file:tests/dialog/test_dialog_skip.cpp"
type: test
path: tests/dialog/test_dialog_skip.cpp
domain: tests
bucket: dialog
loc: 221
classes: [TestInput]
sources: ["tests/dialog/test_dialog_skip.cpp"]
---
# `test_dialog_skip.cpp`

> **一句定位**：驗證兩個無障礙「跳過」操作——長按 E 超過 300 ms 自動推進對話（含放開重置、連點路徑保留），以及退格鍵立即讓 HUD 提示過期（只認退格鍵，Enter/E 無效）。

## 職責

本檔透過真實的 `GameController::Update` 迴圈驗證兩個新增的無障礙輸入功能，使用本地定義的 `TestInput`（與 `test_i35_interact_vendor.cpp` 中的同名類別相同設計，`Hold/Tap/Release/EndFrame`）。

**長按 E 自動推進對話**（4 個 SUBCASE）：
- 以 `world.Dialog().Open({"L0".."L5"})` 直接開啟六行對話，跳過 NPC 互動。
- **連點仍可用**：單格 Tap 推進一行；無輸入的格不推進；再 Tap 再推進一行。
- **長按達 300 ms 後自動推進**：Hold E，第 0 幀 edge 推進到 L1；再按住 10 幀（≈167 ms）仍在 L1；再按住 40 幀（共≈833 ms）後已離開 L1（自動推進多次）。
- **放開重置計時**：按住 10 幀後放開（計時重置），再按住 10 幀不觸發自動推進；確認計時不跨次按下累積。

**退格鍵強制讓 HUD 過期**（3 個 SUBCASE）：
- **新提示按退格鍵**：`world.SetHudMessage("transient banner")` 後 Tap Backspace，下一格後 `world.HudExpired() == true` 且 `HudAge() >= kHudTtl`。
- **無提示時退格為空操作**：`HudMessage().empty()` 維持，`HudAge` 不變，`HudExpired() == false`。
- **只認退格鍵**：先後 Tap E 和 Enter，`HudExpired()` 仍為 false；最後 Tap Backspace 才真正關閉提示。

## 關鍵內容（類別 / 函式 / 資料）

- `class TestInput : InputSource`：`Hold/Tap/Release/EndFrame`，邊緣語意的輕量驅動器。
- `Frame(controller, in)`：執行一個模擬格的 helper。
- `World::SetHudMessage(text)`、`World::HudMessage()`、`World::HudAge()`、`World::HudExpired()`、`World::DismissHud()`：HUD 狀態介面。
- `nccu::kHudTtl`：HUD 訊息存活時間（秒）。
- `Key::Backspace`：新增的按鍵枚舉值，退格鍵。

## 相依與在架構中的位置
- **#include（往外）**：`GameController.h`、`World.h`、`Player.h`、`EventBus.h`、`ui/MessageView.h`、`Input.h`、`Key.h`、`Time.h`、`Vec2.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 繼承 `InputSource`
- **每幀管線 / MVC 角色**：驗證 Controller 層的輸入處理（長按 E 自動推進、退格關閉 HUD）

## OO 概念與設計重點

本檔是 [arch-harness](../concepts/arch-harness.md) 輸入替換的整合測試：以 `TestInput` 置換真實裝置，在固定步進時鐘下驗證帶時間語意的行為（300 ms 門檻、4 幀冷卻）。測試以格數（10/40 幀）而非毫秒數表達預期，使斷言更穩定。退格鍵的「只認退格鍵」case 明確排除 E/Enter，防止其他鍵意外關閉 HUD 的回歸。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_skip.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_skip.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-harness](../concepts/arch-harness.md)
