# Cycle 9.D — Accessibility Baseline Audit

**HEAD**: f5b9e65 (Cycle 9.D.1, 328/328 tests passing)
**Branch**: claude/restore-project-state-6GSBZ
**Method**: source code grep + Read; no screenshot inspection
**Scope**: code paths under `src/` `include/`; content under `docs/content/`
**Standard**: WCAG 2.2 AA where applicable; SC numbers cited.

## Executive Summary

- **BLOCKING (must fix for AA)**: 2 — D3 contrast (DarkGray hints on RayWhite & dialog `▼` marker), D4 keyboard remap (WASD / E / Tab / Esc / M all hardcoded; no input rebinding layer)
- **HIGH (should fix)**: 3 — D1 fixed font sizes 14–18 px (no scale slider), D5 dialog typewriter / skip ergonomics absent (advance only), D11 rain meter forces pace (timed pressure non-disable-able)
- **MEDIUM / LOW**: 3 — D2 quest-giver indicator is colour-only (Gold #FFC83D, no shape redundancy), D7 NPC hitbox 24×24 below SC 2.5.8 minimum, D8 no reduced-motion flag (rain vignette / interlude marker sweep / toast fade non-adjustable)
- **PASSING / NA-by-design**: 3 — D6 audio (no audio system; NA), D9 state.jsonl is a structured a11y export (passing), D10 CJK strings already mostly mediated via `docs/content/*.md` + a `VendorMessages.h` indirection layer (partial; passing on dialogue, gap on UI chrome)

Total candidates queued for Cycle 9.E+: 8.

---

## 11 Dimension Findings

### 1. 字級可讀性 (SC 1.4.4 Resize Text)

- **現狀**:
  - `include/DialogLayout.h:56` — `kBoxFontSize = 16` for the main dialog box (28-cell wrap).
  - `src/MessageView.cpp:18` — `kFontSize = 18` for the bottom-centre toast.
  - `src/View.cpp:157` — HUD `kHudSize = 16`.
  - `src/View.cpp:285` — objective panel `kObjSize = 14.0f`.
  - `src/View.cpp:324,382,400,413` — affordance / menu hints at 14 px; `遊戲選單` title 28; `說明` body 16; help title 26.
  - `include/QuestGiverIndicator.h:41` — quest-giver "!" glyph 14 px.
  - `include/gfx/Font.h:216` — atlas rasterises CJK at `kFontSize = 32` then scales down via `DrawTextEx`, so glyphs survive scaling.
- **缺口**: every consumer hard-codes its own constant; no central UI scale slider, no per-user multiplier, no settings persistence. 14 px CJK at 800-wide window is on the edge of legibility for older players; SC 1.4.4 expects ≥200 % resize without loss of content. Nothing in the codebase routes through a `kUiScale` multiplier.
- **Severity**: **HIGH** (not blocking because the font atlas itself rasterises at 32 — there is no quality cliff if multipliers are added later; but the lack of any user control fails AA verbatim).
- **修法草案**: add `gfx/UiScale.h` with `static float Get() noexcept` + `static void Set(float)` (clamped 0.75–2.0). Multiply every `Size(N)` call site, every `kFontSize` and `kBoxFontSize` reference, and layout `kBoxH / kBoxLineH / kHudY / kPanelW / kPanelH` by `UiScale::Get()`. Add a pause-menu fifth row "字級 +/-" using ← → at the `說明` cursor level.
- **工作量**: ~120 lines spread across `View.cpp` (~40), `DialogView.cpp` (~10), `MessageView.cpp` (~10), `InventoryView.cpp` (~10), `CharacterSelect.cpp` / `TitleScreen.cpp` (~25), new `UiScale.h` (~25), pause-menu wiring (~10). One new doctest in `tests/test_ui_scale.cpp`.

### 2. 色盲友善 (SC 1.4.1 Use of Color)

- **現狀**:
  - `include/QuestGiverIndicator.h:58` — Gold `#FFC83D` (255,200,61) square + Black `!` glyph for every quest-giver.
  - `src/View.cpp:240-242` — rain HUD line ramps `Red @≥85` / `Gold @≥60` / `White` only.
  - `src/View.cpp:259-268` — rain vignette is pure value (black at α=45 / 90); luminance-redundant by design.
- **缺口**:
  - Quest-giver indicator IS shape-redundant (a `!` glyph sits inside the gold square; the `!` itself is the affordance, the colour is amplification). Per Web Almanac / WCAG analysis on Gold = 255,200,61, deuteranopes still see it shift toward dull yellow; protanopes lose saturation but the black glyph contrasts (`!` reads at ~17:1 vs Gold). So this is borderline OK.
  - The rain HUD line uses **colour alone** (white→gold→red) to signal three pressure tiers. A deuteran/protan viewer will perceive gold and red as near-identical olive/brown. The 4-band vignette IS shape-redundant (its appearance itself signals stress) so the visual channel is not solely colour, but the text readout is colour-only.
- **Severity**: **MEDIUM** — `rain: 60%` and `rain: 85%` digits ARE still readable; the colour is amplification not a substitute. But add a glyph (✓ / ! / !!) to be fully SC 1.4.1 compliant.
- **修法草案**: in `src/View.cpp:243` `TextBuilder{rbuf}` prefix the buffer with `"  "`, `" !"`, or `"!!"` derived from `rm`. Pure cosmetic, no sim change.
- **工作量**: ~6 lines in `src/View.cpp`, one regression assertion via spy renderer in `test_rain_hud_redundant.cpp` (~30 lines).

### 3. WCAG 對比度 (SC 1.4.3 Contrast (Minimum))

- **現狀** (estimated; the IRenderer spy could record exact RGBA but a screenshot pass is out of scope per the brief):
  - **HUD panel**: `Color{20,22,30,185}` background, `Colors::White` text → ~14:1 luminance ratio on the panel. **PASS**.
  - **HUD karma/chapter `Colors::Gold` on the same dark panel** → ~10:1. **PASS**.
  - **Dialog box**: `Colors::RayWhite` (245,245,245) background, `Colors::Black` text → ~19:1. **PASS**.
  - **Dialog `▼` more-marker**: `Colors::DarkGray` (80,80,80) on RayWhite → ~5.3:1. PASS (large-text bar is 3:1, normal-text bar is 4.5:1; 16 px CJK is borderline non-large).
  - **Pause-menu hints**: `Colors::DarkGray` on `Color{20,22,30,230}` panel → catastrophic `~1.05:1` — the dark-grey text on near-black panel is essentially invisible (`src/View.cpp:382` `↑ ↓ 選擇   Enter 確認   ESC 繼續` and `src/View.cpp:413` `ESC / E 返回選單`). **FAIL — BLOCKING**.
  - **Vendor decline "先不買，謝謝"**: rendered through `DialogView` as `Colors::Black` on RayWhite — PASS.
- **缺口**: every `.Color(Colors::DarkGray)` call onto a dark panel fails AA. Two known sites in `src/View.cpp` (pause-menu hint, help-overlay hint).
- **Severity**: **BLOCKING**.
- **修法草案**: replace `Colors::DarkGray` at `src/View.cpp:382` and `src/View.cpp:413` with `Color{180,180,180,255}` (~7:1 against the `20,22,30` panel — AAA-large, AA-normal). The dialog `▼` is on RayWhite so its DarkGray stays. Add a doctest asserting the spy never records `DarkGray` for any text whose backing rect is darker than mid-grey.
- **工作量**: ~4 lines + ~40 line regression test.

### 4. 鍵盤 remapping (SC 2.1.1 Keyboard / 2.5.4 Motion Actuation)

- **現狀**:
  - `src/Player.cpp:92-95` — W/A/S/D for movement, hardcoded enum `Key::W/A/S/D`.
  - `src/GameController.cpp:193,201-204,208,214,259-262,344,432` — Esc/M for menu, ↑/↓ for cursor, Enter for confirm, E for interact/advance, Tab for inventory.
  - `src/TitleScreen.cpp:49-94`, `src/CharacterSelect.cpp:64-75` — same pattern at the menus.
  - `include/gfx/Input.h:30-46` — `Input` is a thin static façade over an `InputSource`; the harness already swaps it for `ScriptInput`. **There is no `KeyAction → Key` lookup table — every call site asks for a physical key directly.**
- **缺口**: a left-handed player, an arrow-keys-only player, a single-hand player, or a player on a non-QWERTY layout (AZERTY: ZQSD vs WASD) cannot remap. SC 2.1.1 Level A requires keyboard operability; SC 2.5.4 Level A says single-key actuators must be remappable. The Input layer is one indirection short of supporting this — perfect refactor target.
- **Severity**: **BLOCKING** (Level A, not AA). Movement must be remappable.
- **修法草案**: introduce `enum class Action { MoveUp, MoveDown, MoveLeft, MoveRight, Interact, Cancel, Menu, Inventory, ChoiceUp, ChoiceDown, Confirm }` and `KeyBindings` map. `Input::IsDown(Action)` reads the bind. Persist to `~/.config/尋傘記/keybinds.json` (or a sibling of `state.jsonl`). Default binds = current behaviour; harness-friendly (the ScriptInput layer is untouched).
- **工作量**: ~200 lines: new `KeyBindings.h/.cpp` (~80), refactor 17 call sites listed above (~70), pause-menu "鍵位設定" overlay (~50). Two new doctest files.

### 5. 暫停 / 跳過 (SC 2.2.1 Timing Adjustable / 2.2.2 Pause, Stop, Hide)

- **現狀**:
  - Pause: `src/View.cpp:346-383` + `src/GameController.cpp:189-237` — ESC / M opens the menu, sim is fully frozen (HUD message also frozen since `world_.TickHud(dt)` is past the early-return on line 354 — verified in code comment `src/GameController.cpp:348-353`). **Pause exists and is robust**.
  - Dialog: `src/DialogView.cpp:31-53` paints the FULL page each frame — there is **no typewriter / per-character reveal**, so there is nothing to "skip past". The whole row is on screen at once; E (`src/GameController.cpp:262`) advances the page. No way to fast-forward through a multi-page line beyond mashing E.
  - Toast: `kHudTtl = 4.0f` (`include/MessageView.h:13`) auto-dismisses; no hide-now button.
- **缺口**: SC 2.2.2 expects the player to be able to pause/stop/hide moving or auto-updating content. The 4-second toast (`MessageView`) and the W→E sweep of the interlude marker (`src/View.cpp:142-144`, 30 px/s phase advance) are auto-animations the player cannot pause once started. Mash-to-skip on dialog is functional but a per-line "hold E to fast-forward × N" would respect SC 2.2.3 better.
- **Severity**: **HIGH** — the pause menu already exists (the hardest infra is built); the missing piece is per-element pause controls.
- **修法草案**:
  - (a) Hold E ≥ 0.3 s in a dialog → advance every frame instead of waiting for the next press. Code site: `src/GameController.cpp:262` — replace the edge-triggered `Input::IsPressed(Key::E)` with `IsPressed || (IsDown && holdMs > 300)` plus a 4 px/frame guard on auto-advance.
  - (b) Skip-toast key (Backspace or X): when a toast is up, dismisses it. Site: a check next to `src/GameController.cpp:354` `world_.TickHud(dt)`; if pressed, `world_.SetHudAge(kHudTtl)`.
- **工作量**: ~30 lines + one test.

### 6. 音效 (SC 1.4.2 Audio Control)

**N/A (by design)** — the project ships with no audio system. `grep -ri "PlaySound\|LoadSound\|LoadMusic\|SetSoundVolume" src/ include/` returns zero hits. No background music, no SFX, no spoken dialogue. SC 1.4.2 requires audio control only if audio exists. Caveat: this is *itself* an accessibility concern in the opposite direction (visual content with no audio fallback), but the GDD positions the game as silent-by-design.

If audio is added later (e.g., a rain-loop layer to amplify the rain meter), the same `World::SetMenuOpen(true)` freeze should also call `PauseAllSounds()`, and an `audio: on/off` toggle must live in the pause menu. **No work needed in Cycle 9.E.**

### 7. 目標尺寸 (SC 2.5.8 Target Size (Minimum))

- **現狀**:
  - Player & NPC hit-box: `include/WorldConfig.h:11-12` — `kPlayerWidth = kPlayerHeight = 24.0f` px.
  - Interaction reach: `src/GameController.cpp:447` + `src/ScriptInput.cpp:302,352` — `kInteractReach = 8.0f`, so the effective E-probe box is **40 × 40 px**.
- **缺口**: SC 2.5.8 sets a 24 × 24 CSS-pixel minimum, applied to **pointer** targets. Keyboard-driven 2D RPGs are technically exempt (the player aims via positioning their avatar, no pointer), but the *intent* of the SC — "small targets are unusably precise" — still bites for tremor-affected users approaching an NPC. The 40 × 40 probe is healthy here; the literal 24 × 24 hitbox is the floor, not the operative target.
- **Severity**: **MEDIUM** (technically passing because reach is 40 × 40, not blocking).
- **修法草案**: if a user opts into a "larger targets" accessibility profile, bump `kInteractReach` to 16 (effective 56 × 56 probe). Pause-menu toggle "易接觸 (擴大互動範圍)".
- **工作量**: ~15 lines (settings flag + read in `GameController.cpp:447` & `ScriptInput.cpp:302,352`).

### 8. 減少動畫 (SC 2.3.3 Animation from Interactions)

- **現狀** (every motion the code drives):
  - Typewriter: NONE (text is full-page each frame; `src/DialogView.cpp:42-43`).
  - Rain vignette: 4 black bands at α=45 or 90, NO animation — drawn each frame from `GetRainMeter()` (`src/View.cpp:264-267`). Static; **safe**.
  - Interlude exit marker: phase ticks `Time::DeltaSeconds() * 30.0f` px/s (`src/View.cpp:142`); a dashed gold line sweeps W→E. Continuous motion ~12 frames per 60-fps second; on-screen only inside Interlude_Market.
  - Player walk anim: `include/Player.h:147` `animTimer_` / `animStep_` cycle 4 frames; cosmetic.
  - HUD toast fade: `kHudTtl = 4 s`, `kHudFade = 1 s` (linear `t = (kHudTtl - age) / kHudFade`). Fades affect ~one banner-sized region.
  - Ending fade-in: `View::endingAlpha_ += dt` to 1.0 (`src/View.cpp:64-65`). One-shot, ~1 s.
- **缺口**: nothing flashes ≥ 3 Hz (the marker phase is continuous motion, not flicker), so SC 2.3.1 is safe. But SC 2.3.3 (AAA, but a UX hygiene baseline) asks for a "reduced-motion" preference. Currently every animation is unconditional.
- **Severity**: **MEDIUM** — the player can already pause the world with ESC, so animations are always interruptible at the system level.
- **修法草案**: a single `bool ReducedMotion()` flag; the interlude marker becomes static (no phase tick), the toast fade ends in 1 frame, ending-card uses `endingAlpha_ = 1.0f` instantly. Pause-menu toggle "減少動畫".
- **工作量**: ~20 lines (flag in `World`, three sites in `View.cpp`, one in `MessageView.cpp`).

### 9. Screen reader (SC 4.1.2 / 1.3.1)

- **現狀**: the harness emits `state.jsonl` per frame with player position, karma, money, rain meter, building, dialog active+npc+line+choices+cursor, HUD text, active object/NPC ids, and per-frame events (`src/Harness.cpp:122-204`). This is a structured, line-oriented machine-readable view of every piece of on-screen state.
- **缺口**: not exposed by default — `UMBRELLA_STATE` env var must be set for the file to exist (the harness is harness-only, not live during normal play). To meet SC 4.1.2 for assistive tech, you'd want a similar emitter wired to a UNIX FIFO or AT-SPI bridge in live runs.
- **Severity**: **PASSING for the harness use case** (it's an excellent structured export — better than most indie engines achieve), but if a blind player wanted to play live, they couldn't.
- **修法草案** (optional, low priority): add an `UMBRELLA_LIVE_STATE=/tmp/umbrella.fifo` mode that writes the same JSONL to a named pipe even when no script is set. A companion `tools/say_state.py` could `tail -f` the pipe and `espeak`-vocalise events.
- **工作量**: ~30 lines in `Harness.cpp` (decouple emitter activation from script-presence) + ~80 line Python sidecar (not a project asset).

### 10. i18n (SC 3.1.1 Language of Page)

- **現狀**:
  - Dialogue: 100 % runtime-loaded from `docs/content/chapter[1-4].md` + `interlude_market.md` + `ending_[abc].md` + `voice_bible.md` via `DialogLoader.cpp`. Per CLAUDE.md §2 this is the source of truth — adding a `_en.md` sibling and a locale switch in the loader would translate dialogue without touching C++.
  - Vendor strings: `include/VendorMessages.h:8` — `// constants also makes a future i18n table a drop-in swap.` — indirection layer already exists.
  - **Hardcoded CJK in C++**: `src/InventoryView.cpp:28,42` (`物品欄`, `（空）`), `src/View.cpp:366,380,398,411` (`繼續/說明/重新開始/離開`, `↑ ↓ 選擇   Enter 確認   ESC 繼續`, `遊戲說明`, `ESC / E 返回選單`), `src/View.cpp:171,201` (`金幣: %d 元`), `src/View.cpp:326` (`ESC 選單`), `src/GameObjectFactory.cpp:28` (`市集攤主`, `歡迎光臨`), `src/DialogOpener.cpp:309` (`（他不再說話，繼續巡考）`).
- **缺口**: about 15 UI-chrome strings live in source. A drop-in `Locale::Get("menu.resume")` table would liberate them. Dialogue itself is already table-driven — the bulk of the localisation work is *not* in C++.
- **Severity**: **LOW** (single-language by design for an OOP assignment).
- **修法草案**: add `include/Locale.h` with `std::string_view Get(std::string_view key) noexcept`, lookup table in `src/Locale.cpp` indexed by enum, default = zh-TW. Replace the ~15 hardcoded literals with `Locale::Get(LocaleKey::MenuResume)` etc.
- **工作量**: ~120 lines: new `Locale.h/.cpp` (~80), 15 site rewrites (~30), one doctest (~10).

### 11. 遊戲速度 / 時限 (SC 2.2.1 Timing Adjustable)

- **現狀**:
  - **No chapter timer** — Semester progression is gated on `Flag_*` not on a real-time deadline. `grep -rn timer_\|deadline\|TimeLimit src include` confirms zero hits beyond the `kHudTtl` toast TTL.
  - **Rain pressure IS a soft real-time deadline**: `src/Player.cpp:140` `+5.0f * dt` when exposed → 100 % in 20 s; `:148` `-10.0f * dt` indoors → empty in 10 s; `:162` umbrella-but-exposed `+1.5f * dt` → 100 % in ~67 s. Hitting 100 % teleports the player to the front gate (`src/Player.cpp:141,163`) and resets the meter.
  - Fixed game step: `src/Harness.cpp:318` `gfx::Time::SetFixedStep(1.0f / 60.0f)` (harness only); live play uses raylib's vsync.
  - World pause via ESC works (D5).
- **缺口**: the rain meter is an unmodifiable timed pressure. A player who needs to step away mid-Ch2 with no umbrella will be teleported. SC 2.2.1 Level A requires the player be able to extend or disable any timing < 20 hours that affects the experience.
- **Severity**: **HIGH** — Level A violation in the strict reading. Pausing with ESC freezes the meter (the GC early-returns before `ApplyRain`), which DOES satisfy SC 2.2.1 ("user can pause") provided the player KNOWS that pressing ESC pauses the rain. Add this hint to the help overlay.
- **修法草案**:
  - (a) Help-overlay line addition (`include/GameHelp.h`, `kGameHelpLines`): "ESC 暫停會凍結雨壓力計". 1 line of content, no code change.
  - (b) Pause-menu toggle "降低雨速 (x0.5)" → scales `5.0f / 1.5f` rain-accrual rates in `src/Player.cpp:140,162` by 0.5 when on. 4 lines of code.
  - (c) Optional accessibility profile "關閉雨懲罰": ApplyRain becomes a no-op for `lethal=false` callers when set. 6 lines.
- **工作量**: ~20 lines + 1 doctest.

---

## Cycle 9.E+ 候選工作項

| 優先 | 維度 | 修法 | 入手 file:line | 工作量 |
|------|------|------|----------------|--------|
| B1 | D3 對比度 | `DarkGray` text on dark panels → `Color{180,180,180,255}`; add spy regression | `src/View.cpp:382,413` | ~4 LOC + 40 test |
| B2 | D4 鍵盤 remap | `Action` enum + `KeyBindings` table; refactor 17 IsDown/IsPressed call sites; pause-menu remap UI | `include/gfx/Input.h:30`; 17 sites listed in §4; new `KeyBindings.h/.cpp` | ~200 LOC |
| H1 | D1 字級 | `gfx/UiScale.h` multiplier; all `Size(N)` & `kBoxFontSize/kHudSize/kObjSize` scale; pause-menu `字級 +/-` | new `UiScale.h`; ~10 View sites | ~120 LOC |
| H2 | D5 跳過 | hold-E to fast-advance dialog; Backspace to skip toast | `src/GameController.cpp:262,354`; `src/World.cpp` `SetHudAge` | ~30 LOC |
| H3 | D11 雨速 | help-line note + pause-menu "降低雨速" toggle; scale `Player.cpp:140,162` accrual | `src/Player.cpp:140,162`; `include/GameHelp.h`; `src/View.cpp` menu | ~20 LOC |
| M1 | D2 色盲 | append "  " / " !" / "!!" prefix to rain HUD text | `src/View.cpp:243` | ~6 LOC + 30 test |
| M2 | D7 目標尺寸 | accessibility-profile flag → bump `kInteractReach` to 16 | `src/GameController.cpp:447`; `src/ScriptInput.cpp:302,352` | ~15 LOC |
| M3 | D8 減少動畫 | `World::ReducedMotion()` flag; gate marker phase / toast fade / ending fade | `src/View.cpp:142-144,64-65`; `src/MessageView.cpp:91` | ~20 LOC |
| L1 | D10 i18n | `Locale.h/.cpp` table for ~15 UI-chrome strings | `src/View.cpp:171,201,326,366,380,398,411`; `src/InventoryView.cpp:28,42`; `src/GameObjectFactory.cpp:28`; `src/DialogOpener.cpp:309` | ~120 LOC |
| L2 | D9 SR live | live-mode `state.jsonl` emitter on a FIFO regardless of `UMBRELLA_SCRIPT` | `src/Harness.cpp` activation guard | ~30 LOC |

**Recommended Cycle 9.E sequence**: B1 (1 day, unblocks AA visually) → H2 (0.5 day, biggest UX win for the time invested) → H3 (0.5 day) → M1+M3 (0.5 day together) → B2 (2–3 days, the structural one).

---

## Methodology notes

- All findings traced to a `file:line` anchor; the `grep` corpus covers every `IsKeyDown/IsKeyPressed`, every `Size(N)` / `kFontSize` / `kBoxFontSize`, every `DrawText` site, and every CJK-literal hit returned by the spot-grep `繼續|說明|不買|遊戲說明|歡迎`.
- Contrast ratios estimated using the IEC 61966 luminance formula on the constant `Color{R,G,B,A}` tuples in `include/gfx/Color.h` and the panel backings in `src/View.cpp`. No screenshot inspection performed.
- WCAG SC numbering: 2.2 latest (2023). Level A violations flagged as BLOCKING; Level AA failures as HIGH; AAA / nice-to-have as MEDIUM/LOW.
- Audio is unambiguously NA-by-design: no `PlaySound`, `LoadSound`, `LoadMusic`, `SetSoundVolume` or `InitAudioDevice` calls in the entire `src/` `include/` tree.

DONE.
