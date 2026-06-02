#!/usr/bin/env python3
"""bundle.py — 把 graph/ 的 CSS / JS / 資料內嵌成「單一自足」的 index.html。

這樣使用者直接雙擊 index.html（file://）、或放上任何主機（GitHub Pages…）都能看，
完全不需要相對資源載入——解決「在沙箱預覽 / 缺少同層檔案時整頁空白」的問題。

來源（可編輯）：index.src.html（版面）、assets/style.css、assets/app.js、
vendor/cytoscape.min.js、data/graph-data.js（由 build_graph.py 產生）。
build_graph.py 末端會自動呼叫本檔；也可單獨執行：python3 graph/bundle.py
"""
from pathlib import Path

GRAPH = Path(__file__).resolve().parent


def read(rel: str) -> str:
    return (GRAPH / rel).read_text(encoding="utf-8")


def safe(s: str) -> str:
    # 避免內嵌的程式/資料裡若有 </script> 提前關閉 script 區塊
    return s.replace("</script>", "<\\/script>")


def main() -> None:
    html = read("index.src.html")
    repl = {
        '<link rel="stylesheet" href="assets/style.css">':
            f"<style>\n{read('assets/style.css')}\n</style>",
        '<script src="vendor/cytoscape.min.js"></script>':
            f"<script>\n{safe(read('vendor/cytoscape.min.js'))}\n</script>",
        '<script src="data/graph-data.js"></script>':
            f"<script>\n{safe(read('data/graph-data.js'))}\n</script>",
        '<script src="assets/app.js"></script>':
            f"<script>\n{safe(read('assets/app.js'))}\n</script>",
    }
    missing = [k for k in repl if k not in html]
    if missing:
        raise SystemExit("index.src.html 缺少預期的標籤：\n  " + "\n  ".join(missing))
    for k, v in repl.items():
        html = html.replace(k, v)
    out = GRAPH / "index.html"
    out.write_text(html, encoding="utf-8")
    print(f"wrote self-contained graph/index.html ({len(html) // 1024} KB, 零外部相依)")


if __name__ == "__main__":
    main()
