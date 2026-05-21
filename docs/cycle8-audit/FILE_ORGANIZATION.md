# Audit — docs/FILE_ORGANIZATION.md

**Overview (≤3):**
1. The document is a **forward-looking refactor proposal** (proposals A & B + migration script), not a description of the current layout. As of HEAD `b33db2b` no migration has been performed: `include/` still holds 68 flat `.h` files (only `include/gfx/` is a subfolder, as the doc explicitly acknowledges).
2. The 32 specific headers cited by name across the two proposals all exist on disk in `include/` — so the doc's *premises* (which files would be moved) are factually correct. The proposed *destination* directories (`entities/`, `world/`, `state/`, `event/`, `factory/`, `ui/`, `model/`, `view/`, `controller/`, `infra/`) do NOT yet exist.
3. The CMake claim (`GLOB_RECURSE` already recurses, no CMakeLists edit needed) verifies against `CMakeLists.txt:19-22` exactly as cited; the `gfx/` precedent (`#include "gfx/Color.h"`) is verified in `src/View.cpp:15-17`.

## Per-element annotations

- **TL;DR — "30+ headers散在 include/ 底層"** — 1-liner motivating reorg.
  **[是否實作?]** Yes — `ls include/*.h | wc -l = 68` flat headers (well above 30); `include/gfx/` is the lone subfolder. Premise factually correct.
  **[邏輯衝突?]** No.

- **CMakeLists 不用改半行** — claim that `GLOB_RECURSE` auto-picks up moved files.
  **[是否實作?]** Yes — `CMakeLists.txt:19-22` matches the quoted snippet verbatim (`file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp" "${INCLUDE_DIR}/*.h")`).
  **[邏輯衝突?]** No.

- **`include/gfx/` 已經是這種佈局的先例** — used as existence proof.
  **[是否實作?]** Yes — `include/gfx/` exists with 18 headers (`Bounds.h`, `Camera2D.h`, `Color.h`, `Font.h`, `Input.h`, `Renderer.h`, `Texture.h`, `Time.h`, `Window.h`, etc.); `src/View.cpp:15` uses `#include "gfx/Renderer.h"` confirming the include-path pattern.
  **[邏輯衝突?]** No.

- **提案 A — 桶 `entities/`** (GameObject, Character, Player, NPC, Item, TransparentUmbrella + 4衍生, ConsumableItem + 3衍生, CashPickup, Vendor, VendorConfig).
  **[是否實作?]** No (proposal). All 16 listed source headers DO exist at `include/<Name>.h`; destination dir `include/entities/` does not exist.
  **[邏輯衝突?]** No.

- **提案 A — 桶 `world/`** (Buildings, Obstacles, Physics, BuildingTracker, WorldConfig).
  **[是否實作?]** No (proposal). All 5 source headers exist flat in `include/`; `include/world/` absent.
  **[邏輯衝突?]** No.

- **提案 A — 桶 `state/`** (SemesterState, SemesterStateMachine, Chapter1AddDrop, Chapter2Midterms, Chapter3SportsDay, Chapter4Finals, InterludeMarket).
  **[是否實作?]** No (proposal). All 7 source headers exist flat in `include/`; `include/state/` absent.
  **[邏輯衝突?]** No — but note doc omits sibling state-related headers actually present: `Chapter2Quest.h`, `Chapter3Quest.h`, `Chapter4Quest.h`, `ChapterGate.h`, `ChapterPickups.h`, `ChapterQuestItems.h`, `ChapterSpawns.h`, `ChapterVendors.h`, `EndingGate.h`. (Incomplete inventory, not a contradiction.)

- **提案 A — 桶 `event/`** (EventBus).
  **[是否實作?]** No (proposal). `include/EventBus.h` exists; `include/event/` absent. Doc omits `EventWiring.h` (also event-related, present in `include/`).
  **[邏輯衝突?]** No.

- **提案 A — 桶 `factory/`** (GameObjectFactory).
  **[是否實作?]** No (proposal). `include/GameObjectFactory.h` exists; `include/factory/` absent.
  **[邏輯衝突?]** No.

- **提案 A — 桶 `ui/`** (CharacterSelect, "一次性畫面").
  **[是否實作?]** No (proposal). `include/CharacterSelect.h` exists; `include/ui/` absent. Doc omits other UI-ish headers present today: `TitleScreen.h`, `GameHelp.h`, `EndingView.h`, `InventoryView.h`, `MessageView.h`, `DialogView.h`.
  **[邏輯衝突?]** No (proposal predates those views; under-scoped not contradictory).

- **提案 B — MVC 分層 (`model/ view/ controller/ infra/`)** — alternate proposal.
  **[是否實作?]** No (proposal, explicitly "工程量大"). None of the four dirs exist.
  **[邏輯衝突?]** No.

- **不建議：扁平 + 後綴前綴命名** — anti-pattern note.
  **[是否實作?]** N/A (advisory).
  **[邏輯衝突?]** No.

- **遷移腳本 (`git mv` + `sed -i ''`)** — executable migration recipe for proposal A.
  **[是否實作?]** No — not run; would no-op safely today as none of the target dirs exist yet. Minor portability note: `sed -i ''` is BSD syntax; GNU sed (Linux, this repo's CI) needs `sed -i` without the empty arg. Not a conformance bug because the script is a sample, not policy.
  **[邏輯衝突?]** No.

- **建議流程 / 注意事項** — "先別動 / 一次重組 / `git mv` 保歷史 / 別碰 `resources/`".
  **[是否實作?]** Yes (followed) — no reorg attempted; `resources/` untouched (curated-empty per CLAUDE.md §1).
  **[邏輯衝突?]** No.

## Summary

- Documented elements audited: **13**
- Yes (implemented / matches reality): **4** (TL;DR premise, CMake claim, `gfx/` precedent, "先別動" followed)
- No (intentionally unimplemented — proposal status): **8** (all 6 proposal-A buckets, all of proposal B, migration script)
- N/A: **1** (anti-pattern advisory)
- Partial: **0**
- 邏輯衝突 (logical contradictions): **0** — every header cited by name exists; the doc is internally consistent and accurately classifies itself as a proposal.
- Minor caveats (non-conflicts): proposal A's `state/`, `ui/`, and `event/` buckets under-inventory siblings that have since landed (`Chapter*Quest.h`, `ChapterGate.h`, `EndingGate.h`, `EventWiring.h`, `TitleScreen.h`, `GameHelp.h`, several `*View.h`); `sed -i ''` in the script is macOS syntax, not Linux.
