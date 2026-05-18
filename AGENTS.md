# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Identity

**《尋傘記：政大山下篇》** (The Lost Umbrella: NCCU Downhill Campus) — a 2D top-down JRPG built in **C++17 + Raylib 5.5** as the OOP Final Project. The game is the extension of `Assignment #5` (due 2026-05-12) and intentionally targets four GoF design patterns from *Design Patterns: Elements of Reusable Object-Oriented Software*.

The two design documents (Chinese, in repo root) are the **single source of truth** for architecture and narrative:
- `系統架構與UML分析：尋傘記.md` — UML class diagram, state machine, sequence diagram
- `遊戲企劃與敘事架構.md` — chapters, karma/rain meters, ending matrix, ripple effects

**Read both before adding new classes or chapters. Do not reproduce or restate their content here.**

## Build & Run

```bash
cmake -B build              # configure (FetchContent pulls raylib 5.5 on first run)
cmake --build build         # compile
./build/OOP_Raylib_Lab      # run
```

⚠️ **Executable is `OOP_Raylib_Lab`** (from `project()` in `CMakeLists.txt:2`), **not** `OOPFinal` as the assignment README example shows. Do not rename the project — the assignment grader follows whatever the CMake says.

CMake details that are non-obvious:
- `GLOB_RECURSE` auto-picks up new `.cpp` in `src/` and `.h` in `include/` — no manual list maintenance, but re-run `cmake -B build` if the IDE's CMake cache is stale.
- `main.cpp` is excluded from the lib target and linked only into the executable.
- `resources/` is auto-copied next to the executable on build, so runtime paths use **relative form**: `LoadTexture("resources/assets/sprites/player.png")`.
- macOS `-framework Cocoa` is already wired.
- Headers in `include/` need NO prefix when included: `#include "Player.h"`, not `#include "include/Player.h"`.

## Repository Layout

```
assignment-5-jiangjiangian/
├── src/                      C++ source (currently only main.cpp, ~empty)
├── include/                  C++ headers (empty)
├── tools/
│   └── composite_worldmap.py    Python/PIL — composes worldmap.png from 27 building tiles
├── resources/
│   ├── design/               Reference material — NOT loaded by game
│   │   ├── 山下校園.jpg          NCCU official campus map (visual ref only)
│   │   ├── GameMap.png            Google Maps + red-line playable area boundary
│   │   ├── CoursesList.xlsx       For NPC dialog flavor / fake course schedules
│   │   └── WHAT-IF_*.pdf          LLM branching narrative paper (R&D ref)
│   └── assets/               Game-bound assets (LoadTexture/LoadSound targets)
│       ├── Real_Buildings/         Source photos used as image-to-image inputs
│       │                           (one folder per building, 1–3 jpg/png each)
│       ├── style/                  style_ref.png + pixel_technique_anchor.png
│       ├── sprites/                Player, NPCs, items
│       ├── tiles/                  Tilesheet for ground/walls/decoration
│       ├── maps/                   worldmap_base.png + composed worldmap.png (2048²)
│       ├── buildings_3d/           27 oblique-3/4 pixel building tiles (1024² each)
│       ├── ui/                     Dialog box, HUD, rain meter, inventory
│       └── audio/                  BGM + SFX
├── 系統架構與UML分析：尋傘記.md   Architecture spec
├── 遊戲企劃與敘事架構.md          Narrative + system spec
├── README.md                   Assignment requirements & grading rubric
├── CMakeLists.txt
└── .gitignore                  Already ignores build/ .env *.key .DS_Store
```

⚠️ Chinese filenames use a **full-width colon `：`**, not ASCII `:`. Quote the path in bash: `"系統架構與UML分析：尋傘記.md"`.

## Architecture

### Inheritance tree (from `系統架構與UML分析`)

```
GameObject (abstract; position, hitBox, isActive, virtual Update/Draw/Interact)
├── Character (abstract; speed, direction, spriteSheet, animation)
│   ├── Player (rainMeter, karma, hasUmbrella, Inventory, HandleInput)
│   └── NPC (dialogLines, isQuestGiver, UpdateDialogByState)
└── Item (abstract; itemName, isPickable, OnPickup)
    └── TransparentUmbrella (abstract; umbrellaTint, beClaimed)
        ├── TrueUmbrella           — triggers chapter clear
        ├── FragileUmbrella        — leakRate, slow rain accumulation
        ├── ProfessorTrapUmbrella  — spawns chasing TA NPCs on pickup
        └── CursedUmbrella         — karmaPenalty, drives Bad Ending
```

### GoF patterns → planned files

| Pattern | Header (to create) | Purpose |
|---|---|---|
| **Factory Method** | `include/GameObjectFactory.h` | Required by Assignment 5 grading |
| **State** | `include/SemesterStateMachine.h` | Chapter1 → Interlude → Ch2 → Ch3 → Ch4 → Endings A/B/C |
| **Observer** | `include/EventBus.h` (or `IObserver` mixin) | Item → UI Manager events; karma / rainMeter changes broadcast |
| **Template Method / Strategy** | `include/TransparentUmbrella.h` + 4 derived | Polymorphic `beClaimed()` |

Optional later: **Object Pool** (the `isActive` flag is already in the spec for this), **Command** (player input → action), **Composite** (GameObject children).

### Hard architectural rules

1. **Player must NOT `#include` any concrete umbrella class.** It only knows `TransparentUmbrella*`. Dynamic dispatch via VTable does the rest. (See sequence diagram in design doc.)
2. **UI ↔ Data decoupling**: items emit events to a UI/Event Manager; they do not call `DrawText` themselves.
3. **Map Manager owns object lifetime**: removal must be deferred to end-of-frame (mark `isActive = false`, sweep after the loop). Never `delete` while iterating the GameObject vector — iterator invalidation will crash.

### State machine summary (do not duplicate the GDD)

5 main states, transitions gated by inventory flags + ripple-effect bools:
- `Chapter1_AddDrop` → `Interlude_Market` → `Chapter2_Midterms` → `Chapter3_SportsDay` → `Chapter4_Finals`
- Endings: A (`karma > 80` + full quest line), B (`karma < 0` via `CursedUmbrella`), C (buy ugly green umbrella for cash)

NPC dialog selects line set from current `SemesterState` via `UpdateDialogByState()`. Same campus map, contextual rendering (rain particle density, ambient light, market tents) keys off the state.

## Asset Generation Workflow

This project generates 16-bit pixel art via the **`nanobanana` MCP server** (Google Gemini Image, configured user-scope). All artwork shares a single style anchor.

### One-time setup (already done)

- `Codex mcp list` should show `nanobanana ✓ Connected`
- API key is in `~/.Codex.json` user-scope MCP config (NOT in this repo)
- **Tier 1 billing required** — image generation has zero free tier as of 2026-05. Only Flash (`gemini-2.5-flash-image`, $0.039/img) is reliably available; NB 2 (`gemini-3.1-flash-image-preview`) and Pro (`gemini-3-pro-image`) currently return `limit: 0` even on Tier 1 for some accounts. Default to `model_tier="flash"`.
- The MCP **silently swallows 429 errors** — if `returned: 0, images: []` shows up, run `curl https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-image:generateContent ...` to see the real error.

### The style anchor rule (current convention)

Two anchor images live in `resources/assets/style/`:

| Anchor | When to use as input |
|---|---|
| `style_ref.png` | Original illustrated JRPG mood (rainy scene + transparent umbrellas). Use for **player sprites, NPC sprites, umbrella icons, narrative scene art**. |
| `pixel_technique_anchor.png` | Neutral 3-building pixel-art technique sample on flat gray. Use for **architectural building tiles** where real-photo fidelity matters more than narrative mood. |

Building generation pattern (validated for all 27 campus buildings):

```python
mcp__nanobanana__generate_image(
    input_image_path_1 = "<real_building_photo>.png",          # architectural fidelity
    input_image_path_2 = "resources/assets/style/pixel_technique_anchor.png",  # pixel art technique
    model_tier         = "flash",
    output_path        = "resources/assets/buildings_3d/<name>.png",
    # NO negative_prompt (triggers MCP bug with input images — see Pitfalls)
    # Prompt must NOT contain "this" or "reference photo" (also triggers bug)
)
```

For sprites / scene art (style_ref.png path):

```python
input_image_path_1 = "resources/assets/style/style_ref.png"
model_tier         = "flash"
output_path        = "resources/assets/<category>/<descriptive_name>.png"
```

### Prompt rules

- **Always include**: `16-bit pixel art`, `sharp pixel outlines`, `NO anti-aliasing`, `limited palette`, `Stardew Valley × EarthBound aesthetic`, `NO TEXT NO LETTERS NO LABELS`.
- **For sprites**: specify pixel size (`32 pixels tall`, `64x64 sprite`), facing direction, transparent background.
- **For buildings**: `oblique 3/4 perspective` (entry-screen) — explicitly NOT top-down.
- **For maps**: `pure 90 degree birds-eye, FLAT, NOT isometric` — Gemini drifts toward isometric otherwise; reinforce in the negative prompt.
- **Never include**: real-world brand names (7-Eleven, etc.), real campus building names, the official NCCU map. Generate **original interpretations inspired by the design docs**, not 1:1 reproductions of NCCU's official cartography or signage.

### Folder → category mapping

| Generation target | Output folder | Use case |
|---|---|---|
| Single illustrative scene | `resources/assets/style/` | Mood board / palette anchor |
| Player + NPC sprites (4-direction) | `resources/assets/sprites/` | LoadTexture for Character classes |
| Top-down playable world map | `resources/assets/maps/` | Background blit + collision lookup |
| Building exterior preview | `resources/assets/buildings_3d/` | Shown when entering a building |
| Tilesheets (ground/walls/decor) | `resources/assets/tiles/` | Tilemap rendering |
| Dialog box, HUD frames, item icons | `resources/assets/ui/` | Hand-coded overlay layer |

### Cost ledger (track manually if budget matters)

Approximate plan, all `flash` tier:
- ~7 building 3D exteriors × $0.039 ≈ $0.27
- 4 worldmap tiles ≈ $0.16
- Player 4-dir × 3 frames = 12 sprites ≈ $0.47
- 4 transparent umbrella icons ≈ $0.16
- 5–10 NPC sprites ≈ $0.20–0.40
- UI frames ≈ $0.12
- **Full pass ≈ $1.5 USD**, with retries probably $3–5.

### Image-to-image workflow

Once `style_ref.png` exists, the most reliable consistency tool is the Files API ID returned in each `generate_image` response (e.g. `files/8pmk7dpgg8xj`). Reuse via `file_id` for edits. The MCP also supports `mcp__nanobanana__continue_editing` (if available in this MCP build) for delta edits without re-uploading reference.

## Development Workflow

### Per task type

| Task | First step | Then |
|---|---|---|
| New game feature (e.g. inventory UI) | `superpowers:brainstorming` to align with GDD intent | `superpowers:writing-plans` → implement → `superpowers:verification-before-completion` |
| New raylib API call | `mcp__plugin_context7_context7` query first — do NOT trust training data on raylib 5.x signatures | implement → build → run |
| New asset | Check `style_ref.png` is referenced; pick correct output folder | generate → review → if reject, adjust prompt; do NOT manually edit pixels |
| Bug / crash | `superpowers:systematic-debugging` | only propose fix after reproducing |
| Pre-commit | `superpowers:verification-before-completion`: must `cmake --build build` AND launch the binary for ≥5s without crash | `commit-commands:commit` |
| Multi-step refactor | `superpowers:writing-plans` first | `superpowers:executing-plans` |

### Verification gate

Before claiming any feature is "done":
1. `cmake --build build` — zero warnings on `-Wall -Wextra` (add to CMake if not present yet)
2. `./build/OOP_Raylib_Lab` opens, the new feature is exercised, the window survives ≥5 seconds
3. The four GoF patterns above each have at least one concrete derived class living in the codebase

For pure-data logic (Inventory, Karma arithmetic, SemesterState transitions), unit tests are valuable. Raylib rendering layer is not unit-tested.

### Commit conventions

- Never add `Co-Authored-By` trailers (global user preference).
- Never `--no-verify` to skip hooks.
- Use `commit-commands:commit` skill so messages stay terse and consistent.

## Tooling Setup (already configured)

| Tool | Status | Use for |
|---|---|---|
| `nanobanana` MCP | ✓ user-scope | All artwork generation |
| `context7` MCP | ✓ plugin enabled | Raylib 5.x API lookups |
| `superpowers` plugin | ✓ enabled | brainstorming, plans, debugging, verification |
| `commit-commands` plugin | ✓ enabled | Standardized commits |
| `code-review` plugin | ✓ enabled | Pre-merge review |
| `Codex-md-management` plugin | ✓ enabled | Updating this file |

## Reference Documents (read these, don't restate them)

| Document | Authority over |
|---|---|
| `系統架構與UML分析：尋傘記.md` | Class diagram, state machine, sequence diagram, OOD principles |
| `遊戲企劃與敘事架構.md` | Chapter content, dialog tone, Karma/Rain mechanics, ending matrix, LLM prompting schema |
| `README.md` | Grading rubric (build / draw / WASD / Factory / UML — 7 items × 10–20 pts) |
| `resources/design/山下校園.jpg` + `GameMap.png` | Spatial layout reference (red line = playable boundary; trapezoidal — wider south, narrower north) |

## Known Pitfalls

1. **Executable name** is `OOP_Raylib_Lab`, not `OOPFinal`.
2. **Chinese filenames** require quoted paths in bash (full-width colon).
3. **Image-gen MCP swallows 429s** — if image returns empty, debug with direct `curl` to Gemini API.
4. **NB 2 / NB Pro models** still hit `limit: 0` on Tier 1 in some accounts; stick to `model_tier="flash"`.
5. **Map prompts drift to isometric** — explicitly negate "isometric, perspective, oblique view" when you want pure top-down.
6. **The parent directory `三下/oop/` is also a git repo** with unrelated weekly homework. Always run `git` from inside `assignment-5-jiangjiangian/`. `/ultraplan` will fail at the parent if invoked from the wrong cwd.
7. **`resources/design/` contains 6+ MB of reference material** — do NOT blindly `LoadTexture` from there. Only `resources/assets/` should ever be opened by raylib.
8. **Save state format not yet decided** — when implementing, propose a simple JSON in `resources/save/` (gitignored).
9. **Progressive JPEGs break Nano Banana** as `input_image_path_*` — the MCP raises `'NoneType' object has no attribute 'get'`. Convert sources to PNG first: `sips -s format png input.jpg --out output.png`. The `tools/composite_worldmap.py` and any new building-gen calls should always feed PNGs.
10. **Specific prompt phrasings trigger the same MCP bug** when input images are attached: avoid demonstrative pronouns like "this" or "this reference photo" — they consistently 500. Refer to inputs by index instead: "input image 1", "input image 2". Also avoid attaching `negative_prompt` together with input images — the combination triggers the bug too. Negative-prompt content can be folded into the main prompt as "no X, no Y" clauses.
11. **Worldmap composition is not AI-generated as one shot.** Use `tools/composite_worldmap.py`: it removes each building tile's gray plate via flood-fill, scales it nearest-neighbor, and pastes it onto a NB-generated empty terrain base. Adjust positions by editing the `BUILDINGS` dict in that script; rerunning is free.

## Current Progress (snapshot, last updated 2026-05-08)

### Art assets
| Item | State |
|---|---|
| `style/style_ref.png` | ✅ JRPG mood anchor — for sprites & narrative scenes |
| `style/pixel_technique_anchor.png` | ✅ Neutral 3-building pixel-tech anchor — for buildings |
| `buildings_3d/*.png` | ✅ **27/27 done**: 中正圖書館, 井塘樓, 商學院, 四維堂, 大仁樓, 大勇樓, 大智樓, 學思樓, 志希樓, 操場, 新聞館, 果夫樓, 校友服務中心, 樂活小舖, 樂活館, 正門, 法學院, 游泳館, 研究大樓, 綜合院館, 羅馬廣場, 行政大樓, 資訊大樓, 集英樓, 風雩樓, 風雩走廊, 體育館 |
| `maps/worldmap_base.png` | ✅ 1024² NB-generated empty terrain (roads, river, forest, plaza, sports placeholder) |
| `maps/worldmap.png` | ✅ 2048² composite of all 27 building tiles on the base, via `tools/composite_worldmap.py` |
| `general_building.png` | ⚠️ Older illustrated-mood version of 綜合院館 — superseded by new tile, kept for narrative entry-screen if desired |
| Player sprites (4-dir × 3 frames) | ⏳ Not started |
| 4 transparent umbrella icons (True/Fragile/ProfessorTrap/Cursed) | ⏳ Not started |
| NPC sprites (西裝學長, 學霸, 助教, 福利社阿姨, 苦主) | ⏳ Not started |
| UI frames (dialog box, HUD, rain meter) | ⏳ Not started |
| Cumulative Nano Banana spend | ~$1.20 USD across both sessions |

### Code
| Item | State |
|---|---|
| `src/main.cpp` | Empty 800×450 raylib window, no game logic |
| `GameObject` abstract + `Character` / `Item` subtree | ⏳ Not started |
| `Player` (WASD + rainMeter + karma + Inventory) | ⏳ Not started |
| `GameObjectFactory` (Assignment 5 grading must-have) | ⏳ Not started |
| `SemesterStateMachine` (5 chapters + 3 endings) | ⏳ Not started |
| `EventBus` / Observer for UI ↔ Data decoupling | ⏳ Not started |
| `TransparentUmbrella` family (4 derived classes) | ⏳ Not started |
| `tools/composite_worldmap.py` | ✅ Working — adjust `BUILDINGS` dict + rerun to reposition |
| UML diagram | Mermaid in design doc; needs export to PNG for submission |
