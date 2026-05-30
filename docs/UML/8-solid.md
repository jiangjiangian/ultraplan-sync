## 8. 設計原則總結（SOLID / 其他）

| 原則 | 體現位置 |
|---|---|
| **單一職責 (SRP)** | `World`＝純資料、`View`＝只繪圖、`GameController`＝輸入＋協調；god-method 拆成 5 個單一職責 `ISystem`、4 個 `Handle*` 輸入處理器 |
| **開放封閉 (OCP)** | 新增一把傘＝加一個 `TransparentUmbrella` 子類別 + Factory enum；新增互動鉤子＝`QuestHookTable` 加一行；新增模擬 stage＝push 一個 `ISystem`——皆不需改既有 switch |
| **里氏替換 (LSP)** | 場景容器只持 `GameObject*`，透過 `As*()` 角色存取器多型分派；`Vendor` 可替換 `NPC` |
| **介面隔離 (ISP)** | `Roles.h` 把胖介面拆成 `IUpdatable`/`IDrawable`/`IInteractable`/`IMortal`——道具不必實作 `Update`，看板不必 `IMortal` |
| **依賴反轉 (DIP)** | Model 寫 `IRenderer&` 而非具體 raylib；輸入經 `InputSource` 抽象（`LiveInput`/`ScriptInput` 可換）；`Player` 只持 `TransparentUmbrella*`，永不 include 具體傘類 |
| **針對介面寫程式** | E 互動只認 `IInteractable`，傘的具體後果靠 vtable；對話副作用經 `EventBus` 廣播而非直接呼叫 UI |
| **記憶體安全 / RAII** | 物件以 `std::unique_ptr` 持有；移除採 `isActive_` 旗標 + 幀末 `World::Sweep()` mark-then-sweep（不在迭代中刪除、`objects_.front()` 恆為 Player）；`Texture`/`Font`/`EventBus::Subscription` 皆 RAII；GL 資源在 `~Window`／`CloseWindow` 前顯式釋放 |
| **決定性 / 可重播** | harness 固定 1/60 timestep + 腳本輸入源 ⇒ 同腳本 byte-identical `state.jsonl`；正常遊玩完全不受影響（無 `UMBRELLA_SCRIPT` 即旁路） |

### 架構鐵律（紅線）

1. `Player` 不得 `#include` 任何具體 umbrella header——只認 `TransparentUmbrella*`。
2. `Item` / `TransparentUmbrella` / 任何 Model 類別不得直接呼叫 raylib `Draw*`——一律經
   注入的 `IRenderer`，或經 `EventBus` 廣播交給 View 訂閱者。
3. 主迴圈不得在迭代中 `delete` GameObject——改 `isActive_ = false`，由 `SweepSystem` →
   `World::Sweep()` 於幀末統一 erase-remove。
4. `ISystem` 只動 model（`World&` / `Player&`）——不讀輸入、不呼叫 raylib、不繪圖。
5. harness 絕不改變正常遊玩行為（已驗證 bit-for-bit 不變）。

---

[← 回 UML 總覽](README.md) ｜ [上一節：§7 設計模式對照（GoF）](7-gof.md)
