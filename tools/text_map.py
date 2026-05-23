#!/usr/bin/env python3
"""text_map.py — text↔flag↔code dependency map for 《尋傘記》.

Game text lives in TWO homes, which is what makes "will this dialogue logic
break?" hard to see:
  - narrative dialogue   → docs/content/*.md   (runtime-loaded by DialogLoader)
  - UI / system strings  → src/ + include/      (.h/.cpp string literals)
  - branch LOGIC          → Flag_* set in one place, read in another

This tool stitches those together. For every Flag_* it records WHERE it is
SET (a content `Flag_X = true` directive, a code SetFlag, OR a data-driven
wire — a "Flag_X" literal handed to a DialogChoice / pickup ctor) and WHERE
it is READ (HasFlag, by string literal OR a resolved `kFlag* = "Flag_*"`
constant). So editing one file shows the related files, and it surfaces:
  - SET but never READ  → dead flag / dead dialogue branch (B3 class)
  - READ but never SET  → orphan gate (a branch that can never fire)

Flag names reach SetFlag/HasFlag three ways here, all handled:
  1. string literal:   SetFlag("Flag_X") / HasFlag("Flag_X")
  2. named constant:   kFlagX = "Flag_X"; HasFlag(kFlagX)
  3. data-driven wire: DialogChoice{.., "Flag_X", ..}; SetFlag(choice.setsFlag)
                       QuestFlagPickup(pos, "Flag_X")  → SetFlag(flagName_)

Emits docs/text-map.md (mermaid + tables + drift). No LLM, no deps — same
spirit as tools/docs_graph.py. LangGraph would be the wrong tool: this is
deterministic static analysis, not an agent workflow.

Usage:  python3 tools/text_map.py [out.md]      # default docs/text-map.md
"""
from __future__ import annotations
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTENT = ROOT / "docs" / "content"
CODE_DIRS = [ROOT / "src", ROOT / "include"]
DEFAULT_OUT = ROOT / "docs" / "text-map.md"

FLAG_RX = re.compile(r"Flag_[A-Za-z][A-Za-z0-9_]*")
FLAG_LIT_RX = re.compile(r'"(Flag_[A-Za-z0-9_]+)"')
CONST_DEF_RX = re.compile(r'\b(k[A-Za-z0-9_]+)\b\s*[={]\s*"(Flag_[A-Za-z0-9_]+)"')
CONTENT_SET_RX = re.compile(r"(Flag_[A-Za-z0-9_]+)\s*=\s*(?:true|false)")
CJK_LITERAL_RX = re.compile(r'"[^"\n]*[㐀-鿿＀-￯][^"\n]*"')


def rel(p: Path) -> str:
    return str(p.relative_to(ROOT))


def code_files() -> list[Path]:
    out = []
    for base in CODE_DIRS:
        for p in sorted(base.rglob("*")):
            # Skip build artifacts and the harness: the harness only LISTS
            # flag literals for the state.jsonl dump (instrumentation), not
            # real set/read logic, so it would pollute the set-sites.
            if p.suffix in (".cpp", ".h") and "build" not in p.parts \
                    and "harness" not in p.parts:
                out.append(p)
    return out


def call_args(line: str, fn: str) -> list[str]:
    """Tokens passed to fn( on this line: the "Flag_X" literal or a kConst."""
    out = []
    for m in re.finditer(re.escape(fn) + r'\(\s*([^),]+)', line):
        arg = m.group(1).strip()
        lit = re.match(r'"(Flag_[A-Za-z0-9_]+)"', arg)
        if lit:
            out.append(("lit", lit.group(1)))
        elif re.match(r"k[A-Za-z0-9_]+$", arg):
            out.append(("const", arg))
    return out


def main() -> int:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUT

    # flag -> kind -> set(files)
    sites: dict[str, dict[str, set[str]]] = defaultdict(lambda: defaultdict(set))
    ui_files: set[str] = set()

    # ---- content ----
    content_files = sorted(CONTENT.glob("*.md"))
    for p in content_files:
        text = p.read_text(encoding="utf-8", errors="replace")
        name = rel(p)
        for fl in CONTENT_SET_RX.findall(text):
            sites[fl]["set"].add(name)
        for fl in set(FLAG_RX.findall(text)):
            sites[fl]["content"].add(name)

    # ---- code: pass 1, resolve kName -> Flag_X ----
    const_map: dict[str, str] = {}
    code_text: dict[str, str] = {}
    for p in code_files():
        t = p.read_text(encoding="utf-8", errors="replace")
        code_text[rel(p)] = t
        for k, fl in CONST_DEF_RX.findall(t):
            const_map[k] = fl
        if CJK_LITERAL_RX.search(t):
            ui_files.add(rel(p))

    # ---- code: pass 2, classify per line ----
    def resolve(tok):
        kind, val = tok
        return val if kind == "lit" else const_map.get(val)

    for name, t in code_text.items():
        for line in t.splitlines():
            if "Flag_" not in line and "kFlag" not in line and not any(
                    k in line for k in const_map):
                continue
            reads = [resolve(a) for a in call_args(line, "HasFlag")]
            sets = [resolve(a) for a in call_args(line, "SetFlag")]
            sets += [resolve(a) for a in call_args(line, "ClearFlag")]
            for fl in filter(None, reads):
                sites[fl]["read"].add(name)
            for fl in filter(None, sets):
                sites[fl]["set"].add(name)
            # any remaining literal NOT consumed by HasFlag = a set-wire
            # (DialogChoice payload / pickup ctor → SetFlag(var) elsewhere)
            consumed = set(filter(None, reads)) | set(filter(None, sets))
            for fl in FLAG_LIT_RX.findall(line):
                if fl not in consumed and "HasFlag" not in line:
                    sites[fl]["set"].add(name)
                sites.setdefault(fl, defaultdict(set))

    flags = sorted(sites)

    def of(fl, *kinds):
        s: set[str] = set()
        for k in kinds:
            s |= sites[fl].get(k, set())
        return sorted(s)

    dead = [f for f in flags if of(f, "set") and not of(f, "read")]
    orphan = [f for f in flags if of(f, "read") and not of(f, "set")]

    def mid(s):
        return re.sub(r"[^A-Za-z0-9]", "_", s)

    L: list[str] = []
    L += ["# Text Map — 《尋傘記》 (text ↔ flag ↔ code)", "",
          "Generated by `tools/text_map.py` (static, no LLM). Bridges dialogue in",
          "`docs/content/*.md` and UI strings + branch logic in `src/`+`include/`",
          "through the `Flag_*` they share. Touch a file and find — via the flag",
          "table — every other file the same flag couples to.", ""]

    L += ["## 1. Where the text lives", "",
          "- **Dialogue (runtime-loaded `.md`)**: " +
          ", ".join(f"`{rel(p)}`" for p in content_files),
          "- **UI / system strings (compiled `.h`/`.cpp`)**: " +
          ", ".join(f"`{f}`" for f in sorted(ui_files)), ""]

    L += ["## 2. Flag ↔ file graph (content/code that SET ▶ flag ▶ code that READS)",
          "", "```mermaid", "flowchart LR"]
    seen: set[str] = set()

    def node(nid, label, shape='["{}"]'):
        if nid not in seen:
            L.append(f"    {nid}{shape.format(label)}")
            seen.add(nid)
    for fl in flags:
        node("F_" + mid(fl), fl, '{{"{}"}}')
        for f in of(fl, "set"):
            n = "S_" + mid(f)
            node(n, f.replace("docs/content/", "").replace("src/", "").replace("include/", ""))
            L.append(f"    {n} --> F_{mid(fl)}")
        for f in of(fl, "read"):
            n = "R_" + mid(f)
            node(n, f.replace("src/", "").replace("include/", ""))
            L.append(f"    F_{mid(fl)} --> {n}")
    L += ["```", ""]

    L += ["## 3. Flag table", "",
          "| Flag | set in | read in (HasFlag) | content files |",
          "|------|--------|-------------------|---------------|"]
    for fl in flags:
        s = ", ".join(f.replace("docs/content/", "").replace("src/", "").replace("include/", "")
                      for f in of(fl, "set")) or "—"
        r = ", ".join(f.replace("src/", "").replace("include/", "")
                      for f in of(fl, "read")) or "—"
        c = ", ".join(f.replace("docs/content/", "") for f in of(fl, "content")) or "—"
        L.append(f"| `{fl}` | {s} | {r} | {c} |")
    L += [""]

    L += ["## 4. Drift warnings", ""]
    if not dead and not orphan:
        L += ["None — every set flag is read, every read flag is set. "
              "(Pair with `dialog_lint.py` for the gate.)"]
    else:
        if dead:
            L += ["**Set but never read (dead flag / dead branch — B3 class):**"]
            L += [f"- `{f}` — set in {', '.join(of(f, 'set'))}, read nowhere" for f in dead]
            L += [""]
        if orphan:
            L += ["**Read but never set (orphan gate — branch can't fire):**"]
            L += [f"- `{f}` — read in {', '.join(of(f, 'read'))}, set nowhere" for f in orphan]
    L += [""]

    out.write_text("\n".join(L) + "\n", encoding="utf-8")
    print(f"wrote {out} — flags={len(flags)} dead={len(dead)} orphan={len(orphan)} "
          f"ui_code_files={len(ui_files)} consts={len(const_map)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
