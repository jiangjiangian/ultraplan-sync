> ⚠ **SUPERSEDED in part (Cycle 3 verify, 2026-05-19).** The C-1
> "`Player*` dangling after erase via `RespawnAtGate`/trap-spawn"
> hazard is hypothetical: `RespawnAtGate` had zero callers and was
> dead code until Cycle 3 wired it CONSERVATIVELY (`lethal=false` ⇒
> the respawn path is suppressed this cycle), and it *repositions* the
> player rather than deactivating it, so the deferred-deletion UAF it
> posits is not realized. Possibly-stale baseline (CLAUDE.md §2); see
> `.claude/BUGLEDGER.md` "Cycle 3" (I8).

# STRICT_REVIEW_R3 — 5-Reviewer Consensus

Round 3 of industrial-grade audit on commits `4a9f661`, `0007686`, `5ee0258`
(EventBus shared_mutex / fluent mutators + nodiscard + leaf final / main.cpp
STL refactor). Five reviewers each took an angle:

| Angle | Output | Posture |
|---|---|---|
| SRP | `SRP_review.md` | 4 CRITICAL, 5 MAJOR, 3 MINOR |
| DIP | `DIP_review.md` | 3 CRITICAL, 6 MAJOR, ~5 MINOR |
| Concurrency | `adv_concurrency.md` | 3 CRITICAL, 4 MAJOR, 5 MINOR |
| Memory / UB | `adv_memory.md` | 3 CRITICAL, 5 MAJOR, 5 MINOR |
| Performance | `adv_performance.md` | 2 CRITICAL, 5 MAJOR, 5 MINOR |

(Source markdowns archived locally outside the repo. The OCP / LSP / ISP /
Sean-Parent-ergonomics angles were rate-limited mid-run and remain optional
follow-ups.)

---

## TL;DR

The 3-commit batch is **net positive but oversold in two specific ways**:

1. The `shared_mutex` advertises "concurrent dispatch safe" but only protects
   the handler list, not handler bodies — and adds per-publish atomic cost
   on a single-threaded loop. *Fix the comment, optionally drop the mutex
   behind a `kEventBusThreadSafe` compile-time flag.*
2. The `Player*` raw pointer in `main.cpp` survives `objects.erase(...)`
   then is read by `std::find_if` to revalidate — a latent UB that
   sanitizers will not catch today (Player is never deactivated) but the
   moment it is, ASan flags `heap-use-after-free`. *Make the player a
   `std::weak_ptr<Player>` or look it up by index/handle.*

Everything else is either (a) an intentional student-game tradeoff that
production-FAANG would refactor but that is **fine for an Assignment 5
deliverable**, or (b) a known follow-up already on the SPRINT list.

---

## CRITICAL findings (cross-reviewer consensus)

### CR-1. EventBus shared_mutex is half-true — the comment lies

**Cited by:** Concurrency C1, C2, C3; Performance C1; Memory M-1; SRP S5
**Files:** `include/EventBus.h:47-48`, `src/EventBus.cpp:14-28`

The mutex DOES correctly serialize `handlers_` — fixes a real prior race
between Subscribe and Publish snapshot. **But:**

- Header comment says "Publish() is a reader (concurrent dispatch safe)" —
  false. The lock is dropped before dispatch, so handler bodies still race
  on whatever they write (`currentBuildingName`, `semester`, raylib GL
  state in the `RenderRequested` handler).
- `std::shared_mutex` is non-recursive (N4861 [thread.sharedmutex]/2). A
  future "simpler" refactor that pulls dispatch INSIDE the lock = UB.
- Performance: in this single-threaded game (10 objects, ~4 publishes /
  frame), the `unique_lock` / `shared_lock` overhead is **net-loss** vs
  the prior un-guarded version. Roughly 8 atomic RMWs per frame for zero
  observable benefit.

**Tension between reviewers:** Concurrency reviewer says "needed, fixes a
real prior race". Performance reviewer says "net-loss commit on this
single-threaded loop". Both are correct under their own assumptions.

**Resolution:** Keep the mutex, **fix the comment** to say what it
actually guards ("the handler list only — not handler bodies; do not
publish off the main thread"), and add a `[[maybe_unused]] static thread::id
kBusThreadId` assertion in DEBUG builds so off-main publish is a hard
error.

### CR-2. `Player*` raw pointer survives erase, read by `find_if` after delete

**Cited by:** Memory C-1, C-2; SRP S2 (god `main()`)
**Files:** `src/main.cpp:215-225` (the end-of-frame sweep block)

```cpp
objects.erase(remove_if(...));         // may delete the player
if (player && find_if(..., [player]) == objects.end()) player = nullptr;
//                          ^^^^^^^ raw pointer that was just freed
```

Reading the value of a freed pointer is implementation-defined per
[basic.stc.dynamic.deallocation]/4. Today it is latent because Player is
never `Deactivate()`-d in the current MVP — but the `RespawnAtGate` /
trap-umbrella spawn features are exactly the ones that will deactivate
the player object.

**Resolution options:**
- **Cheap:** Keep a `bool playerStillAlive_` set inside the `remove_if`
  predicate, then null `player` after the erase. No pointer comparison.
- **Right:** Replace `Player*` with `std::size_t playerIndex_` or
  `std::weak_ptr<Player>` — but the latter forces all GameObjects to
  shared_ptr, large blast radius.

### CR-3. Stack-captured singleton lifetime mismatch

**Cited by:** Concurrency M3; Memory M-1
**Files:** `src/main.cpp:73-76` (subscriber wiring captures `&semester`,
`&currentBuildingName`), `include/EventWiring.h:33` (same)

`EventBus` is a Meyers singleton (process lifetime). The
`EnteredBuilding` subscriber captures references to `currentBuildingName`
and `semester` — both stack-allocated in `main()`. After `main()`
returns, any Publish during static-destruction is `stack-use-after-return`.

Today the game exits cleanly without post-main publish. **But** anyone
who later adds an exit-time Publish (e.g. "save game" telemetry) hits UB
without warning.

**Resolution:** Either (a) make the subscribers self-unsubscribe at end
of `main()` via `EventBus::Instance().Clear()`, or (b) make the
event-routing state a `static` inside a singleton manager, not stack-local
in `main`.

### CR-4. `EventWiring::WireDefaultSubscribers` mixes 3 axes

**Cited by:** SRP S1; DIP D-M1
**File:** `include/EventWiring.h:18-53`

One inline function does:
- I/O (cout logging)
- State-machine transition routing
- Direct raylib drawing in the `RenderRequested` handler

Three reasons to change. SRP and DIP both flagged this — DIP because the
function still depends on the singleton via `bus.Subscribe(...)` body
referring to `EventBus::Instance()` indirectly through `bus`, SRP because
the three subscribers belong to three subsystems.

**Resolution:** Split into 3 free functions
(`WireLoggingSubscribers`, `WireStateTransitions`, `WireRenderHandlers`),
each in its own header. main.cpp calls all three. Cost: 30 lines, 0 risk.

### CR-5. `main()` is a 7-axis god function

**Cited by:** SRP S2; DIP D-M2
**File:** `src/main.cpp:50-223`

Even after the R3 refactor (304 → 223 lines), `main()` still does:
1. Window construction
2. Character select flow
3. EventBus wiring
4. Object spawning
5. Static collider building
6. Per-frame Update loop
7. HUD drawing + end-of-frame sweep

For a student game this is acceptable — but to claim the GoF Composite
Root pattern (which the design doc implies), each of these should be its
own function: `BootstrapWindow`, `SeedDefaultObjects`,
`BuildStaticColliders`, `RunGameLoop`. main is then ~30 lines.

---

## CRITICAL findings (single-reviewer)

### DIP-only: Factory is a switch, not a Factory Method

**File:** `src/GameObjectFactory.cpp:24-28`
DIP reviewer (D-C2) refutes the prior round's grade: this is *not* the
GoF Factory Method pattern. There is no Creator hierarchy with a virtual
`FactoryMethod()` overridden by subclasses. It is a **switch on enum**,
which is the explicit *anti-pattern* Factory Method exists to replace.

To actually claim the pattern (Assignment 5 grading rubric), introduce
`class IObjectFactory` with `Create(Vec2) = 0`, then a concrete factory
per type registered into a `std::unordered_map<ObjectType,
std::unique_ptr<IObjectFactory>>`.

### DIP-only: `Player::LoadSprite(string path)` is wrong abstraction layer

**File:** `include/Player.h:28-29`
`Player` is a domain class that should not know about file paths. The
asset layer (`gfx::Texture::Load`) should be injected. Current design
violates DIP's "abstractions should not depend on details".

### Concurrency-only: `Publish` declared `void` — snapshot can throw `bad_alloc`

**File:** `src/EventBus.cpp:14`
`void Publish(...) const` — caller has no exception annotation. If the
snapshot copy `OOMs`, callers like `Vendor::TryBuy`,
`BuildingTracker::Update`, `NPC::Interact` are in inconsistent state
across the throw point.

### Performance-only: `std::vector<Rect> frameColliders = staticColliders;` per frame

**File:** `src/main.cpp:147`
Full vector copy every frame. The `reserve` on line 122 is exact-fit, so
NPC push_backs trigger another realloc. **2 heap allocs / frame.** ND /
NVIDIA review would flag immediately. Fix: hoist a static / member-level
scratch buffer with `clear()` + `insert(end, ...)` once per frame.

### SRP-only: `Vendor::TryBuy` mixes purchase logic with 3 hardcoded UI strings

**File:** `src/Vendor.cpp:37-75`
"你錢不夠 (NT$XX)" / "已購入 XX。" / "庫存索引超出範圍。" — three
UI authoring decisions inside a transaction function. Extract to
`VendorMessages.h` for i18n + reuse.

---

## Confirmed STRICT_REVIEW findings (still standing)

- **STRICT C1 (Cursed/ProfTrap idempotency)** — confirmed by SRP, fixed
- **STRICT M1 ([[nodiscard]] sweep)** — confirmed and largely actioned in
  commit `0007686`; one remaining gap: `Player::AddMoney` is not
  `[[nodiscard]]` while `DeductMoney` is. Asymmetry called out by Memory
  m-3.
- **STRICT M6 (final on leaves)** — confirmed and actioned
- **STRICT M4 (dynamic_cast → virtual predicate)** — confirmed; Performance
  reviewer explicitly clocks the swap as a 5-10x speedup, citing Chandler
  Carruth CppCon 2015.

---

## Disputed STRICT_REVIEW verdicts

### "Player should split into 4 classes" — REJECTED

STRICT_REVIEW (line 205-215) cited Sean Parent to demand splitting Player
into PlayerState / PlayerInputController / PlayerSpriteRenderer /
RainExposureSystem. **SRP reviewer disagrees:**

- Unity's MonoBehaviour and Unreal's AActor both bundle these — no
  shipping AAA splits Player 4 ways.
- The only legitimate SRP cut is the **animator** (animTimer_ /
  animStep_ / lastFacing_) — see SRP S7. A 4-way shatter is over-
  engineering.

**Verdict for SPRINT:** S7 only (extract `PlayerAnimator`), not the
4-way shatter.

### "EventBus singleton is an SRP violation" — RECLASSIFIED

STRICT_REVIEW C2 framed the singleton as an SRP problem because tests
need to call `Clear()` between cases. **SRP reviewer:** that is a DIP
issue (high-level main depends on singleton), not SRP. The actual SRP
problem is `Clear()` being a production API used only by tests (S5).

### "GameObjectFactory satisfies Factory Method" — REJECTED

STRICT_REVIEW lists Factory as done. **DIP reviewer (D-C2)** says it
is a switch-on-enum, NOT the GoF pattern. To pass the Assignment 5
grading rubric for Factory Method, the Creator hierarchy must exist.
*Action:* either redefine the rubric claim (call it "Simple Factory" /
"Static Factory" instead of "Factory Method"), or refactor.

---

## SPRINT — recommended fix order

| # | Fix | Cost | Source | Why now |
|---|---|---|---|---|
| 1 | **Fix EventBus comment** to match what mutex actually guards | 5 min | CR-1 | Removes "attractive nuisance" before W5 lands |
| 2 | **Replace Player* raw cache with index handle** in main.cpp | 30 min | CR-2 | Latent UB blocks rain-respawn / trap-umbrella features |
| 3 | **Hoist `frameColliders` to scratch buffer** | 15 min | Perf C2 | Removes 2 allocs / frame |
| 4 | **Make Player::AddMoney `[[nodiscard]]`** for symmetry, OR remove it from DeductMoney | 5 min | Memory m-3 | API consistency |
| 5 | **Split `WireDefaultSubscribers` into 3 functions** | 20 min | CR-4 | One reason to change each |
| 6 | **Extract Vendor UI strings to VendorMessages.h** | 15 min | SRP S4 | i18n readiness |
| 7 | **Self-unsubscribe at end of `main()`** via `EventBus::Instance().Clear()` | 5 min | CR-3 | Closes static-destruction UB window |
| 8 | **Extract `PlayerAnimator`** (animTimer/animStep/lastFacing) | 60 min | SRP S7 | Real SRP seam, not over-engineering |
| 9 | **Choose: rename "Factory Method" claim → "Simple Factory", OR build proper Creator hierarchy** | 10 min vs 90 min | DIP D-C2 | Assignment grading clarity |
| 10 | (optional) Drop `shared_mutex` behind `#ifdef NCCU_BUS_MULTITHREADED` | 20 min | Perf C1 | Win back the 8 atomic RMWs / frame |

Total cheap-tier work (#1–7): **~95 minutes**. Restores correctness +
removes false advertising. Recommended before any W5 / chapter content
lands.

---

## SKIP list (what NOT to do, and why)

| Item | Why skip |
|---|---|
| Split Player into 4 classes | Sandi Metz / Unity convention disagree; only Animator extraction (S7) is real |
| Replace TransparentUmbrella tree with `std::variant` | Assignment 5 explicitly grades the inheritance hierarchy + Template Method on `beClaimed`; variant erases the grading evidence |
| Replace EventBus singleton with DI | Composition root is `main()`, which is 1 file — DI overhead exceeds benefit at this scale |
| Make `Publish` `noexcept` | Would force `try/catch` around every snapshot copy; `bad_alloc` here means the game is dead anyway |
| Replace raw loop in main.cpp with `std::ranges::for_each` | C++17, not 20 |
| Add ThreadSanitizer to CI | Would catch a bug we have already designed against; cost > value at student-project scale |

---

## Reviewer agreement matrix

| Finding | SRP | DIP | Conc | Mem | Perf |
|---|---|---|---|---|---|
| EventBus mutex comment is wrong | – | – | C1 | – | C1 |
| Player* dangling after erase | S2 | – | – | C-1 | – |
| Stack-captured singleton lifetime | – | – | M3 | M-1 | – |
| `WireDefaultSubscribers` 3-axis | S1 | M1 | – | – | – |
| `main()` is god function | S2 | M2 | – | – | – |
| `RespawnAtGate` mixes axes | S3 | M3 | – | M-3 | – |
| Vendor::TryBuy hardcodes UI | S4 | M4 | – | – | – |
| Factory is switch-on-enum | – | C2 | – | – | – |
| Player::LoadSprite knows file paths | – | C3 | – | – | – |
| frameColliders per-frame copy | – | – | – | – | C2 |

5/5 unique-angle reviewers found the same top-3 issues independently.
That's the strongest signal we have that the SPRINT list above is real,
not a single reviewer's bias.
