#!/usr/bin/env python3
"""Composite all building tiles onto the worldmap base.

Usage:
    python3 tools/composite_worldmap.py

Output: resources/assets/maps/worldmap.png

To adjust building positions, edit the BUILDINGS dict below — values are
(x_center, y_center, target_pixel_height) in the 2048x2048 output canvas.
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "resources" / "assets"
BASE_MAP = ASSETS / "maps" / "worldmap_base.png"
# Switched from buildings_3d (oblique 3/4) to buildings_topdown (pure
# bird's-eye plan). Top-down bboxes hug the visible footprint so trigger
# rects and collision rects line up with what the player sees, instead of
# the building roof overhang extending past the wall.
BUILDINGS_DIR = ASSETS / "buildings_topdown"
OUTPUT = ASSETS / "maps" / "worldmap.png"

OUTPUT_SIZE = 2048
GRAY_TOLERANCE = 35

# (cx, cy, target_height_px) in 2048x2048 canvas.
#
# Layout aligned with the new base map (resources/assets/maps/worldmap_base.png):
#   - Indigo river runs E-W along the NORTH inner edge (y ~ 400-600)
#   - Si Wei Blvd horizontal road around y ~ 1340-1400
#   - Zhinan Rd south promenade y ~ 1820-1900
#   - Central plaza with fountain rendered at (~1000, 1080)
#   - Running-track oval rendered in NE quadrant (~1400, 760)
#
# Top-down tiles have variable aspect ratios — target_h is picked per
# building so the placed width stays under ~330 px and adjacent buildings
# leave a walking lane. Anchor positions follow the real NCCU 山下 map:
#   NW -> 游泳館 / 樂活館; NE -> 操場 / 體育館;
#   centre -> 中正圖書館 + 羅馬廣場; SE -> 大字 academic cluster;
#   SW corner -> 正門 main gate next to Zhinan Rd.
BUILDINGS = {
    # ===== North zone (between river y~560 and the plaza-row y~1100) =====
    # Mid-N west row, west to east
    "游泳館":        ( 200,  720, 180),
    "樂活館":        ( 520,  720, 180),
    "研究大樓":      ( 820,  720, 160),
    # Library sits south of the west cluster
    "中正圖書館":    ( 360,  990, 200),
    # NE: overlay the base's track + gym row
    "體育館":        (1750,  610, 180),
    # East mid: politics / ethnology cluster proxies
    "綜合院館":      (1700,  900, 180),
    "校友服務中心":   (1880, 1100, 180),

    # ===== South row 1 (just south of Si Wei Blvd, y~1430) =====
    "行政大樓":      ( 180, 1430, 200),
    "樂活小舖":      ( 440, 1430, 180),
    "商學院":        ( 720, 1430, 180),
    "新聞館":        ( 985, 1430, 180),
    "資訊大樓":      (1280, 1430, 180),
    "風雩樓":        (1600, 1430, 180),
    "大仁樓":        (1920, 1430, 140),

    # ===== South row 2 (y~1620), 大字 cluster spreading east =====
    "法學院":        ( 110, 1620, 200),
    "井塘樓":        ( 330, 1620, 180),
    "四維堂":        ( 580, 1620, 180),
    "果夫樓":        ( 870, 1620, 160),
    "集英樓":        (1140, 1620, 180),
    "大勇樓":        (1430, 1620, 180),
    "學思樓":        (1730, 1620, 180),

    # ===== South row 3 (y~1770) for overflow + the long corridor =====
    "風雩走廊":      ( 900, 1770, 100),
    "志希樓":        (1330, 1770, 140),
    "大智樓":        (1620, 1770, 140),

    # ===== SW gate, alone on Zhinan Rd =====
    "正門":         ( 180, 1890, 140),
}

# Buildings whose tile is NOT composited (the base map already draws their
# visual — track oval in NE, fountain plaza in centre) but which still
# need a trigger rect for BuildingTracker. These are emitted into the
# Buildings.h kAll[] block alongside the composited ones; the (cx, cy, w, h)
# describes the trigger footprint in world pixels, not a tile placement.
TRIGGER_ONLY = {
    # Inner field of the track, inside the perimeter strips from Obstacles.h.
    # Player can enter via the south gap at x [1380, 1480], walk into the
    # field, and trip the 操場 entry event.
    "操場":      (1445, 810, 390, 160),
    # Central plaza around the fountain medallion rendered in the base.
    "羅馬廣場":  (1000, 1080, 210, 200),
}


def remove_background(tile_path: Path) -> Image.Image:
    """Remove the gray plate background using flood-fill from each corner."""
    img = Image.open(tile_path).convert("RGB").copy()
    w, h = img.size
    SENTINEL = (255, 0, 255)
    for corner in [(0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1)]:
        ImageDraw.floodfill(img, corner, SENTINEL, thresh=GRAY_TOLERANCE)

    rgba = img.convert("RGBA")
    pixels = list(rgba.getdata())
    new_pixels = [
        (255, 255, 255, 0) if (px[0] > 240 and px[1] < 40 and px[2] > 240) else px
        for px in pixels
    ]
    rgba.putdata(new_pixels)

    bbox = rgba.getbbox()
    if bbox:
        rgba = rgba.crop(bbox)
    return rgba


def fit_to_height(img: Image.Image, target_h: int) -> Image.Image:
    """Resize keeping aspect ratio, target by height. Nearest-neighbor (pixel-art)."""
    w, h = img.size
    new_w = max(1, round(w * target_h / h))
    return img.resize((new_w, target_h), Image.NEAREST)


def main():
    if not BASE_MAP.exists():
        raise SystemExit(f"Base map not found: {BASE_MAP}")

    base = Image.open(BASE_MAP).convert("RGBA").resize(
        (OUTPUT_SIZE, OUTPUT_SIZE), Image.NEAREST
    )

    placed = []
    missing = []
    for name, (cx, cy, target_h) in BUILDINGS.items():
        tile_path = BUILDINGS_DIR / f"{name}.png"
        if not tile_path.exists():
            missing.append(name)
            continue
        sprite = remove_background(tile_path)
        sprite = fit_to_height(sprite, target_h)
        sw, sh = sprite.size
        x = cx - sw // 2
        y = cy - sh // 2
        base.paste(sprite, (x, y), sprite)
        placed.append((name, x, y, sw, sh))

    base.save(OUTPUT, optimize=True)
    print(f"✓ Saved {OUTPUT}  ({OUTPUT_SIZE}x{OUTPUT_SIZE})")
    print(f"  Placed: {len(placed)} / {len(BUILDINGS)}")
    if missing:
        print(f"  Missing tiles: {', '.join(missing)}")

    # Emit Buildings.h kAll[] entries using the ACTUAL placed rect (not a
    # square derived from target_h). The compositor crops each tile to its
    # foreground bbox before scaling, so a wide top-down building yields
    # a wide placed sprite; using target_h for both axes leaves a
    # "trigger / visual" mismatch the player can walk over. TRIGGER_ONLY
    # entries (track, plaza) are appended as-is — their visual already
    # exists in the base map and no tile is pasted.
    print()
    print("// Paste into include/Buildings.h kAll[]:")
    for name, x, y, w, h in placed:
        print(f'    {{"{name}", '
              f'{{{float(x):7.1f}f, {float(y):7.1f}f, '
              f'{float(w):6.1f}f, {float(h):6.1f}f}}}},')
    for name, (cx, cy, w, h) in TRIGGER_ONLY.items():
        x = cx - w // 2
        y = cy - h // 2
        print(f'    {{"{name}", '
              f'{{{float(x):7.1f}f, {float(y):7.1f}f, '
              f'{float(w):6.1f}f, {float(h):6.1f}f}}}},')


if __name__ == "__main__":
    main()
