#!/usr/bin/env python3
"""docs_graph.py — domain-specific knowledge-graph extractor for 《尋傘記》

Walks docs/, parses each markdown file for:
  - heading hierarchy (#, ##, ###)
  - Flag_* references (the SCRIPT_HANDOFF whitelist)
  - NPC headings (`## <name>：` with full-width colon U+FF1A)
  - karma annotations (`// karma ±N`)
  - cycle8-audit conflict counts (parsed from cycle8-audit/**/*.md summaries)

Emits docs/cycle8-audit/docs-graph.md with:
  1. a mermaid flowchart of file-to-file references (via shared flags / NPC names)
  2. a per-file stats table (heading count, flag mentions, NPC mentions, conflicts)
  3. a global flag-usage matrix
  4. a karma-flow summary
"""
from __future__ import annotations
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
AUDIT = DOCS / "cycle8-audit"
OUT = AUDIT / "docs-graph.md"

FLAG_RX = re.compile(r"Flag_[A-Za-z][A-Za-z0-9_]*")
KARMA_RX = re.compile(r"karma\s*([+\-]?\d+)")
NPC_HEADING_RX = re.compile(r"^##\s*(.+?)：")  # full-width colon
SUMMARY_COUNTS_RX = re.compile(
    r"\b(Implemented|Yes|Partial|No|N/A|Conflicts?|Stale[- ]doc[- ]only)\b[^\d]{0,20}(\d+)",
    re.IGNORECASE,
)


def parse_file(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace")
    headings = re.findall(r"^(#{1,6})\s+(.+)$", text, re.MULTILINE)
    flags = sorted(set(FLAG_RX.findall(text)))
    karma_deltas = [int(m) for m in KARMA_RX.findall(text)]
    npcs = sorted(set(NPC_HEADING_RX.findall(text)))
    return {
        "rel": str(path.relative_to(ROOT)),
        "lines": text.count("\n") + 1,
        "headings": len(headings),
        "flags": flags,
        "karma_count": len(karma_deltas),
        "karma_sum": sum(karma_deltas),
        "karma_pos": sum(d for d in karma_deltas if d > 0),
        "karma_neg": sum(d for d in karma_deltas if d < 0),
        "npcs": npcs,
    }


def parse_audit_conflicts(path: Path) -> int | None:
    text = path.read_text(encoding="utf-8", errors="replace")
    explicit = re.compile(r"[Cc]onflicts?\s*[:=][^\d\n]{0,8}(\d+)")
    matches = explicit.findall(text)
    if matches:
        return int(matches[-1])
    return None


def short(rel: str) -> str:
    base = rel.replace("docs/", "")
    if base.startswith("content/"):
        return base.removeprefix("content/").removesuffix(".md")
    if base.startswith("superpowers/"):
        return base.removeprefix("superpowers/").removesuffix(".md")
    return base.removesuffix(".md")


def mermaid_id(rel: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", rel)


def main() -> int:
    files: list[dict] = []
    for p in sorted(DOCS.rglob("*.md")):
        if "cycle8-audit" in p.parts or "iteration-history" in p.parts or "kb" in p.parts:
            continue
        files.append(parse_file(p))

    audit_conflicts: dict[str, int] = {}
    if AUDIT.exists():
        for p in sorted(AUDIT.rglob("*.md")):
            if p.name in {"README.md", "cycle9-ux-diagnosis.md", "docs-graph.md"}:
                continue
            n = parse_audit_conflicts(p)
            if n is None:
                continue
            stem = str(p.relative_to(AUDIT)).removesuffix(".md")
            audit_conflicts[stem] = n

    # build cross-ref edges via shared flags
    edges = defaultdict(int)
    by_flag: dict[str, list[str]] = defaultdict(list)
    for f in files:
        for fl in f["flags"]:
            by_flag[fl].append(f["rel"])
    for fl, owners in by_flag.items():
        if len(owners) < 2:
            continue
        for i, a in enumerate(owners):
            for b in owners[i + 1 :]:
                key = tuple(sorted([a, b]))
                edges[key] += 1

    # ---- emit markdown ----
    lines: list[str] = []
    lines.append("# Docs Knowledge Graph — 《尋傘記》")
    lines.append("")
    lines.append("Domain-specific extractor (`tools/docs_graph.py`). Walks `docs/`,")
    lines.append("parses heading tree + `Flag_*` references + NPC headings + karma")
    lines.append("annotations + cycle8-audit conflict counts. **No LLM required**.")
    lines.append("Replaces the abandoned `/graphify` attempt (required an LLM key).")
    lines.append("")
    lines.append("## 1. Cross-reference graph (edges = shared `Flag_*` between files)")
    lines.append("")
    lines.append("```mermaid")
    lines.append("flowchart LR")
    for f in files:
        nid = mermaid_id(f["rel"])
        label = short(f["rel"])
        confl = audit_conflicts.get(
            f["rel"].removeprefix("docs/").removesuffix(".md"), 0
        )
        suffix = f" ⚠{confl}" if confl > 0 else ""
        lines.append(f'    {nid}["{label}{suffix}"]')
    for (a, b), weight in sorted(edges.items(), key=lambda kv: -kv[1]):
        if weight < 1:
            continue
        lines.append(f"    {mermaid_id(a)} -- {weight} --- {mermaid_id(b)}")
    lines.append("```")
    lines.append("")
    lines.append("Node label suffix `⚠N` = N `[邏輯衝突?] Yes` items in")
    lines.append("`cycle8-audit/<same-path>.md` summary.")
    lines.append("")

    lines.append("## 2. Per-file stats")
    lines.append("")
    lines.append("| File | Lines | Headings | Flags | NPCs | karma Σ (+/−) | Conflicts |")
    lines.append("|------|-------|----------|-------|------|---------------|-----------|")
    for f in files:
        confl = audit_conflicts.get(
            f["rel"].removeprefix("docs/").removesuffix(".md"), "—"
        )
        karma_cell = f"{f['karma_sum']:+d} ({f['karma_pos']:+d}/{f['karma_neg']:+d})"
        lines.append(
            f"| `{f['rel']}` | {f['lines']} | {f['headings']} | "
            f"{len(f['flags'])} | {len(f['npcs'])} | {karma_cell} | {confl} |"
        )
    lines.append("")

    lines.append("## 3. Global flag-usage matrix")
    lines.append("")
    lines.append("| Flag | Files using it |")
    lines.append("|------|----------------|")
    for fl in sorted(by_flag.keys()):
        owners = sorted(set(by_flag[fl]))
        owners_short = ", ".join(short(o) for o in owners)
        lines.append(f"| `{fl}` | {owners_short} |")
    lines.append("")

    total_karma = sum(f["karma_sum"] for f in files)
    total_pos = sum(f["karma_pos"] for f in files)
    total_neg = sum(f["karma_neg"] for f in files)
    total_count = sum(f["karma_count"] for f in files)
    lines.append("## 4. Karma flow summary")
    lines.append("")
    lines.append(f"- Total karma annotations across `docs/content/`: **{total_count}**")
    lines.append(f"- Net karma if a player triggered every annotation: **{total_karma:+d}**")
    lines.append(f"- Positive routes sum: **{total_pos:+d}** · Negative routes sum: **{total_neg:+d}**")
    lines.append("- Starting karma per GDD: **50** → Ending A gate `karma > 80` requires")
    lines.append(f"  **{80 - 50}+** net positive picks; Ending B gate `karma < 0` requires")
    lines.append("  **−50+** net negative picks. The above totals show whether either gate")
    lines.append("  is reachable purely via dialog karma.")
    lines.append("")

    lines.append("## 5. Generation")
    lines.append("")
    lines.append("`python3 tools/docs_graph.py` rewrites this file. GitHub renders the")
    lines.append("mermaid block natively. No API key, no LLM, no external deps.")
    lines.append("")

    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)} — files={len(files)} flags={len(by_flag)} edges={len(edges)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
