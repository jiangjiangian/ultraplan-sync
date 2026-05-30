#!/usr/bin/env python3
"""一次性前處理：把 resources/assets/buildings_3d/ 內每張 PNG 的白色背景
去除，並將去背裁切後的結果存到 resources/assets/buildings_3d_trimmed/。

與 trim_tiles.py 做法相同，但針對 3D／斜視角的 tile 集合調校：這組素材
使用純白背景，而非俯視 tile 所附的灰底。

用法：
    python3 tools/trim_3d.py
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC  = ROOT / "resources/assets/buildings_3d"
DST  = ROOT / "resources/assets/buildings_3d_trimmed"
TOL  = 25
# SRC 內並非真正建築 tile 的檔案。
SKIP_SUFFIXES = ("_thumb.jpeg", "_thumb.png")
SKIP_TOKENS   = ("_nb2_test", "_test", "_old")
SKIP_NAMES    = {"general_building.png"}


def trim(src_path: Path) -> Image.Image:
    """裁掉單張 tile 的白色背景並回傳裁切後的圖像。

    參數：
        src_path：來源 PNG 的路徑。
    回傳：
        去背並裁到前景外框的 RGBA Image；若全圖皆被去背則回傳原圖。

    做法：從四個角落以 floodfill 把白底填成洋紅色哨兵值，再把該哨兵色
    轉成全透明，最後依前景外框 (getbbox) 裁切。
    """
    img = Image.open(src_path).convert("RGB").copy()
    w, h = img.size
    SENT = (255, 0, 255)
    for corner in [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]:
        ImageDraw.floodfill(img, corner, SENT, thresh=TOL)
    rgba = img.convert("RGBA")
    px = list(rgba.getdata())
    rgba.putdata([
        (255, 255, 255, 0) if (p[0] > 240 and p[1] < 40 and p[2] > 240) else p
        for p in px
    ])
    bbox = rgba.getbbox()
    return rgba.crop(bbox) if bbox else rgba


def main():
    """主流程：批次去背裁切 SRC 內所有 PNG 並輸出到 DST。

    無參數、無回傳值。建立輸出資料夾，依 SKIP_NAMES／SKIP_TOKENS／
    SKIP_SUFFIXES 過濾掉非建築 tile 的檔案，逐張裁切後存檔並印出尺寸，
    最後印出處理總數。
    """
    DST.mkdir(exist_ok=True)
    sources = []
    for p in sorted(SRC.glob("*.png")):
        if p.name in SKIP_NAMES:
            continue
        if any(tok in p.name for tok in SKIP_TOKENS):
            continue
        if p.name.endswith(SKIP_SUFFIXES):
            continue
        sources.append(p)

    for src in sources:
        out = DST / src.name
        trimmed = trim(src)
        trimmed.save(out)
        print(f"  {src.name}: -> {trimmed.size[0]}x{trimmed.size[1]}")
    print(f"\n{len(sources)} tiles trimmed -> {DST.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
