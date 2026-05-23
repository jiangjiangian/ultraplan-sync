# 實體角色介面拆分 (ISP) 與 CRTP 靜態多型 — 架構說明

本文件補充 `系統架構與UML分析：尋傘記.md` 的實體系統 (Entity System)
類別圖。它記錄一次**結構重構**：把 `GameObject` 的「胖介面」拆成三個
獨立的角色介面 (role interface)，並以一個 CRTP mixin (`WithRoles`) 在
**編譯期**靜態綁定角色能力查詢。對應檔案：

- `include/entities/Roles.h` — 三個角色介面 + `WithRoles` + `ForEachRole`
- `include/entities/GameObject.h` — 移除三個純虛擬函式，改為三個能力查詢
- 各實體 header / `src/entities/*.cpp` — 依實際職責繼承對應角色
- `tests/entities/test_roles.cpp` — 靜態派發的回歸測試

---

## 1. 重構前：胖介面 (fat interface)

```
                       ┌─────────────────────────────┐
                       │         GameObject          │  «abstract»
                       │  + Update(float)         = 0 │  ← 每個實體都被
                       │  + Render(IRenderer&)    = 0 │    強制實作這三個，
                       │  + Interact(Player*)     = 0 │    即使 body 是 {}
                       │  + BlocksMovement() ...      │
                       └─────────────────────────────┘
```

問題：`ConsumableItem::Update/Render` 是空的 no-op、`Player::Interact`
是空的 no-op，卻都被純虛擬合約逼著實作。違反介面隔離原則
(Interface Segregation Principle)：類別被迫依賴它用不到的方法。

## 2. 重構後：三個獨立角色介面 + CRTP mixin

```
   «interface»        «interface»          «interface»
  ┌───────────┐      ┌───────────┐      ┌──────────────┐
  │ IUpdatable│      │ IDrawable │      │ IInteractable│
  │ +Update() │      │ +Render() │      │ +Interact()  │
  └───────────┘      └───────────┘      └──────────────┘
   (三者互相獨立，皆 NOT 繼承 GameObject)

  ┌──────────────────────────────────────────────────────────┐
  │                       GameObject                          │
  │  + AsUpdatable()      : IUpdatable*       {return null}   │  ← 預設不扮演
  │  + AsDrawable() const : const IDrawable*  {return null}   │    任何角色；
  │  + AsInteractable()   : IInteractable*    {return null}   │    容器透過這
  │  + BlocksMovement()/DialogLines()/NpcId()/IsVendor()/     │    三個查詢派發
  │    IsQuestGiver()  (分類查詢，原樣保留)                    │
  └──────────────────────────────────────────────────────────┘
                              ▲
        ┌─────────────────────┴───────────────────────┐
        │  WithRoles<Derived, Base> : public Base       │  «CRTP mixin»
        │   using Base::Base;   // 繼承 Base 的建構子    │
        │   AsUpdatable()  override:                     │
        │     if constexpr (derived_from<Derived,        │
        │                    IUpdatable>)                │
        │        return static_cast<Derived*>(this);     │  ← 編譯期偵測
        │     else return nullptr;                       │    Derived 繼承了
        │   (AsDrawable / AsInteractable 同理)           │    哪些角色介面
        └───────────────────────────────────────────────┘
```

### 每個實體保留 / 捨棄的角色（依「原本 body 是否為空」決定）

| 實體 | 原 Update | 原 Render | 原 Interact | 保留角色 | `WithRoles` 鍵在 |
|---|---|---|---|---|---|
| `Player`              | 實作 | 實作 | **空 no-op** | IUpdatable + IDrawable | `Player` (leaf, final) |
| `NPC`                 | 實作 | 實作 | 實作 | 三者皆是 | `NPC` (中介層) |
| `Vendor` (: NPC)      | 繼承 | 繼承 | 繼承 | 三者皆是（與 NPC 同集合） | 沿用 NPC 的 `WithRoles` |
| `ConsumableItem` 家族 | **空 no-op** | **空 no-op** | 實作 (→Consume) | IInteractable | `ConsumableItem` (中介層) |
| `TransparentUmbrella` 家族 | **空 no-op** | 實作 | 實作 | IDrawable + IInteractable | `TransparentUmbrella` (中介層) |
| `CashPickup`          | **空 no-op** | 實作 | 實作 (→OnPickup) | IDrawable + IInteractable | `CashPickup` (leaf) |
| `QuestFlagPickup`     | **空 no-op** | 實作 | 實作 (→OnPickup) | IDrawable + IInteractable | `QuestFlagPickup` (leaf) |

捨棄一個「空 no-op」角色的行為等價性：原本主迴圈會呼叫一個空函式，
重構後則直接「跳過」該物件——跑一個空 no-op 與跳過它，對遊戲狀態完全
等價。這正是 ISP 帶來的好處：少了無意義的覆寫，主迴圈只走訪真正扮演
該角色的物件。

### `WithRoles` 鍵 (key) 的選擇規則

在「角色集合固定」的那一層套用 `WithRoles`：

- 若某中介層底下所有 leaf 共用同一組角色 → `WithRoles` 鍵在該中介層。
  - `ConsumableItem`（三種飲料皆只覆寫 `Consume`，角色集合相同）
  - `TransparentUmbrella`（四把傘皆只覆寫 `beClaimed`，角色集合相同）
  - `NPC`（唯一子類別 `Vendor` 只多了 `TryBuy`/`IsVendor`，角色集合相同；
    `static_cast<NPC*>(this)` 對 Vendor 仍合法）
- 若 leaf 各自不同 → `WithRoles` 鍵在各 leaf（`Player`、`CashPickup`、
  `QuestFlagPickup`）。

## 3. 派發點 (dispatch sites)

容器仍是 `std::vector<std::unique_ptr<GameObject>>`（執行期多型），
三個派發點改走能力查詢，**完全沒有 `dynamic_cast`**：

- `src/controller/GameController.cpp` 更新迴圈：透過樣板工具
  `ForEachRole<IUpdatable>(world_.Objects(), [dt](IUpdatable& u){ u.Update(dt); });`
- `src/ui/View.cpp` 繪製迴圈（const 路徑，直接呼叫 const 查詢）：
  `if (const auto* dr = d.obj->AsDrawable()) dr->Render(renderer_);`
- `src/controller/GameController.cpp` 互動路徑（撿取 / Vendor）：
  `else if (auto* it = o.AsInteractable()) it->Interact(player);`

`ForEachRole<Role>` 是刻意保留的「一個真實、乾淨的樣板工具」：以
`if constexpr (std::same_as<Role, …>)` 在編譯期把 `Role` 對應到正確的
查詢函式，未來要新增第四種角色只需在此多一個分支，而非改動每個呼叫點。

## 4. 設計理由（誠實評估）

**靜態多型 (CRTP `WithRoles`) 在編譯期綁定角色能力查詢**：`AsUpdatable`
等函式以 `static_cast<Derived*>(this)` 取得指標，沒有透過虛擬表
(vtable) 做型別判斷，也沒有 `dynamic_cast` 的 RTTI 走訪。它提供
**編譯期檢查的 ISP 角色分離**——一個型別「扮演哪些角色」由它繼承哪些
角色介面決定，編譯器在 `if constexpr` 當下就確定，寫錯（例如對未繼承
`IDrawable` 的型別呼叫 Render）會在編譯期被擋下，而非執行期才崩。

**就本遊戲的規模而言，靜態 vs 虛擬/`dynamic_cast` 的「執行期效能」差異
可以忽略**：場上同時不過數十個實體、60fps，每幀的角色派發成本微不足道。
這次重構真正的收穫**不是效能**，而是：

1. 乾淨的角色分離——空 no-op 覆寫被消除，每個類別只宣告它真的扮演的
   角色；
2. 徹底移除 `dynamic_cast`——能力查詢由 CRTP 靜態完成，且維持既有的
   分類查詢慣例（`BlocksMovement`/`NpcId`/`IsVendor`/`IsQuestGiver`
   仍是 closed-under-inheritance 的虛擬 bool/pointer，非 `dynamic_cast`）。

**異質場景容器必然維持執行期多型**：`std::vector<unique_ptr<GameObject>>`
在同一個容器裡持有玩家、NPC、傘、撿取物等不同具型，依賴執行期虛擬派發
逐一更新/繪製/互動。這是靜態多型**無法、也不應**取代的一處——樣板/CRTP
無法把「執行期才知道具型」的異質集合攤平成單一靜態型別。`objects_.front()`
仍是玩家、幀末 mark-then-sweep 延遲刪除、`beClaimed`/`isActive_` 冪等
守衛皆原樣保留。

---

*對應回歸測試見 `tests/entities/test_roles.cpp`：每個檢查都透過
`GameObject&` 觀察實體（即容器看到的樣子），證明靜態 `As*()` 查詢能
正確穿過執行期多型基底回報該扮演/不扮演的角色，且回傳的指標真的會
派發到具型覆寫。*
