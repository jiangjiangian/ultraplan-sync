#!/usr/bin/env python3
"""docs_graph.py — 《尋傘記》專用的知識圖譜萃取器。

掃描 docs/ 下的每個 markdown 檔，解析出標題層級、Flag_* 參照、NPC 標題
（以全形冒號 U+FF1A 結尾的 `## <名稱>：`）與 karma 標註（`// karma ±N`），
最後輸出 docs/docs-graph.md，內含檔案間參照的 mermaid 流程圖、每檔統計表、
全域旗標使用矩陣與 karma 流向摘要。純靜態分析，不需 LLM 或外部相依套件。

用法：
    python3 tools/docs_graph.py
"""
from __future__ import annotations
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
OUT = DOCS / "docs-graph.md"

# docs/ 底下屬於參考／封存性質、不納入即時交叉參照範圍的目錄：
# archive/ = 歷史快照，kb/ = Raylib 知識庫，superpowers/ = 僅本機的規劃筆記。
SKIP_DIRS = {"archive", "kb", "superpowers"}

FLAG_RX = re.compile(r"Flag_[A-Za-z][A-Za-z0-9_]*")
KARMA_RX = re.compile(r"karma\s*([+\-]?\d+)")
NPC_HEADING_RX = re.compile(r"^##\s*(.+?)：")  # 全形冒號


def parse_file(path: Path) -> dict:
    """解析單一 markdown 檔，萃取標題、旗標、karma 與 NPC 資訊。

    參數：
        path：要解析的 markdown 檔路徑。
    回傳：
        含相對路徑、行數、標題數、旗標清單、karma 統計與 NPC 清單的 dict。
    """
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


def short(rel: str) -> str:
    """把相對路徑縮短成易讀的標籤。

    參數：
        rel：檔案的相對路徑字串。
    回傳：
        去掉 docs/、content/ 前綴與 .md 副檔名後的精簡名稱。
    """
    base = rel.replace("docs/", "")
    if base.startswith("content/"):
        return base.removeprefix("content/").removesuffix(".md")
    return base.removesuffix(".md")


def mermaid_id(rel: str) -> str:
    """把路徑轉成可當作 mermaid 節點 ID 的安全字串。

    參數：
        rel：檔案的相對路徑字串。
    回傳：
        將非英數字元一律換成底線後的字串。
    """
    return re.sub(r"[^A-Za-z0-9]", "_", rel)


def main() -> int:
    """主流程：掃描 docs/、建立交叉參照圖並寫出 docs-graph.md。

    無參數。逐檔解析後，依共用旗標建立檔案間的關聯邊，組出 mermaid 圖、
    統計表、旗標矩陣與 karma 摘要寫入 OUT，回傳行程結束碼 0。
    """
    files: list[dict] = []
    for p in sorted(DOCS.rglob("*.md")):
        if p == OUT or SKIP_DIRS & set(p.parts):
            continue
        files.append(parse_file(p))

    # 依共用旗標建立檔案間的交叉參照邊
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

    # ---- 輸出 markdown ----
    lines: list[str] = []
    lines.append("# Docs Knowledge Graph — 《尋傘記》")
    lines.append("")
    lines.append("Domain-specific extractor (`tools/docs_graph.py`). Walks `docs/`,")
    lines.append("parses heading tree + `Flag_*` references + NPC headings + karma")
    lines.append("annotations. **No LLM required.**")
    lines.append("")
    lines.append("## 1. Cross-reference graph (edges = shared `Flag_*` between files)")
    lines.append("")
    lines.append("```mermaid")
    lines.append("flowchart LR")
    for f in files:
        nid = mermaid_id(f["rel"])
        label = short(f["rel"])
        lines.append(f'    {nid}["{label}"]')
    for (a, b), weight in sorted(edges.items(), key=lambda kv: -kv[1]):
        if weight < 1:
            continue
        lines.append(f"    {mermaid_id(a)} -- {weight} --- {mermaid_id(b)}")
    lines.append("```")
    lines.append("")

    lines.append("## 2. Per-file stats")
    lines.append("")
    lines.append("| File | Lines | Headings | Flags | NPCs | karma Σ (+/−) |")
    lines.append("|------|-------|----------|-------|------|---------------|")
    for f in files:
        karma_cell = f"{f['karma_sum']:+d} ({f['karma_pos']:+d}/{f['karma_neg']:+d})"
        lines.append(
            f"| `{f['rel']}` | {f['lines']} | {f['headings']} | "
            f"{len(f['flags'])} | {len(f['npcs'])} | {karma_cell} |"
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
