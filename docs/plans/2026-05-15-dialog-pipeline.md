# Dialog Pipeline (Phase 0) Implementation Plan

> **For agentic workers:** Implement task-by-task. Steps use checkbox (`- [ ]`)
> syntax. Each task ends with build + test + commit. Work in the worktree;
> never `git push`; never `rm -rf build`; run the forbidden-word scan before
> any handoff. SoT = `docs/ROADMAP.md` (commit 12657c9).

**Goal:** Turn the 2745-line `docs/content/*.md` script into a compiled,
queryable `include/DialogData.h` and wire `NPC` to consume it, so later
phases can drive real conversations instead of placeholder lines.

**Architecture:** Configure-time codegen. A Python script parses the
markdown into a generated C++ header of `constexpr` tables keyed by
`(npcId, SemesterState, subState)`. The generated header is committed to
git so the grader's `cmake` build never needs Python. A doctest TU includes
the header and asserts known facts from the script. `NPC` gains a loader
that pulls the right line set for a given state.

**Tech Stack:** Python 3 (stdlib only), C++17, doctest, CMake 3.14+.

**Scope note:** This plan covers **Phase 0 only** as full bite-sized tasks.
Phase 1 (DialogBox UI + DialogSession + Ch1 quest) and Phase 2 (full
campaign) are independent subsystems that depend on Phase 0 landing; they
are listed as task-level outlines at the end and get their own bite-sized
plans once Phase 0 is green.

---

## Markdown grammar the parser must recognise

Verified against `docs/content/chapter1.md` and `voice_bible.md`:

- A file's state key: the line `- SemesterState: \`Chapter1_AddDrop\``
  under `## 章節 metadata`. Files without it (`voice_bible.md`) are
  reference-only and **not** parsed for dialog.
- `## NPC：<name>` opens an NPC block. `<name>` is the trimmed text after
  the full-width colon `：`.
- `### (a) ...` / `### (b) ...` / `### (c) ...` / `### (d) ...` open a
  sub-state. Map letter → int: a→0, b→1, c→2, d→3.
- Inside a sub-state, a top-level bullet `- "<text>"` (double-quoted) is a
  dialog line. Order preserved → `lines[0..N]`.
- Blockquote lines `> ...` are author notes. Two are semantic:
  - `> \`// karma +N\`` or `> \`// karma -N\`` → that sub-state's
    `karmaDelta` (default 0). Take the first match in the block.
  - `> \`Flag_Xxx = true\`` / `> \`Flag_Xxx = false\`` → records the flag
    name and the boolean it should be set to. Take the first match.
- `## 場景旁白` is a non-NPC block; its `### 開場` / `### 章節清關` etc.
  bullets are narration. Key them under the pseudo-npc id `__scene__`
  with subState by section order. `### 系統訊息` nested bullets
  (`\`rainMeter ≥ 50\`:` then indented bullets) are skipped in Phase 0
  (rain UI is Tier 3) — parser must not crash on them.
- A line that is `---` or starts a new `##`/`###` closes the current block.

NPC name → stable ascii id (used as the `npcId` key), defined in the
script and mirrored in `gen_dialog.py`'s `NPC_ID` dict:

| 中文名 | npcId |
|---|---|
| 西裝學長 | `suit_senior` |
| 學霸 | `bookworm` |
| 助教 | `ta` |
| 福利社阿姨 | `shop_auntie` |
| 苦主 | `victim` |
| 圖書館管理員 | `librarian` |
| A系烤香腸攤主 | `vendor_a` |
| B系大聲公持有者 | `vendor_b` |
| C系學姊 | `senior_c` |
| (場景旁白) | `__scene__` |

If the parser meets a `## NPC：<name>` whose name is not in `NPC_ID`, it
must `sys.exit(1)` with the offending name — fail loud, never emit a
partial header.

---

## File Structure

- Create `tools/gen_dialog.py` — the parser/codegen (stdlib only).
- Create `include/DialogData.h` — generated, committed. One responsibility:
  static dialog tables + a `For()` query.
- Create `tests/test_dialog_data.cpp` — doctest TU asserting known script
  facts through the generated header.
- Modify `CMakeLists.txt` — add an optional `gen_dialog` convenience
  target (NOT on the build critical path).
- Modify `include/NPC.h` / `src/NPC.cpp` — add
  `NPC& LoadDialog(std::string_view npcId, SemesterState s)`.
- Create `tests/test_npc_loaddialog.cpp` — doctest for the loader.

---

## Task 1: Codegen script — parse one chapter, emit header

**Files:**
- Create: `tools/gen_dialog.py`
- Create: `include/DialogData.h` (script output, committed)

- [ ] **Step 1: Write `tools/gen_dialog.py`**

```python
#!/usr/bin/env python3
"""Parse docs/content/*.md narrative scripts into include/DialogData.h.

Stdlib only. Run from repo root:  python3 tools/gen_dialog.py
Regenerates include/DialogData.h in place; commit the result.
"""
import re
import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
CONTENT = ROOT / "docs" / "content"
OUT = ROOT / "include" / "DialogData.h"

NPC_ID = {
    "西裝學長": "suit_senior", "學霸": "bookworm", "助教": "ta",
    "福利社阿姨": "shop_auntie", "苦主": "victim",
    "圖書館管理員": "librarian", "A系烤香腸攤主": "vendor_a",
    "B系大聲公持有者": "vendor_b", "C系學姊": "senior_c",
}
SCENE_ID = "__scene__"
SUB = {"a": 0, "b": 1, "c": 2, "d": 3}

STATE_RE = re.compile(r"SemesterState:\s*`([A-Za-z0-9_]+)`")
NPC_RE = re.compile(r"^##\s*NPC：(.+?)\s*$")
SCENE_RE = re.compile(r"^##\s*場景旁白\s*$")
SUBSEC_RE = re.compile(r"^###\s*\(([a-d])\)")
SCENE_SUBSEC_RE = re.compile(r"^###\s+(.+?)\s*$")
LINE_RE = re.compile(r'^-\s*"(.*)"\s*$')
KARMA_RE = re.compile(r"//\s*karma\s*([+-]\d+)")
FLAG_RE = re.compile(r"\b(Flag_[A-Za-z0-9_]+)\s*=\s*(true|false)\b")


class Entry:
    def __init__(self, npc_id, state, sub):
        self.npc_id = npc_id
        self.state = state
        self.sub = sub
        self.lines = []
        self.karma = 0
        self.flag = ""        # "" = none
        self.flag_val = False


def parse_file(path):
    state = None
    entries = []
    cur = None
    cur_npc = None
    scene_sub = -1
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.rstrip()
        m = STATE_RE.search(line)
        if m and state is None:
            state = m.group(1)
        if line.startswith("## ") or line.startswith("# "):
            cur = None
            nm = NPC_RE.match(line)
            if nm:
                name = nm.group(1).strip()
                if name not in NPC_ID:
                    sys.exit(f"unknown NPC name in {path.name}: {name!r}")
                cur_npc = NPC_ID[name]
            elif SCENE_RE.match(line):
                cur_npc = SCENE_ID
                scene_sub = -1
            else:
                cur_npc = None
            continue
        if line.startswith("### "):
            if cur_npc is None:
                cur = None
                continue
            sm = SUBSEC_RE.match(line)
            if cur_npc == SCENE_ID:
                scene_sub += 1
                cur = Entry(cur_npc, state, scene_sub)
                entries.append(cur)
            elif sm:
                cur = Entry(cur_npc, state, SUB[sm.group(1)])
                entries.append(cur)
            else:
                cur = None
            continue
        if cur is None:
            continue
        lm = LINE_RE.match(line)
        if lm:
            cur.lines.append(lm.group(1))
            continue
        if line.startswith(">"):
            km = KARMA_RE.search(line)
            if km and cur.karma == 0:
                cur.karma = int(km.group(1))
            fm = FLAG_RE.search(line)
            if fm and not cur.flag:
                cur.flag = fm.group(1)
                cur.flag_val = fm.group(2) == "true"
    return state, [e for e in entries if e.lines]


def cpp_escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def main():
    files = sorted(CONTENT.glob("chapter*.md")) + \
        sorted(CONTENT.glob("interlude*.md")) + \
        sorted(CONTENT.glob("ending_*.md"))
    all_entries = []
    for f in files:
        state, ents = parse_file(f)
        if state is None:
            sys.exit(f"no SemesterState in {f.name}")
        all_entries.extend(ents)
    if not all_entries:
        sys.exit("parsed zero dialog entries — refusing to emit empty header")

    out = []
    out.append("// GENERATED by tools/gen_dialog.py — DO NOT EDIT BY HAND.")
    out.append("// Regenerate: python3 tools/gen_dialog.py")
    out.append("#ifndef DIALOG_DATA_H_")
    out.append("#define DIALOG_DATA_H_")
    out.append('#include "SemesterState.h"')
    out.append("#include <string_view>")
    out.append("#include <cstddef>")
    out.append("")
    out.append("namespace nccu::dialog {")
    out.append("")
    out.append("// C++17: no std::span. lines is a pointer + count into a")
    out.append("// per-entry constexpr string_view array below.")
    out.append("struct Entry {")
    out.append("    std::string_view npcId;")
    out.append("    SemesterState    state;")
    out.append("    int              subState;")
    out.append("    const std::string_view* lines;")
    out.append("    int              lineCount;")
    out.append("    int              karmaDelta;")
    out.append("    std::string_view setsFlag;   // \"\" = none")
    out.append("    bool             flagValue;")
    out.append("};")
    out.append("")
    for i, e in enumerate(all_entries):
        joined = ", ".join(f'"{cpp_escape(x)}"' for x in e.lines)
        out.append(f"inline constexpr std::string_view kL{i}[] = "
                   f"{{ {joined} }};")
    out.append("")
    out.append("inline constexpr Entry kEntries[] = {")
    for i, e in enumerate(all_entries):
        flag = cpp_escape(e.flag)
        out.append(
            f'    {{ "{e.npc_id}", SemesterState::{e.state}, {e.sub}, '
            f'kL{i}, {len(e.lines)}, {e.karma}, "{flag}", '
            f'{"true" if e.flag_val else "false"} }},')
    out.append("};")
    out.append("")
    out.append(f"inline constexpr std::size_t kEntryCount = "
               f"{len(all_entries)};")
    out.append("")
    out.append("struct EntryRange {")
    out.append("    const Entry* b;")
    out.append("    const Entry* e;")
    out.append("    const Entry* begin() const { return b; }")
    out.append("    const Entry* end()   const { return e; }")
    out.append("};")
    out.append("inline EntryRange All() {")
    out.append("    return { kEntries, kEntries + kEntryCount };")
    out.append("}")
    out.append("")
    out.append("} // namespace nccu::dialog")
    out.append("#endif // DIALOG_DATA_H_")
    OUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {OUT} — {len(all_entries)} entries")


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Run the script**

Run: `cd /Users/ian/Desktop/assignment-5-jiangjiangian/.claude/worktrees/agent-a02d0134897989b79 && python3 tools/gen_dialog.py`
Expected: prints `wrote .../include/DialogData.h — <N> entries` with N > 30.
If it exits with `unknown NPC name`, add that name→id to `NPC_ID`
(check the script for the exact string) and rerun.

- [ ] **Step 3: Sanity-check the generated header**

Run: `grep -c 'SemesterState::Chapter1_AddDrop' include/DialogData.h`
Expected: a non-zero count.
Run: `grep -n 'suit_senior".*Chapter1_AddDrop.*2,' include/DialogData.h`
Expected: a line for 西裝學長 sub-state (c) — the one carrying
`karmaDelta` `-5` per `chapter1.md`.

- [ ] **Step 4: Commit**

```bash
git add tools/gen_dialog.py include/DialogData.h
git commit -m "feat(dialog): codegen script + generated DialogData.h"
```

---

## Task 2: doctest proving the generated header round-trips the script

**Files:**
- Create: `tests/test_dialog_data.cpp`

- [ ] **Step 1: Write the failing test**

> Do NOT add `DOCTEST_CONFIG_*` or `int main` — `tests/test_eventbus.cpp`
> already owns the single doctest main for the whole test executable.

```cpp
#include "doctest/doctest.h"
#include "DialogData.h"
#include "SemesterState.h"
#include <string_view>

namespace {
const nccu::dialog::Entry* find(std::string_view npc,
                                SemesterState s, int sub) {
    for (const auto& e : nccu::dialog::All())
        if (e.npcId == npc && e.state == s && e.subState == sub)
            return &e;
    return nullptr;
}
}

TEST_CASE("Ch1 西裝學長 (a) has the 5 opening lines") {
    const auto* e = find("suit_senior",
                         SemesterState::Chapter1_AddDrop, 0);
    REQUIRE(e != nullptr);
    CHECK(e->lineCount == 5);
    CHECK(e->lines[0] == std::string_view{"欸，加退選也沒搶到嗎？"});
}

TEST_CASE("Ch1 西裝學長 (c) carries karma -5") {
    const auto* e = find("suit_senior",
                         SemesterState::Chapter1_AddDrop, 2);
    REQUIRE(e != nullptr);
    CHECK(e->karmaDelta == -5);
}

TEST_CASE("every entry has at least one line and a valid state") {
    for (const auto& e : nccu::dialog::All()) {
        CHECK(e.lineCount >= 1);
        CHECK(e.subState >= 0);
        CHECK(e.subState <= 3);
    }
}
```

- [ ] **Step 2: Configure + build the test, verify it compiles & runs**

Run:
```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build
```
Expected: builds with zero project warnings. (GLOB picks up the new
header and test automatically — re-run `cmake -B build` so the
CONFIGURE_DEPENDS glob refreshes.)

- [ ] **Step 3: Run the suite**

Run: `ctest --test-dir build --output-on-failure`
Expected: all tests pass, including the three new `test_dialog_data`
cases. Total case count > 93 (previous baseline).

- [ ] **Step 4: Commit**

```bash
git add tests/test_dialog_data.cpp
git commit -m "test(dialog): assert DialogData.h round-trips chapter1 script"
```

---

## Task 3: CMake convenience target to regenerate (off critical path)

**Files:**
- Modify: `CMakeLists.txt` (append after the doctest block, before the
  resource-copy function near line 72)

- [ ] **Step 1: Add the custom target**

Append to `CMakeLists.txt`:

```cmake
# Dev convenience: regenerate include/DialogData.h from docs/content/*.md.
# NOT a build dependency — the generated header is committed, so the
# grader's build never needs Python. Run manually: cmake --build build --target gen_dialog
find_program(PYTHON3_EXECUTABLE NAMES python3 python)
if(PYTHON3_EXECUTABLE)
    add_custom_target(gen_dialog
        COMMAND ${PYTHON3_EXECUTABLE}
                "${CMAKE_SOURCE_DIR}/tools/gen_dialog.py"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Regenerating include/DialogData.h from docs/content")
endif()
```

- [ ] **Step 2: Verify the target exists and is NOT auto-built**

Run: `cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build`
Expected: builds the game + tests, does NOT run gen_dialog (it has no
dependents).
Run: `cmake --build build --target gen_dialog`
Expected: prints `wrote .../DialogData.h — <N> entries`; `git status`
shows `include/DialogData.h` unchanged (idempotent regeneration).

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build(dialog): add off-critical-path gen_dialog target"
```

---

## Task 4: `NPC::LoadDialog` — pull a state's lines from DialogData

**Files:**
- Modify: `include/NPC.h:33` (add method declaration after `SetDialogLines`)
- Modify: `src/NPC.cpp` (add include + method definition)
- Create: `tests/test_npc_loaddialog.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
#include "doctest/doctest.h"
#include "NPC.h"
#include "SemesterState.h"
#include "gfx/Vec2.h"

TEST_CASE("LoadDialog pulls suit_senior Ch1 (a) opening lines") {
    NPC npc(nccu::gfx::Vec2{0, 0}, {"placeholder"}, true);
    npc.LoadDialog("suit_senior", SemesterState::Chapter1_AddDrop, 0);
    CHECK(npc.DialogLineCount() == 5);
    CHECK(npc.CurrentLineText()
          == std::string{"欸，加退選也沒搶到嗎？"});
}

TEST_CASE("LoadDialog on a missing key leaves dialog empty") {
    NPC npc(nccu::gfx::Vec2{0, 0}, {"x"}, false);
    npc.LoadDialog("nobody", SemesterState::Ending_A, 0);
    CHECK(npc.DialogLineCount() == 0);
}
```

- [ ] **Step 2: Run, verify it fails to compile**

Run: `cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build 2>&1 | tail -5`
Expected: error — `LoadDialog` is not a member of `NPC`.

- [ ] **Step 3: Declare the method in `include/NPC.h`**

After line 33 (`NPC& SetDialogLines(...)`), add:

```cpp
    // Replace dialog from the generated DialogData table for the given
    // (npcId, state, subState). No match -> dialog cleared. Chainable.
    NPC& LoadDialog(std::string_view npcId, SemesterState state,
                    int subState = 0);
```

Add `#include "SemesterState.h"` and `#include <string_view>` to the
header's include block (after `#include <vector>` on line 7).

- [ ] **Step 4: Define the method in `src/NPC.cpp`**

Add `#include "DialogData.h"` to the include block. Append:

```cpp
NPC& NPC::LoadDialog(std::string_view npcId, SemesterState state,
                     int subState) {
    for (const auto& e : nccu::dialog::All()) {
        if (e.npcId == npcId && e.state == state &&
            e.subState == subState) {
            std::vector<std::string> lines;
            lines.reserve(static_cast<std::size_t>(e.lineCount));
            for (int i = 0; i < e.lineCount; ++i)
                lines.emplace_back(e.lines[i]);
            return SetDialogLines(std::move(lines));
        }
    }
    return SetDialogLines({});
}
```

- [ ] **Step 5: Build + run the suite**

Run: `cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && cmake --build build && ctest --test-dir build --output-on-failure`
Expected: zero project warnings; all tests pass incl. the two new
`test_npc_loaddialog` cases.

- [ ] **Step 6: Forbidden-word scan + commit**

```bash
grep -rniE 'claude|codex|anthropic|nanobanana|gemini|superpowers|CLAUDE\.md|AGENTS\.md|AI agent|GPT-|opus|sonnet' \
  tools/gen_dialog.py include/DialogData.h include/NPC.h src/NPC.cpp \
  tests/test_dialog_data.cpp tests/test_npc_loaddialog.cpp \
  && echo "DIRTY — fix before commit" || echo "CLEAN"
git add include/NPC.h src/NPC.cpp tests/test_npc_loaddialog.cpp
git commit -m "feat(npc): LoadDialog pulls lines from generated DialogData"
```

---

## Phase 0 exit gate

- `cmake --build build` — zero project warnings on `-Wall -Wextra -Wpedantic`.
- `ctest` green; case count strictly greater than the 93 baseline.
- `include/DialogData.h` committed; `python3 tools/gen_dialog.py` is
  idempotent (re-run leaves git clean).
- Forbidden-word scan CLEAN on every new/edited file.
- Not pushed. Report the worktree commit range to the user.

---

## Phase 1 — Playable spine (Ch1 end-to-end) — task outline

Gets its own bite-sized plan once Phase 0 is green. Subsystems:

1. **DialogBox (View):** a component that renders the active line set in a
   bottom panel via `IRenderer::DrawRect`+`DrawText` (placeholder frame;
   swap to `resources/assets/ui/` art when it lands). ≤28 full-width
   chars/line wrap.
2. **DialogSession (GameController):** holds active NPC*, line cursor,
   choice mode. E advances; in choice mode ↑/↓ move cursor, E confirms;
   on confirm apply `Player::SetFlag(entry.setsFlag, entry.flagValue)` +
   `Player::AddKarma(entry.karmaDelta)`. Freeze movement/collision while a
   session is open.
3. **Ch1 quest wiring:** 苦主 → 西裝學長 lead → 集英樓 → 便利店 errand
   (四維道 lost bill) → 西裝學長 2F → ripple choice A/B writes
   `Flag_HelpedSenior` / `Flag_ScoldedSenior`.
4. **Chapter gate:** acquiring `TrueUmbrella` drives
   `SemesterStateMachine` Ch1→Interlude; remove the crude
   `GameController::enterTrigger_` building-walk transition.
5. **Ending screen template:** full-screen card + fade; wire Ending C as
   the first path through it.
6. Exit gate: same common gate; a full Ch1 playthrough from
   CharacterSelect with flags + karma observably changing.

## Phase 2 — Full campaign — task outline

Own plan after Phase 1. Subsystems: Interlude market (Vendor×10 +
ConsumableItem purchase + Tab inventory UI + consume action), Ch2 library
(librarian NPC, note/drink collection, 羅馬廣場 bookworm swap), Ch3 sports
day (3-NPC barter chain → 體育館 prop box), Ch4 finals (full-map
exploration, `Flag_HelpedSenior` ripple callback at 行政大樓, TA in lab),
three ending presentations (A karma>80+TrueUmbrella, B karma<0/Cursed, C
集英樓 convenience-store ugly umbrella). All NPC lines via Phase 0
`LoadDialog`. Each chapter passes the common exit gate; ripple effect must
produce observably different Ch4 dialog.
