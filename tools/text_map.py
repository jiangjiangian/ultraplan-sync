#!/usr/bin/env python3
"""text_map.py — 《尋傘記》的「文字↔旗標↔程式碼」相依關係圖。

遊戲文字分散在兩處，這正是「改了這段對話邏輯會不會壞掉」難以一眼看出的
原因：
  - 敘事對話        → docs/content/*.md  （執行期由 DialogLoader 載入）
  - UI／系統字串    → src/ + include/    （.h/.cpp 內的字串字面值）
  - 分支邏輯        → Flag_* 在某處設定、又在另一處讀取

本工具把這些串接起來。對每個 Flag_*，它都記錄「在哪裡被設定」（content 內
的 `Flag_X = true` 指令、程式碼裡的 SetFlag，或是資料驅動的接線——交給
DialogChoice／拾取物建構子的 "Flag_X" 字面值）以及「在哪裡被讀取」（HasFlag，
不論是字串字面值或解析後的 `kFlag* = "Flag_*"` 常數）。因此改動任一檔案就能
看出相關檔案，並會浮現：
  - 有設定卻從未讀取 → 死旗標／死對話分支
  - 有讀取卻從未設定 → 孤兒關卡（永遠觸發不到的分支）

旗標名會以三種方式抵達 SetFlag／HasFlag，皆已處理：
  1. 字串字面值：  SetFlag("Flag_X") / HasFlag("Flag_X")
  2. 具名常數：    kFlagX = "Flag_X"; HasFlag(kFlagX)
  3. 資料驅動接線：DialogChoice{.., "Flag_X", ..}; SetFlag(choice.setsFlag)
                   QuestFlagPickup(pos, "Flag_X")  → SetFlag(flagName_)

輸出 docs/text-map.md（mermaid + 表格 + 漂移警告）。純靜態分析，不需 LLM 或
外部相依套件，與 tools/docs_graph.py 同一思路。

用法：  python3 tools/text_map.py [out.md]      # 預設 docs/text-map.md
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
    """回傳路徑相對於專案根目錄的字串。

    參數：
        p：絕對路徑。
    回傳：
        相對於 ROOT 的路徑字串。
    """
    return str(p.relative_to(ROOT))


def code_files() -> list[Path]:
    """蒐集要掃描的 C++ 原始碼檔清單。

    無參數。
    回傳：
        CODE_DIRS 底下所有 .cpp／.h 檔的路徑清單，已排除 build 與
        harness 目錄。
    """
    out = []
    for base in CODE_DIRS:
        for p in sorted(base.rglob("*")):
            # 略過建置產物與測試框架：harness 只是為了 state.jsonl 傾印而
            # 「列出」旗標字面值（屬於儀器化），並非真正的設定／讀取邏輯，
            # 納入會污染設定點。
            if p.suffix in (".cpp", ".h") and "build" not in p.parts \
                    and "harness" not in p.parts:
                out.append(p)
    return out


def call_args(line: str, fn: str) -> list[str]:
    """擷取本行傳給 fn( 的引數：可能是 "Flag_X" 字面值或某個 kConst 常數。

    參數：
        line：原始碼的一行文字。
        fn：要比對的函式名稱（例如 HasFlag、SetFlag）。
    回傳：
        (kind, value) 元組清單，kind 為 "lit"（字面值）或 "const"（常數）。
    """
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
    """主流程：建立旗標的設定／讀取點索引並寫出 text-map.md。

    無參數。掃描 docs/content 與 src／include，把每個 Flag_* 的設定點、
    讀取點與內容檔歸檔，再組出文字分佈說明、mermaid 圖、旗標表與漂移
    警告寫入輸出檔，回傳行程結束碼 0。
    """
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_OUT

    # 結構：flag -> kind -> 檔案集合
    sites: dict[str, dict[str, set[str]]] = defaultdict(lambda: defaultdict(set))
    ui_files: set[str] = set()

    # ---- 內容檔 ----
    content_files = sorted(CONTENT.glob("*.md"))
    for p in content_files:
        text = p.read_text(encoding="utf-8", errors="replace")
        name = rel(p)
        for fl in CONTENT_SET_RX.findall(text):
            sites[fl]["set"].add(name)
        for fl in set(FLAG_RX.findall(text)):
            sites[fl]["content"].add(name)

    # ---- 程式碼：第 1 趟，解析 kName -> Flag_X ----
    const_map: dict[str, str] = {}
    code_text: dict[str, str] = {}
    for p in code_files():
        t = p.read_text(encoding="utf-8", errors="replace")
        code_text[rel(p)] = t
        for k, fl in CONST_DEF_RX.findall(t):
            const_map[k] = fl
        if CJK_LITERAL_RX.search(t):
            ui_files.add(rel(p))

    # ---- 程式碼：第 2 趟，逐行分類 ----
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
            # 其餘未被 HasFlag 取用的字面值即視為設定接線
            # （DialogChoice 載荷／拾取物建構子 → 在別處 SetFlag(var)）
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

    L += ["## 4. Drift warnings", "",
          "> CAVEAT: this tool tracks `Flag_*` set/read only — it does NOT model",
          "> event-based transitions (e.g. `UmbrellaClaimed` → `Transition`) or",
          "> test-only stubs. A flag flagged below may still be reachable via an",
          "> event handler, or be a deliberate test affordance. VERIFY in code",
          "> before treating it as a bug. (Known case: `Flag_Ch3Cleared` is read",
          "> by ChapterGate as a test-spine stub; Ch3 actually clears via the",
          "> `UmbrellaClaimed` event in EventWiring — NOT a real orphan.)", ""]
    if not dead and not orphan:
        L += ["None — every set flag is read, every read flag is set. "
              "(Pair with `dialog_lint.py` for the gate.)"]
    else:
        if dead:
            L += ["**Set but never read via HasFlag (candidate dead branch — B3 class):**"]
            L += [f"- `{f}` — set in {', '.join(of(f, 'set'))}, read nowhere" for f in dead]
            L += [""]
        if orphan:
            L += ["**Read but never set via a flag (candidate orphan — "
                  "may fire via an event / test path, verify in code):**"]
            L += [f"- `{f}` — read in {', '.join(of(f, 'read'))}, set nowhere" for f in orphan]
    L += [""]

    out.write_text("\n".join(L) + "\n", encoding="utf-8")
    print(f"wrote {out} — flags={len(flags)} dead={len(dead)} orphan={len(orphan)} "
          f"ui_code_files={len(ui_files)} consts={len(const_map)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
