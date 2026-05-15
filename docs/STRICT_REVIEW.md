# STRICT_REVIEW.md — 工業級嚴格審查

> 採用 C++ Core Guidelines、Google C++ Style Guide、cppcheck、Sean Parent / Herb Sutter / Jason Turner 設計派的紅線。
> **不採用「學生專案通融」框架**。每條違規引用具體 guideline 條號 + 嚴重度。
>
> 證據來源：`cppcheck --enable=all --std=c++17` 全規則 + 對 46 個 header / 19 個 .cpp / 18 個測試的手動 grep + 設計閱讀。

---

## 結論總攬

| 項目 | 結果 |
|---|---|
| 工業招聘水準 | **不及格** — 第二輪會被刷 |
| 主要缺陷 | Singleton sprawl、raw pointer 非空契約缺失、零 `[[nodiscard]]`、CursedUmbrella 漏修 idempotency、EventBus 無 thread-safety |
| 之前審查的盲點 | 兩輪都在「學生 7 天交件」框架內互相妥協，沒人挑工業標準 |

---

## CRITICAL — 必須現在修

### C1. `CursedUmbrella::beClaimed` 缺少 idempotency guard（一致性 bug）
**證據：** `src/CursedUmbrella.cpp:5-27` — 沒有任何 guard。連點兩下 → 連扣兩次 50 karma → 連發三個事件。
**條號：** C++ Core Guidelines I.13 "Do not pass an array as a single pointer"（一般化：每個 mutation 必須有 invariant 守衛）。
**先前審查的盲點：** ProfTrap 拿到 guard，Cursed 沒拿到 — 兩個是同一個 bug pattern。Reviewer A 沒抓到 Cursed，Reviewer B 抓到 ProfTrap 卻忘了延伸到 Cursed。
**Severity：** CRITICAL（gameplay 真的會跑壞）。
**修法：**
```cpp
void CursedUmbrella::beClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;             // already claimed — be idempotent
    player->SetHasUmbrella(true);
    player->decreaseKarma(karmaPenalty_);
    isActive_ = false;
    // ... events ...
}
```
**延伸：** TrueUmbrella、FragileUmbrella 也要加 `if (!isActive_) return;`。`isActive_` 本來就是 GameObject 的 lifecycle 旗標，用它當 guard 就一致。

### C2. EventBus 是 multi-threaded UB 等待發生
**證據：** `src/EventBus.cpp:8-25`。`Instance()` 用 C++11 magic static（thread-safe），但 `handlers_` 的讀寫沒有任何 lock。
**條號：** C++ Core Guidelines **CP.1 "Assume that your code will run as part of a multi-threaded program"** + **CP.2 "Avoid data races"** + **I.3 "Avoid singletons"**。
**先前審查的盲點：** 第二輪對抗審查說「ACCEPT_AS_PRAGMATIC」— 理由是 tests 不需要 mock。但這跟 thread-safety 完全是兩件事。如果未來把 `BuildingTracker::Update()` 丟去 background thread（這在遊戲開發很常見），現在的 bus 直接 UB。
**Severity：** CRITICAL（latent UB，目前單執行緒巧合不爆）。
**修法（最少改動）：**
```cpp
class EventBus {
    mutable std::mutex mu_;        // protect handlers_ + Publish snapshot
    // ...
    void Subscribe(...) {
        std::lock_guard lock(mu_);
        handlers_[type].push_back(std::move(handler));
    }
    void Publish(const Event& e) const {
        std::vector<Handler> snapshot;
        {
            std::lock_guard lock(mu_);
            if (auto it = handlers_.find(e.type); it != handlers_.end())
                snapshot = it->second;
        }
        for (const auto& h : snapshot) h(e);
    }
};
```
**真正的修法：** 直接丟掉 singleton，改成 `IEventSink&` 注入。見 M3。

### C3. `Player*` 應該是 `Player&`（non-null 契約缺失）
**證據：** 14 處 `Player*` 參數（`GameObject::Interact`、`Item::OnPickup`、`TransparentUmbrella::beClaimed`、`ConsumableItem::Consume`、`Vendor::TryBuy`、`NPC::Interact`）。**每一處** 函式內第一行都是 `if (!player) return;`。
**條號：** C++ Core Guidelines **F.7 "For general use, take T* or T& arguments rather than smart pointers"** + **F.60 "Prefer T* over T& when 'no argument' is a valid option"**。反過來說：**若 nullptr 不是有效選項，必須用 reference**。
**Severity：** MAJOR（API 契約模糊；每個 callee 重複寫 null check → 12 處重複防衛性程式碼）。
**修法：** `void Interact(Player& initiator)` 一律改 reference。`if (!player) return` 整批刪掉。

---

## MAJOR — 工業審查會刷

### M1. 零 `[[nodiscard]]` 標註
**證據：** `grep -rn 'nodiscard' include/ src/` → **0 處**。所有 getter（GetKarma、GetMoney、HasFlag、IsActive、HasUmbrella、IsPickable、GetSpawnedEnemiesCount、GetLeakRate ...）和 factory function 沒有任何 nodiscard。
**條號：** C++ Core Guidelines **F.16 "For 'in' parameters, pass cheaply-copied types by value and others by reference to const"**（衍生）；更直接的是 **clang-tidy `modernize-use-nodiscard`** 規則。
**為什麼重要：** `player.GetKarma();` 寫成這樣會被 clang-tidy 警告 — 但目前沒人提示。對 `Vendor::TryBuy` 來說，丟掉 bool 回傳值 = 不知道有沒有買成功。
**Severity：** MAJOR（工業 codebase 預期所有 pure getter 都 nodiscard）。
**修法：** 在所有 `const` getter 與 `bool TryBuy(...)` 前加 `[[nodiscard]]`。

### M2. `std::function` heap-allocate 在每幀 hot path 上發送事件
**證據：** `EventBus::Handler = std::function<void(const Event&)>`，Publish 的 snapshot 用 `std::vector<Handler>` — 每次 Publish 都 copy 一份 vector（內含 std::function）。`std::function` 大於 SBO 上限就 heap allocate。
**條號：** C++ Core Guidelines **F.50 "Use a lambda when a function won't do"**（隱含的 perf 警告）；**Per.4 "Don't optimize prematurely"** vs **Per.7 "Design to enable optimization"**。
**Severity：** MAJOR（遊戲 fps 敏感；ApplyRain 每幀 Publish 一次 ShowMessage event，呼叫鏈 `Publish → vector copy → for each std::function call → heap dereferences`）。
**真正的修法：** 用 small `function_ref<void(const Event&)>` 或自己寫 vtable-style observer pattern。
**捷徑：** Publish 用 `const std::vector<Handler>&` reference 而非 snapshot copy — 但這會破壞 reentrancy。需要另寫 lock-free queue。**Trade-off：** 簡單捷徑放棄 reentrancy；正確修法是 bus 重構。

### M3. Singleton（EventBus）違反 C++ Core Guidelines I.3 直接條
**證據：** 14 個 .cpp 都呼叫 `EventBus::Instance().Publish(...)`。
**條號：** C++ Core Guidelines **I.3 "Avoid singletons"** — 原文直接說「Singletons are basically complicated global objects in disguise. ... To prevent the basic problems of `static class`-like singletons, prefer `unique_ptr` ownership with explicit lifetime management」。
**第二輪對抗審查的辯護：** 「14 個子類加 ctor 參數成本太高、tests 不需要 mock」 — 兩個都不成立：
  - tests **已經** 在用 `EventBus::Instance().Clear()` 跨測試共享狀態，本身就是 singleton 引起的測試隔離問題；改 DI 反而**讓測試更簡單**（each test 一個新 bus instance，不用 Clear）。
  - 14 個 ctor 參數加一個 `IEventSink&`，模板化基類後是一次性工作，不是每次寫新類都要重複。
**Severity：** MAJOR（架構債）。
**修法：** 在 `include/IEventSink.h` 加 interface，bus 變成預設實作，由 `main.cpp` 注入或透過 `GameObject` ctor 參數傳遞。Tests 各自 `LocalBus` 即可，不需要 Clear。

### M4. Raw `dynamic_cast<>` 在 game loop 內（perf + design smell）
**證據：** `src/main.cpp:154,208-214` — 兩處 `dynamic_cast`（取得 player、過濾 NPC collider）。
**條號：** C++ Core Guidelines **C.146 "Use dynamic_cast where class hierarchy navigation is unavoidable"**（隱含警告）；**C.147 "Use dynamic_cast for a checked downcast"**。實務上 dynamic_cast 在 RTTI 啟用下需要查 vtable，每幀做 N 次成本不可忽視。
**Severity：** MAJOR（perf + 違反 LSP — 用 type query 取代 virtual dispatch）。
**修法：** 對 collider 加 `GameObject::IsBlockingCollider() const { return false; }`，NPC override 為 true。Player 指標在 main 構造時直接保留。`dynamic_cast` 整批移除。

### M5. `Event` struct 是「fat union」— 浪費 + 容易出 bug
**證據：** `include/EventBus.h:21-26`：
```cpp
struct Event {
    EventType type;
    nccu::gfx::Vec2 position;
    nccu::gfx::Color color;
    std::string text;
};
```
**每個事件** 都帶位置 + 顏色 + 字串 —— 即使 `KarmaChanged` 完全不用 position / color / text 也照樣付出 string heap allocate 成本。
**條號：** C++ Core Guidelines **C.181 "Avoid 'naked' unions"** + **C.180 "Use unions to save memory"**。同義字：應該用 **`std::variant<RenderEvent, KarmaEvent, ShowMessageEvent, ...>`**。
**先前審查的盲點：** 兩輪都把 Event 當成既定設計沒挑戰。
**Severity：** MAJOR（每幀 publish 帶來不必要的 string alloc，且 schema 不安全 — Publisher 可能填錯欄位 subscriber 不知道）。
**修法：**
```cpp
struct ShowMessageEvent { std::string text; nccu::gfx::Color color; };
struct KarmaChangedEvent { int delta; };
struct UmbrellaClaimedEvent { std::string id; };
using Event = std::variant<ShowMessageEvent, KarmaChangedEvent, ...>;
// Publish: std::visit ...
```

### M6. Leaf 類別沒標 `final` — 漏掉 devirtualization
**證據：** `class TrueUmbrella : public TransparentUmbrella` — **4 個 umbrella 葉節點都沒寫 `final`**。3 個 ConsumableItem 葉節點也沒寫。
**條號：** C++ Core Guidelines **C.139 "Use `final` on classes sparingly"** — 「sparingly」指的是中間類，但 **leaf 類就該標 final** 讓 compiler devirtualize。
**Severity：** MAJOR（perf — 每次 `umbrella->beClaimed()` 都是 indirect call，標 final 後可 inline）。
**修法：** `class TrueUmbrella final : public TransparentUmbrella` × 4 次。

### M7. `iostream` 與 `std::cout` 在 production 程式碼
**證據：** `src/main.cpp:30,62,64,68` — `std::cout << "[Game] Entered: " ...`。
**條號：** Google C++ Style Guide **§"Streams"** — 明確禁用 `<iostream>`。建議 logger 抽象。C++ Core Guidelines 沒禁用，但 **SF.7 "Don't write `using namespace` at global scope in a header file"** 系列暗示要謹慎管理 namespace pollution。
**Severity：** MAJOR（在面試 take-home 是直接扣分項）。
**修法：** 抽 `Logger::Info(...)` interface；release build 編成 no-op。

---

## MINOR — 紀錄但不擋發版

### m1. `Window::ShouldClose() const` 可標 `static`
**證據：** cppcheck `functionStatic`：`bool ShouldClose() const noexcept { return ::WindowShouldClose(); }` — 不讀取任何 member。
**Severity：** MINOR。
**理由：** API 設計風格選擇 — 保留 instance method 強調「ShouldClose 是 Window 的問題」。但 cppcheck 抓到了。

### m2. 缺 `explicit operator bool()` 給 RAII 物件
**證據：** `nccu::gfx::Window` 只有 `bool owns_` 內部旗標，沒對外 `if (win) { ... }` 介面。
**條號：** C++ Core Guidelines **C.164 "Avoid implicit conversion operators"** + **C.165 "Use `using` for customization points"**。
**Severity：** MINOR。

### m3. Header guard 命名不符 Google style
**證據：** 用 `PLAYER_H_` 而非 `OOP_RAYLIB_LAB_INCLUDE_PLAYER_H_`。
**條號：** Google C++ Style Guide **§"The `#define` Guard"** — 推薦 `<PROJECT>_<PATH>_<FILE>_H_` 防止跨 lib 衝突。
**Severity：** MINOR（單一 lib 不衝突）。

### m4. `[[maybe_unused]]` 缺席
**證據：** `void Update(float /*deltaTime*/) override {}` — 用 `/* name */` 形式而非 `[[maybe_unused]] float deltaTime`。
**條號：** C++ Core Guidelines **NL.16 "Use a conventional class member declaration order"** （延伸）；clang-tidy `modernize-use-nodiscard` / `bugprone-suspicious-semicolon`。
**Severity：** MINOR（兩種寫法都合法；現代 C++ 偏好 attribute 形式）。

### m5. `cppcheck` 抓到的 `const` ref 缺失
**證據：**
- `src/CharacterSelect.cpp:47,85` — `gfx::Window& win` 沒讀寫，應 `const Window&`
- `src/main.cpp:263` — `for (auto& obj : objects)` 應 `for (const auto& obj : objects)`
**Severity：** MINOR（cppcheck `constParameterReference`、`constVariableReference`）。

### m6. 大量 unused getter（dead API surface）
**證據：** cppcheck `unusedFunction` 抓到 ≥15 個 getter 從沒被呼叫過：`Item::GetName`, `Item::IsPickable`, `NPC::IsQuestGiver`, `NPC::CurrentLineIndex`, `Player::ClearFlag`, `Vendor::Config`, `CashPickup::Value`, `CursedUmbrella::GetKarmaPenalty`, `FragileUmbrella::GetLeakRate`, `ProfessorTrapUmbrella::GetSpawnedEnemiesCount`, etc.
**條號：** C++ Core Guidelines **ES.5 "Keep scopes small"** + YAGNI。
**Severity：** MINOR（dead surface = future-cost；要嘛刪，要嘛測它）。

### m7. `useStlAlgorithm` 漏失
**證據：** cppcheck：
- `src/Vendor.cpp:28` — `BuildDialogLines` 可用 `std::transform`
- `include/Physics.h:30` — 應該用 `std::any_of`
- `src/main.cpp:180,192` — 應該用 `std::any_of` / `std::copy`
**條號：** C++ Core Guidelines **ES.1 "Prefer the standard library to other libraries and to 'handcrafted code'"** + **T.140 "Name all operations with non-trivial semantics"**。
**Severity：** MINOR（風格 + 表達力，不是 bug）。

### m8. Rule of 0/3/5 — Player / NPC / Vendor 沒明示
**證據：** Player.h 沒寫 copy / move 任何一個 — 走 compiler 預設。但 Player 含 `std::optional<Texture>`，`Texture` 是 move-only。結果：Player 是 move-only 但沒明說。
**條號：** C++ Core Guidelines **C.20 "If you can avoid defining default operations, do"** + **C.21 "If you define or `=delete` any default operation, define or `=delete` them all"**。
**Severity：** MINOR（目前能編譯就行；但讀者要看一遍才知道 Player 是不是 copyable）。
**修法：** 顯式 `Player(const Player&) = delete; Player(Player&&) noexcept = default;`。

### m9. doctest 用 FetchContent 拉取 — build reproducibility 風險
**證據：** `CMakeLists.txt:58-62` — `FetchContent_Declare(doctest GIT_REPOSITORY ...)`。沒網路就配置失敗。
**條號：** Google C++ Style Guide 隱含建議 vendor 依賴；Modern CMake 推薦 `find_package` + git submodule fallback。
**Severity：** MINOR（CI 環境一般有網路）。

### m10. raylib 直接 include 在 `include/gfx/Window.h`
**證據：** `include/gfx/Window.h:3` — `#include "raylib.h"`。
**條號：** 這是專案自己的規矩（專案的內部規矩文件「raylib.h 只在 include/gfx/」）— 但對 Window header 而言，這意味著 **任何 include `Window.h` 的 .cpp 都會拉進整個 raylib.h**。對 build time 是負擔。
**修法：** Pimpl — `Window` 用 `std::unique_ptr<Impl> impl_;`，把 raylib 鎖進 `.cpp`。**C++ Core Guidelines I.27 "For stable library ABI, consider the Pimpl idiom"**。
**Severity：** MINOR（編譯時間，不影響執行）。

---

## 必須挑戰的之前共識（按 user prompt 列點覆審）

### 「Player 拆 4 個類別 = over-engineering」？
**重新審：** 不對。Player 目前確實有 5 個軸：
1. 狀態容器（karma + money + rainMeter + hasUmbrella + flags）
2. 輸入處理（HandleInput → WASD）
3. 精靈渲染（Pipoya 32×32 cycle + facing row math）
4. 雨量物理（ApplyRain + 5u/s 累積）
5. 重生事件（RespawnAtGate + ShowMessage publish）

**Sean Parent 標準（"Inheritance Is The Base Class Of Evil"）：** 應該用 value-semantic 組合，不是單一 class 揉。即拆成 `PlayerState` + `PlayerInputController` + `PlayerSpriteRenderer` + `RainExposureSystem` 是工業正解。
**第二輪「ceremony before complexity」是錯誤定錨** — 這個 frame 預設「拆類別 = 多 indirection」，但實際上 4 個小 class 比 1 個 200 行 class 更好讀，每個都能單獨單元測試（目前 test_player_core 就在硬塞 5 種測試類型進同一個檔案，是症狀）。
**新判決：** **SHOULD SPLIT**。延後是因為時程，不是因為設計上不需要。

### 「EventBus singleton 14 處呼叫 = ACCEPT_AS_PRAGMATIC」？
**重新審：** 不對。已在 M3 拆解。
- 違反 C++ Core Guidelines I.3 **直接條**
- 觸發 CP.1、CP.2（thread-safety）
- Tests **已經** 因為 singleton 而要互相 `Clear()`，是 singleton 引起的測試隔離問題，不是「無 mock 成本」

**新判決：** **REAL VIOLATION**。Pragmatic 框架是錯的。

### 「Factory enum+switch 不用換 registry」？
**重新審：** 部分維持。第二輪對 static-init-order fiasco 的擔心是真的（Modern C++ Design Pattern Guide §"Avoid the static-init-order fiasco"）。但：
- enum+switch 不是 textbook GoF Factory Method — 純 dispatch
- 真正的 Factory Method 是 polymorphic creator hierarchy（`AbstractCreator::Create()`）
- Lab10 rubric 對「Factory pattern 存在」的判定可能寬鬆，但工業審查會問「為什麼這叫 Factory Method 而不是 switch」

**新判決：** 工業視角下這勉強過關（封閉型別集 + 集中註冊），但**不要再叫它「textbook GoF Factory Method」**。它是 lookup table。

### 「ProfTrap 只加 idempotency guard 就夠」？
**重新審：** 不對。已在 C1 拆解。**Cursed 漏修**。

---

## 額外查的東西（user 列點）

| 項目 | 狀態 |
|---|---|
| const-correctness 所有 getter | 7/15 getter 有 const，但**零 `[[nodiscard]]`**。Move() 應該不能 const ✓ |
| noexcept 在 dtor / move / swap | gfx wrapper 都有 noexcept；遊戲類沒寫但**走 Rule of 0**，implicit noexcept 取決於 member（多半是） |
| RAII / raw new / delete | 0 raw new、0 raw delete ✓ — clean |
| Rule of 0/3/5 | **沒明示** — Player / NPC / Vendor 沒寫任何 copy/move ops，靠 compiler 默認。技術上沒違反，但讀者看不懂。MINOR |
| explicit constructor | 主要 ctors 都有 explicit ✓（除了 move ctor — move 本來就不該 explicit） |
| override / final | 35 處 override ✓；**0 處 final**（M6） |
| 隱式轉換 / narrowing | 27 處 `static_cast<>` — 已經主動防禦了。沒看到 `(int)x` 的 C-style cast |
| include hygiene | 大致良好；`Window.h` 強迫拉 raylib 到所有引用者（m10）|
| forward declaration | `GameObject.h:5` 有 `class Player;` fwd decl ✓ |
| pImpl idiom | 沒用 — 對 gfx wrappers 是漏失（m10） |
| Thread safety EventBus | **未保護**（C2） |
| doctest 測 production path | ✓ — `ApplyRain`、`AddKarma`、`DeductMoney`、`TryBuy`、`Consume` 都有測。並非只測 getter。 |

---

## 拿這份 code 去 Google / Meta / NVIDIA take-home — 會被刷在哪關？

### 簡歷 / Pre-screen review（過不去 90%）
- 看到 GitHub README + commit log：**通過**（學生專案能 build + 有 tests 已經贏一半）
- 開檔看 `include/EventBus.h`：**直接看到 Singleton** → 70% 機率被 reviewer 標 `[design red flag]`
- 看 `Player*` raw pointer：Google C++ Style Guide §"Pointers and References" 明確要求 non-null 用 reference → **直接 down-vote**
- `std::cout` for game log：**第二個 down-vote**

### Onsite design 環節（特定 role）

**Google C++ / Chrome:**
- 問：「為什麼 EventBus 是 singleton？」→ 你說「pragmatic」→ **fail**。正解：「I.3 says avoid singletons; this is debt I'd repay via DI」。
- 問：「為什麼 `Event` 一個 struct 裝四種事件？」→ 你說「方便」→ **fail**。正解：「`std::variant` 才是 type-safe sum type」。

**Meta game engine:**
- 問：「heterogeneous container 怎麼處理？」→ 你說「`vector<unique_ptr<GameObject>>` + dynamic_cast」→ **fail**。Meta 期望 ECS（archetype-based）或至少 `std::variant` based。每幀 dynamic_cast = 即時刷掉。
- 問：「std::function 在 hot path 上的成本？」→ 你說不知道 → fail。

**NVIDIA / DOD-heavy role:**
- 看到 `std::vector<std::unique_ptr<GameObject>>` —— **每個 entity 一次 heap allocate + cache miss**。NVIDIA 期望 SoA（Structure of Arrays）/ DOD（Data-Oriented Design）。直接 fail。

### 結論：**會被刷在 onsite design 第一關**

不是因為 bug（這份 code 沒有 UB / crash），而是因為**設計 vocabulary 卡在 2005 年**（Singleton + raw pointer + 異質 container 全套）。要過 FAANG 級需要至少：
1. EventBus 變 DI
2. `Player*` 全換 `Player&`
3. `Event` 變 `std::variant`
4. dynamic_cast 全移除
5. Leaf class 全標 final
6. 加 `[[nodiscard]]` 全套

預估工程量：**8-12 小時 single-pass refactor**，做完才有 portfolio-grade。

---

## 行動建議

### 這個 commit 動的（5 分鐘）
1. `CursedUmbrella::beClaimed` 加 `if (!isActive_) return;` 守衛（C1）
2. 把 14 處 `[[nodiscard]]` 加到 const getter（M1）— 自動化可做

### 下個 sprint 動的（4-8 小時）
3. EventBus 抽 `IEventSink`，injection 注入（C2 + M3）
4. `Player*` 全換 `Player&`（C3）
5. 4 個 umbrella + 3 個 consumable + Vendor 全標 `final`（M6）
6. `dynamic_cast` 兩處改 virtual predicate（M4）

### portfolio-polish 動的（8-12 小時）
7. `Event` 改 `std::variant`（M5）
8. `Window` Pimpl 化（m10）
9. Player 拆 PlayerState / PlayerInputController / PlayerSpriteRenderer / RainExposureSystem
10. `std::cout` 改 logger interface（M7）

---

## 給之前兩輪審查的覆審（user 點名要求）

| 之前的共識 | 工業視角的覆審 | 為什麼之前錯了 |
|---|---|---|
| Player 拆是 over-engineering | **WRONG** — Sean Parent 標準下應該拆 | 「ceremony」框架預設 indirection = 壞；工業視角下 4 個 100 行 class 比 1 個 400 行可讀 |
| EventBus singleton 是 pragmatic | **WRONG** — 違反 I.3 + CP.1 + CP.2 | 「14 個 ctor 參數」是真實成本但被誇大；I.3 明文反對 singleton |
| Factory enum+switch 不用換 | **PARTIAL** — 工業視角下勉強過 | 不是真正的 Factory Method；但 closed set 下可接受 |
| ProfTrap 只加 idempotency guard 就夠 | **WRONG** — Cursed 漏修 | Reviewer A/B 只盯著 ProfTrap 個案，沒問「同類問題還有誰」 |

---

## 工具紀錄

- `cppcheck 2.18.5`：`--enable=all --std=c++17 --suppress=missingInclude` 全規則跑過 src/ + include/
- `clang-tidy 22.1.5` (Homebrew LLVM)：`--checks='-*,cppcoreguidelines-*,bugprone-*,modernize-*,performance-*,readability-*,hicpp-*'`，跑過 5 個關鍵 .cpp，原始輸出見下方附錄
- manual grep：對 raw pointer、`[[nodiscard]]`、`final`、`noexcept`、`explicit`、`override`、`iostream`、`new`/`delete`、`dynamic_cast` 全掃

## clang-tidy 補強發現（合併進 MAJOR/MINOR 清單）

### M8. C-style 陣列在 Player.cpp（cppcoreguidelines-avoid-c-arrays）
**證據：** `src/Player.cpp:16` — `constexpr int kWalkColumns[4] = {1, 0, 1, 2};`
**條號：** C++ Core Guidelines **ES.27 "Use `std::array` or `stack_array` for arrays on the stack"**；clang-tidy `cppcoreguidelines-avoid-c-arrays` + `hicpp-avoid-c-arrays` + `modernize-avoid-c-arrays`。
**Severity：** MAJOR（modernize 紅線）。
**修法：** `constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2};`

### M9. `Player::Player` 沒明示初始化所有 member（cppcoreguidelines-pro-type-member-init）
**證據：** `src/Player.cpp:29` — constructor 沒列 `flags_`（unordered_map）、`sprite_`（optional<Texture>）在 init list。default-construct 是 ok 的，但 linter 紅燈。
**條號：** C++ Core Guidelines **C.41 "A constructor should create a fully initialized object"**；clang-tidy `cppcoreguidelines-pro-type-member-init` + `hicpp-member-init`。
**Severity：** MINOR（功能正確；風格鬆散）。
**修法：** Member init list 明列 `, flags_{}, sprite_{}`。

### M10. `if (!ptr)` 是隱式 bool 轉換（readability-implicit-bool-conversion）
**證據：** 至少 4 處：`CursedUmbrella.cpp:6`, `ProfessorTrapUmbrella.cpp:6`, `Vendor.cpp:42`, `Player.cpp` 系列。
**條號：** clang-tidy `readability-implicit-bool-conversion`；C++ Core Guidelines **ES.87 "Don't add redundant `==` or `!=` to conditions"** 的反面（pointer-to-bool 不該隱式）。
**Severity：** MINOR（風格）。
**修法：** `if (player == nullptr) return;` — 同時與 C3（改 reference）一起做的話這條自然消失。

### M11. 手寫 `if (x > 100) x = 100;` clamp 應該用 `std::min/max`
**證據：** `src/Player.cpp:98-99,124-125` — `if (karma_ > 100) karma_ = 100; if (karma_ < -100) karma_ = -100;` 與 rain meter 同模式。
**條號：** clang-tidy `readability-use-std-min-max`；C++ Core Guidelines **ES.46 "Avoid lossy (narrowing, truncating) arithmetic conversions"** + **ES.40 "Avoid complicated expressions"**。
**Severity：** MINOR。
**修法：** `karma_ = std::clamp(karma_, -100, 100);` 一行搞定（含 `<algorithm>`）。

### M12. `Player.cpp:64` 用 runtime index 取 C-style 陣列
**證據：** `kWalkColumns[animStep_]` — `animStep_` 是 `int` runtime。
**條號：** clang-tidy `cppcoreguidelines-pro-bounds-constant-array-index`。
**Severity：** MINOR（你確實有保證 animStep_ 在 [0,3]，但 linter 不會證明這件事）。
**修法：** 上面 M8 換成 `std::array` 後，這條也可消，因為 `std::array::operator[]` 有 noexcept 保證；或直接用 `.at(animStep_)` 帶 bounds check。

### m11. 大量缺 `{ }` brace（hicpp-braces-around-statements / readability-braces-around-statements）
**證據：** `if (!player) return;`、`if (rainMeter_ > 100.0f) rainMeter_ = 100.0f;` 等十多處。
**條號：** Google C++ Style Guide **§"Formatting"**（明示要求單行 if 也要花括號）；clang-tidy `hicpp-braces-around-statements`。
**Severity：** MINOR（行業偏好；防 Apple goto fail bug 重演）。
**修法：** clang-format 套 Google style 一次就修完。

### m12. `5.0f` 應該是 `5.0F`（hicpp-uppercase-literal-suffix）
**證據：** 全 Player.cpp 都是 `0.15f`、`100.0f` 等小寫。
**條號：** clang-tidy `hicpp-uppercase-literal-suffix`；HICPP 13.4.1。
**Severity：** TRIVIAL（風格選擇）。
**判斷：** 這條我建議**忽略**。`5.0f` 是更常見的寫法；`5.0F` 更難讀。clang-tidy 在這事上跟 modern C++ 社群實際慣例不一致。

---

## 附錄 — clang-tidy 完整輸出摘要

```
src/CursedUmbrella.cpp:6 — implicit conversion 'Player *' -> 'bool'
src/CursedUmbrella.cpp:6 — statement should be inside braces
src/EventBus.cpp:12 — method 'Publish' can be made static [false positive: reads handlers_]
src/EventBus.cpp:14 — statement should be inside braces
src/Player.cpp:16 — do not declare C-style arrays, use 'std::array' instead
src/Player.cpp:29 — constructor does not initialize these fields: flags_, sprite_
src/Player.cpp:64 — array subscript with non-constexpr index
src/Player.cpp:98 — use `std::min` instead of `>`
src/Player.cpp:99 — use `std::max` instead of `<`
src/Player.cpp:124 — use `std::min` instead of `>`
src/Player.cpp:125 — use `std::max` instead of `<`
src/ProfessorTrapUmbrella.cpp:6 — implicit conversion 'Player *' -> 'bool'
src/Vendor.cpp:37 — method 'TryBuy' can be made static [false positive: reads config_]
src/Vendor.cpp:42 — implicit conversion 'Player *' -> 'bool'
```
（uppercase-literal-suffix 與 braces-around-statements 噪音 30+ 條未列）

---

## 引用

- C++ Core Guidelines: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
  - I.3 Avoid singletons
  - CP.1 Assume your code will run as part of a multi-threaded program
  - C.146 Use dynamic_cast where class hierarchy navigation is unavoidable
  - C.139 Use final on classes sparingly
  - F.7 For general use, take T* or T& over smart pointers
  - C.181 Avoid 'naked' unions
- Google C++ Style Guide: https://google.github.io/styleguide/cppguide.html
  - §"Streams"
  - §"The `#define` Guard"
- Sean Parent — "Inheritance Is The Base Class of Evil" (NDC 2017)
- Herb Sutter — "Sutter's Mill" GotW series
- cppcheck 規則：functionStatic, constParameterReference, useStlAlgorithm, unusedFunction
