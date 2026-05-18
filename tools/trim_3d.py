#!/usr/bin/env python3
"""One-time preprocess: remove the white background from every PNG in
resources/assets/buildings_3d/ and save the cropped, alpha-cut result to
resources/assets/buildings_3d_trimmed/.

Mirrors trim_tiles.py but tuned for the 3D / oblique tile set, which
uses a pure-white background instead of the gray plate the top-down
tiles ship with.

Usage:
    python3 tools/trim_3d.py
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC  = ROOT / "resources/assets/buildings_3d"
DST  = ROOT / "resources/assets/buildings_3d_trimmed"
TOL  = 25
# Files inside SRC that are not real building tiles.
SKIP_SUFFIXES = ("_thumb.jpeg", "_thumb.png")
SKIP_TOKENS   = ("_nb2_test", "_test", "_old")
SKIP_NAMES    = {"general_building.png"}


def trim(src_path: Path) -> Image.Image:
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
