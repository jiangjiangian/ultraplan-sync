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

# chapter/interlude: "- SemesterState: `Chapter1_AddDrop`"
STATE_RE = re.compile(r"SemesterState:\s*`([A-Za-z0-9_]+)`")
# endings: yaml "state_entry: nccu::SemesterState::Ending_A" (key name
# varies: state / state_entry / state_machine_entry). The entry-state
# line always precedes the from-state line in the script files.
STATE_RE2 = re.compile(r"SemesterState::([A-Za-z0-9_]+)")
NPC_RE = re.compile(r"^##\s*NPC：(.+?)\s*$")
SCENE_RE = re.compile(r"^##\s*場景旁白\s*$")
# Group 1 = sub-block letter (a-d -> subState via SUB), group 2 = the
# heading text that becomes the choice label (see clean_label).
SUBSEC_RE = re.compile(r"^###\s*\(([a-d])\)\s*(.*)$")
SCENE_SUBSEC_RE = re.compile(r"^###\s+(.+?)\s*$")
LINE_RE = re.compile(r'^-\s*"(.*)"\s*$')
KARMA_RE = re.compile(r"//\s*karma\s*([+-]\d+)")
FLAG_RE = re.compile(r"\b(Flag_[A-Za-z0-9_]+)\s*=\s*(true|false)\b")


def clean_label(h):
    """Derive a choice menu label from a sub-block heading.

    Rule: a 「…」 quoted span is the author's explicit label override and
    wins (e.g. `(b) 玩家給予安慰（選擇「我去幫你追」）` -> `我去幫你追`).
    Otherwise the label is the heading minus any trailing （…）
    parenthetical (e.g. `(c) 玩家接受，取傘後交給學長` stays as-is).
    """
    h = h.strip()
    m = re.search(r"「(.+?)」", h)      # quoted span wins (author override)
    if m:
        return m.group(1)
    return re.sub(r"（.*?）\s*$", "", h).strip()  # else drop trailing （…）


class Entry:
    def __init__(self, npc_id, state, sub):
        self.npc_id = npc_id
        self.state = state
        self.sub = sub
        self.lines = []
        self.karma = 0
        self.flag = ""        # "" = none
        self.flag_val = False
        self.choice_label = ""   # "" = not a choice (scene / subState 0)


def parse_file(path):
    state = None
    entries = []
    cur = None
    cur_npc = None
    scene_sub = -1
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.rstrip()
        if state is None:
            m = STATE_RE.search(line) or STATE_RE2.search(line)
            if m:
                state = m.group(1)
        if line.startswith("## ") or line.startswith("# "):
            cur = None
            nm = NPC_RE.match(line)
            if nm:
                raw_name = nm.group(1).strip()
                # Script files annotate names: "圖書館管理員（新角色）"
                # and space them: "A 系烤香腸攤主". Normalise to the
                # canonical NPC_ID key: drop a trailing full-width
                # parenthetical, then remove all whitespace.
                name = re.sub(r"（.*?）\s*$", "", raw_name)
                name = re.sub(r"\s+", "", name)
                if name not in NPC_ID:
                    sys.exit(f"unknown NPC name in {path.name}: "
                             f"{raw_name!r} (normalised {name!r})")
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
                cur.choice_label = clean_label(sm.group(2))
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
    out.append("    std::string_view choiceLabel;  // \"\" = not a choice")
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
            f'{"true" if e.flag_val else "false"}, '
            f'"{cpp_escape(e.choice_label)}" }},')
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
