#!/usr/bin/env python3
"""One-time preprocess: trim the gray-plate padding off every PNG in
resources/assets/buildings_topdown/ and save the cropped result to
resources/assets/buildings_topdown_trimmed/.

The trimmed PNGs are what you import into Tiled as the building tile
collection. Because the gray plate is gone, the visible image inside
Tiled is exactly the building's footprint — what you see lined up on
the canvas is what tiled_to_world.py will paste onto worldmap.png.

Usage:
    python3 tools/trim_tiles.py
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC  = ROOT / "resources/assets/buildings_topdown"
DST  = ROOT / "resources/assets/buildings_topdown_trimmed"
TOL  = 35


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
    sources = sorted(p for p in SRC.glob("*.png") if "_thumb" not in p.name)
    for src in sources:
        out = DST / src.name
        trimmed = trim(src)
        trimmed.save(out)
        print(f"  {src.name}: -> {trimmed.size[0]}x{trimmed.size[1]}")
    print(f"\n{len(sources)} tiles trimmed -> {DST.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
