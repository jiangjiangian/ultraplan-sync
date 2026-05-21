# Audit — docs/content/interlude_market.md

**Overview (≤3):**
- The doc mixes loader-consumed §10 vendor stanzas (`## 攤位：` blocks, faithfully parsed by `VendorLoader.cpp`) with design-prose sections (economy / class structure / map overlay) that are largely aspirational. Vendor lineup, mechanics tags, karma hook, exit logic, plaza-centre layout, and three-consumable Item Factory chain are all in code; the rest is documentation that the engine never reads.
- Several numeric specs in the prose are NOT what code does: HotPack/EnergyDrink prices doc-vs-code drift (25→30 / 35→40), per-buff durations are placeholder no-ops, the `class ConsumableItem` shape in §三 (price field, `isStackable`, `quantity`, `effectDurationSec`) is fiction relative to the real `include/ConsumableItem.h`. CHANGELOG/BUGLEDGER do not document these as intentional Cycle-N redesigns.
- Rain throttle (§五「特殊規則」) is partially implemented as "no tick" (skip in `GameController::Update`), not as "10% strength + 0 accumulation"; consumable wipe on entry is correctly per ClearConsumables.

## Per-element annotations

- **SemesterState `Interlude_Market`** — enum + state class
  - [是否實作?] Yes — `include/SemesterState.h:9`, `src/SemesterStateMachine.cpp:44`, `include/InterludeMarket.h:7`
  - [邏輯衝突?] No

- **觸發: Ch1 結算後自動切換** — auto-transition
  - [是否實作?] Yes — `include/EventWiring.h:49-54` and `src/ChapterGate.cpp:18`
  - [邏輯衝突?] No

- **結束: 走出南端 OR 公告板 NPC 選「離開」** — exit
  - [是否實作?] Partial — south band only (`include/InterludeExit.h:30-37`, `src/GameController.cpp:501-511` arms `Flag_LeaveInterlude`, `src/ChapterGate.cpp:36-39` consumes). 公告板 NPC menu does NOT exist — InterludeExit.h header explicitly chose data-driven exit zone over a board NPC ("F.1-board=C")
  - [邏輯衝突?] Yes — doc still names 公告板 NPC selection; not in BUGLEDGER/CHANGELOG as a sanctioned redesign. SCRIPT_HANDOFF.md:136 still lists 公告板 NPC.

- **特殊規則: 雨粒子降至 10% / rainMeter 累積率 0 / 不淋雨** — rain overlay
  - [是否實作?] Partial — `src/GameController.cpp:374` skips all rain ticks in `Interlude_Market` (effectively accumulation=0 ✓). Visual 10% rain particle strength: no `setRainParticleStrength` anywhere — single rain pipeline only.
  - [邏輯衝突?] Yes (cosmetic) — particle 10% spec unimplemented; not flagged in CHANGELOG.

- **Player money 起始 100, HUD 右上, soft-cap** — money model
  - [是否實作?] Yes — `src/Player.cpp:34` ctor money_(100); `include/Player.h:32` `kMoneySoftCap = 300`; HUD `金幣: N 元` (Cycle-6 UI cycle CHANGELOG)
  - [邏輯衝突?] No (soft-cap 300 documented in §五 doc待覆核 #1 as open; resolved in code)

- **賺錢管道: NPC 打賞 / Pickup / 攤位小遊戲** — 3 sinks
  - [是否實作?] Partial — `CashPickup` ships (`include/CashPickup.h`, World.cpp:117 spawns); NPC tip and 套圈圈 / 二手書攤 小遊戲 sub-state are NOT implemented. `VendorLoader.cpp:161-168` recognises `onPurchase/onDonate/onAccept` but `onChat`/game/sell mechanics have no runtime handler — only `mechanic=buy` and `mechanic=donate` (via `karmaOnInteract`) function.
  - [邏輯衝突?] Yes — "套圈圈期望值 -1.7 元/次" and "二手書回收 30-60 元" copy is undeliverable; not redesigned per CHANGELOG.

- **HotPack 25元 / 60秒 ×0.5 rainMeter** — consumable Tier 1
  - [是否實作?] Partial — `include/HotPack.h:9` `kPrice = 30` (NOT 25); `src/HotPack.cpp:7` calls `resetRainMeter()` (instant full dry, NOT a 60-second ×0.5 throttle) + karma +5
  - [邏輯衝突?] Yes — price drift (25 vs 30) + effect model drift (instant reset vs timed buff). Neither in CHANGELOG.

- **WaterproofSpray 50元 / 90秒 rainMeter 凍結** — consumable Tier 1
  - [是否實作?] Partial — `include/WaterproofSpray.h:10` `kPrice = 50` ✓; `src/WaterproofSpray.cpp:8` explicitly stubbed "placeholder mood-only" — emits flavour message only, no rain effect
  - [邏輯衝突?] Yes — effect not delivered (acknowledged in header as "future-phase"; not in CHANGELOG).

- **EnergyDrink 35元 / +30% 速度** — Ch2 quest item
  - [是否實作?] Partial — `include/EnergyDrink.h:9` `kPrice = 40` (NOT 35); `src/EnergyDrink.cpp:7` karma +3 + flavour message (no speed buff). However is correctly wired as Ch2 學霸 quest item via `Player::ConsumeOne("EnergyDrink")` at `src/Chapter2Quest.cpp:22` ✓
  - [邏輯衝突?] Yes — price (35 vs 40) and speed effect drift. Ch2 wiring intact.

- **`class ConsumableItem` 介面 (price, isStackable, quantity, effectDurationSec, virtual Consume)** — proposed class shape
  - [是否實作?] Partial — real `include/ConsumableItem.h:9-27` has only `price_`, `Consume(Player*)` pure-virtual, no `isStackable`/`quantity`/`effectDurationSec`. Stackability instead lives in `Player::consumables_` map (`include/Player.h:142`).
  - [邏輯衝突?] Yes — `Consume(Player&)` ref vs real `Consume(Player*)` ptr signature; field set differs. Code is the truth.

- **GameObjectFactory `CreateConsumable`** — factory hookup
  - [是否實作?] Yes — `src/GameObjectFactory.cpp:21-23` handles HotPack/WaterproofSpray/EnergyDrink via existing `Create` switch (not a separate `CreateConsumable` overload; same Factory Method evidence)
  - [邏輯衝突?] No (overload-vs-switch is cosmetic).

- **Vendor 共用 class + VendorConfig 資料驅動 (10 攤 1 class)** — design key
  - [是否實作?] Yes — `include/VendorConfig.h:30-49`, `include/Vendor.h`, `src/Vendor.cpp:23-93`; runtime parse `src/VendorLoader.cpp:122-210`
  - [邏輯衝突?] No

- **VendorConfig 介面草稿 (StallSlot/dialogLines/greeting/onPurchase/onLeave/karmaOnInteract/worldPos)** — proposed shape
  - [是否實作?] Partial — real shape: `name`, `greeting`, `stock(VendorItem)`, `stallKeeper`, `tier`, `mechanic`, `karmaOnInteract`, `greetingLines`, `onPurchase`, `onLeave` (`include/VendorConfig.h`). `worldPos` lives separately in `VendorPlacement` (`include/ChapterVendors.h:23`). `StallSlot::stockLeft` instead on `VendorItem::stockLeft`.
  - [邏輯衝突?] No (architecturally equivalent split; CHANGELOG S5b-3 hints).

- **10 攤位 lineup 解析格式 (§10 grammar: `## 攤位：` / `> 攤主：` / `> 商品：item=price` / `> 機制：` / `> tier：` / `> karma：` / `> stock：`; `### greeting/onPurchase/onLeave`)** — parse contract
  - [是否實作?] Yes — `src/VendorLoader.cpp:50-194` matches the documented grammar (incl. variant `（…）` stripped at `:64-74`, ASCII-colon design notes ignored at `:104-114`)
  - [邏輯衝突?] No

- **Stall 1 熱騰騰雞排攤 — 炸物阿姨 / HotPack=25 / tier1 buy** — Tier-1 stall
  - [是否實作?] Partial — loaded (`tests/test_vendor_loader.cpp` covers); but `HotPack=25` in md is parsed as price=25 — overrides the `HotPack::kPrice=30`. VendorItem.price 25 is the operative charge.
  - [邏輯衝突?] Yes (vs HotPack.h:9 default).

- **Stall 2 校友手沖咖啡 / EnergyDrink=35** — Tier-1 stall
  - [是否實作?] Partial — same drift vs `EnergyDrink.h:9` default 40; md value 35 wins (vendor parses; default unused in market)
  - [邏輯衝突?] Yes

- **Stall 3 文創手作攤 / WaterproofSpray=50** — Tier-1 stall
  - [是否實作?] Yes (price matches `WaterproofSpray.h:10`)
  - [邏輯衝突?] No

- **Stall 4 雞蛋糕伯伯 / EggCake=10 / tier2 buy** — Tier-2 flavour
  - [是否實作?] Partial — VendorLoader parses + Vendor::TryBuy charges/inventories, but `EggCake` is NOT a `ConsumableItem` subclass (Factory has no case) — purely a string-keyed `Player::consumables_` count with no effect on `Consume`. Pure money sink per §10 note "Tier 2 風味食物無 Consume 效果".
  - [邏輯衝突?] No (doc explicitly notes Tier-2 sinks)

- **Stall 5 茶藝社花茶 / FlowerTea=15** — Tier-2 flavour
  - [是否實作?] Partial — same as above; pure money sink
  - [邏輯衝突?] No

- **Stall 6 三色章魚燒 / Takoyaki=20** — Tier-2 flavour
  - [是否實作?] Partial — same; money sink
  - [邏輯衝突?] No

- **Stall 7 學生會募款箱 / donate / karma+1 / stock 5** — Tier-3 donate
  - [是否實作?] Yes — `VendorLoader.cpp:188-191` reads `> karma:` & `> stock:`; `Vendor::TryBuy:85-86` applies `karmaOnInteract`; `:87` decrements finite stock
  - [邏輯衝突?] No

- **Stall 8 畢業生二手書攤 / sell / tier3 (陷阱傘殘骸 +20 回收)** — Tier-3 sell
  - [是否實作?] No — `mechanic=sell` has NO runtime handler; no path consumes `Flag_HasProfessorTrap` to credit money. `Vendor::TryBuy` is buy-only. Stall loads as a buy stall with empty stock (no `> 商品：`), so its menu shows only the decline.
  - [邏輯衝突?] Yes — promised "Ch1 ProfessorTrap → 回收 +20 元" recycling channel undelivered.

- **Stall 9 套圈圈遊戲攤 / game / tier3 (5元/次, 中三成 30元)** — Tier-3 game
  - [是否實作?] No — `mechanic=game` has no handler; no random-roll subsystem. Stall has no `> 商品：` line ⇒ empty stock.
  - [邏輯衝突?] Yes

- **Stall 10 新生招生宣傳攤 / flavor / tier4 / onAccept block** — Tier-4 flavour
  - [是否實作?] Partial — `onAccept` parsed as a success block alias (`VendorLoader.cpp:161-163`), but no `> 商品：` line → empty stock → only the decline appears.
  - [邏輯衝突?] No (flavour-only by design)

- **公告板 NPC** — Interlude exit point
  - [是否實作?] No — replaced by south-band exit zone per InterludeExit.h header decision ("F.1-board=C"). No 公告板 GameObject, no menu.
  - [邏輯衝突?] Yes — doc still describes it; SCRIPT_HANDOFF.md:136 still lists it. Not in CHANGELOG as a sanctioned redesign.

- **MapManager::ApplyStateOverlay (`spawnStalls/setRainParticleStrength/setRainMeterAccumulation/spawnNoticeBoard/despawnStalls`)** — proposed manager
  - [是否實作?] Partial — no `MapManager` class. Stall spawn handled by `World::RespawnChapterRoster` reading `ChapterVendors(state)` (`src/ChapterVendors.cpp:120-126`); rain skip by `GameController::Update:374`; `spawnNoticeBoard`/`despawnStalls`/particle-strength absent.
  - [邏輯衝突?] No (architectural fork; functionally equivalent for stalls + accrual; particle/board still missing).

- **Observer (SemesterStateMachine = Subject, MapManager/UIManager = Observer)** — pattern claim
  - [是否實作?] N/A — uses `EventBus` (Cycle-1 H1 RAII subscriptions) + per-state `IChapterState` polymorphism (`include/SemesterStateMachine.h`); no MapManager/UIManager observers literal.
  - [邏輯衝突?] No (functional equivalent)

- **Karma ×0.6 累積型刻度 (募款+1/封頂+5/-1 抱怨/+1 招生)** — table
  - [是否實作?] Partial — donate +1/+5 covered via `karmaOnInteract` (md `karma：+1` × stock 5). 套圈圈 / 招生 / 中獎致謝 / 強行抱怨 not implementable without sell/game handlers.
  - [邏輯衝突?] Yes (rows missing)

- **待覆核 #1 賺錢上限 999** — open question
  - [是否實作?] Resolved — `kMoneySoftCap = 300` per `include/Player.h:32` (CHANGELOG S5b-4 note)
  - [邏輯衝突?] No (intentional redesign)

- **待覆核 #2 市集存檔點** — open
  - [是否實作?] No — no save system; CHANGELOG silent
  - [邏輯衝突?] No (explicit open item)

- **待覆核 #3 章魚燒下章劇情提示 / #4 公告板 NPC 視覺 / #5 Vendor sprite 變化** — open
  - [是否實作?] Partial — #5 resolved by R6 Cycle-7 (`src/World.cpp:96-109`, `include/VendorSprite.h`, distinct curated fallbacks); #3/#4 outstanding
  - [邏輯衝突?] No

- **Ch1 ProfessorTrap 回收 +20 (二手書攤)** — cross-chapter hook
  - [是否實作?] No — see Stall 8; mechanic missing
  - [邏輯衝突?] Yes

- **Ch1 CursedUmbrella 路線: 攤主出現 flavor + karma 偏低被拒絕交易** — reaction
  - [是否實作?] No — no `Flag_TookCursedUmbrella` branch inside the Interlude vendor flow; `DialogOpener` Ch2/Ch4 routing exists but not market-side. Vendors do not gate on karma.
  - [邏輯衝突?] Yes

- **Ending C 醜綠傘不在市集 — 集英樓** — exclusion
  - [是否實作?] Yes — `src/ChapterVendors.cpp:108-118` puts the UglyUmbrella vendor in Ch4 集英樓 only; `Flag_BoughtUglyUmbrella` set there per VendorItem.setsFlag (`src/Vendor.cpp:91`); BUGLEDGER N2 reaffirms this is the sole Ending-C trigger
  - [邏輯衝突?] No

## Summary
- Elements audited: **34**
- Yes: 9 · Partial: 14 · No: 8 · N/A: 1
- Conflicts (Yes flagged): **15** — mostly content-vs-code numeric/effect drift (HotPack 25/30; EnergyDrink 35/40; HotPack/Spray buff models; particle 10%) and unbuilt sell/game/flavor-aware mechanics (二手書攤 ProfessorTrap recycle, 套圈圈, karma reactions, 公告板 NPC). None recorded as sanctioned Cycle-N redesigns in CHANGELOG/BUGLEDGER; CLAUDE.md §3 permits rebalance but requires changelog entry — these are drifts, not declared redesigns.
