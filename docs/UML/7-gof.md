## 7. 設計模式對照（GoF）

| 模式 | 落點 | 角色 |
|---|---|---|
| **Factory Method** | `GameObjectFactory::Create(ObjectType, Vec2)` | 由 `ObjectType` 列舉動態產生 12 種具體 `GameObject`（4 傘 + 3 消耗品 + Vendor + 3 種金幣 + Player） |
| **Template Method** | `TransparentUmbrella::BeClaimed`（純虛）、`ConsumableItem::Consume`（純虛） | 傘的 4 子類別提供 4 種被拾取行為；消耗品 3 子類別提供 3 種使用效果 |
| **Observer** | `EventBus::Subscribe` / `ScopedSubscribe` / `Publish` | UI/HUD 訂閱 `ShowMessage`、`KarmaChanged`、`UmbrellaClaimed`、`EnteredBuilding`、`PickupAcquired`；`Subscription` 為 RAII 退訂 token (H1) |
| **State** | `SemesterStateMachine` + `IChapterState` 的 5 個具體章節狀態 | 學期 5 章 + 4 結局（結局以哨兵記錄，非狀態物件）之間的轉換 |
| **Strategy / Pipeline（新）** | `ISystem::Run` 的 5 個 stage（Survival/Movement/Collision/Spawn/Sweep），由 `GameController` 依序執行 | 把原本約 793 行 god-method 拆成可組合、可重排、可單獨測試的 model 端 stage；Assignment #6 生存玩法可直接擴充 `CollisionSystem`／加 Spawner |
| **CRTP static mixin（新）** | `WithRoles<Derived, Base>` 編譯期實作 `AsUpdatable/AsDrawable/AsInteractable/AsMortal` | 以 `std::derived_from` + `if constexpr` 取代 `dynamic_cast` 做能力查詢；ISP 角色介面的靜態分派 |
| **Singleton** | `EventBus::Instance()` | 全域事件匯流排（`shared_mutex` 僅護 handler list） |
| **Command/Table（資料化）** | `QuestHookTable`（`InteractQuestHooks` / `RunInteractHooks`） | 把 ~14 個內嵌 `TryXxx` 互動鉤子改為一張有序、自我把關的資料表（OCP：加章節＝加一行 `RegisterHook`） |

---

[← 回 UML 總覽](README.md) ｜ [上一節：§6 系統互動：循序圖](6-sequence.md) ｜ [下一節：§8 設計原則總結（SOLID） →](8-solid.md)
