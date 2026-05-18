#!/usr/bin/env python3
"""Emit Buildings.h kAll[] and bake the terrain collision mask from a
Tiled JSON map.

Workflow:
  1. In Tiled:
       File -> New Map...  Orthogonal, 64x64 tiles @ 32px (= 2048x2048).
       Image Layer "Base" -> resources/assets/maps/worldmap_base.png.
       Tileset "buildings" -> every PNG in
         resources/assets/buildings_3d_trimmed/.
       Object Layer "Buildings" -> Insert Tile, place each building
         (may be flipped X / Y for orientation variety).
       Per building art: open the tile in the Tile Collision Editor and
         trace the SOLID WALL BASE (the part that meets the ground) with
         a polygon / rectangle. Leave the roof + eaves OUTSIDE the shape
         so the player can still walk behind the building.
  2. File -> Export As... -> Tiled JSON, e.g. tools/world.tmj
  3. python3 tools/tiled_to_world.py tools/world.tmj

Two ways to author terrain collision, both baked EXACTLY (pixel-perfect,
no grid quantisation, no footprint fallback):
  a. A building's wall base — Tile Collision Editor on its tile.
  b. Trees / planters / the campus perimeter wall — free
     rectangles / polygons / ellipses on a map-coordinate object layer
     named "Collision" (props baked into worldmap_base.png have no tile
     to attach a shape to).
The river is hard-coded. An un-traced building contributes NO collision
(so the campus gate stays walkable — nothing is drawn across it).

Outputs:
  - stdout: include/Buildings.h kAll[] paste block (sprite rects + flip
    flags — the trigger zones BuildingTracker keys chapter events on).
  - resources/assets/maps/collision_mask.png — the SHIPPED mask the
    engine loads. REGENERATED every run from Tiled (wall bases +
    "Collision" layer shapes + river). White = walkable, black = solid,
    RGB so any editor round-trips it. You may hand-edit it for a quick
    experiment, but re-running this tool overwrites it — Tiled is the
    source of truth.
  - resources/assets/maps/collision_mask_base.png — byte-identical
    pristine copy; the engine falls back to it if collision_mask.png is
    missing, so terrain never silently vanishes.
  - resources/assets/maps/collision_mask_guide.png — worldmap_base
    dimmed with solid pixels tinted red (visual check). Best-effort:
    only written when Pillow is importable.

Collision model:
  The engine samples collision_mask.png per pixel under the player's
  footprint (see include/CollisionMask.h): walkable iff a pixel is pure
  white or fully transparent, else solid.

No third-party dependency: the PNG writer and polygon rasteriser are
pure stdlib (zlib). Pillow, if present, only adds the optional dimmed
guide / sprite preview.

Robustness:
  - Tiled flip bits (high gid bits) are stripped for the tileset lookup
    and re-emitted as flipX/flipY; collision shapes are mirrored to
    match. A DIAGONAL flip on a building is a hard error.
  - The emitted name is the canonical building name: a trailing
    art-variant suffix (_nb2, _test, _old, _trimmed, _vN) and any parent
    folder are removed (中正圖書館_nb2 -> 中正圖書館).
  - If the tileset's recorded image path is stale, the real PNG is
    located by basename inside resources/assets/buildings_3d_trimmed/.
  - Ellipses on the "Collision" layer are filled as true ellipses (the
    natural tree / planter shape), not their bounding box.
"""
import json
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "resources" / "assets"
BASE_MAP = ASSETS / "maps" / "worldmap_base.png"
OUTPUT = ASSETS / "maps" / "worldmap.png"
MASK_BASE = ASSETS / "maps" / "collision_mask_base.png"
MASK_SHIP = ASSETS / "maps" / "collision_mask.png"
MASK_GUIDE = ASSETS / "maps" / "collision_mask_guide.png"
BUILDINGS_DIR = ASSETS / "buildings_3d_trimmed"

SIZE = 2048

FLIP_H = 0x80000000
FLIP_V = 0x40000000
FLIP_D = 0x20000000
GID_MASK = 0x0FFFFFFF

# Baked into worldmap_base.png — no sprite composited, no collision
# baked (open ground / plaza); still emitted to Buildings.h as a
# trigger-only entry.
NO_TILE_OVERLAY = {"操場", "羅馬廣場"}

# The river is PAINTED into worldmap_base.png as a curved band across the
# top, with a walkable bridge gap. Hand-coded rectangles drifted off the
# art (sealing grass, missing the real bridge), so the river collision is
# sampled straight from the base map: a pixel is "water" iff it is clearly
# blue. Aligned with what the player sees by construction — it can never
# drift again. The scan is bounded to the top RIVER_MAX_Y rows: the river
# is the only water on the map (verified), and the bound stops any future
# blue prop elsewhere from being misread as river.
RIVER_MAX_Y = 650


def is_water_rgb(r, g, b):
    # Calibrated to the CURRENT worldmap_base.png palette. If that art is
    # ever regenerated (new palette / post-processing / re-render),
    # re-sample a few known water pixels and re-tune this threshold —
    # otherwise the river bake silently misses water or grabs non-water.
    return b > 90 and b > r + 20 and b > g

_SUFFIX_RE = re.compile(r"_(nb\d+|test|old|trimmed|v\d+)$")
_TRAIL_DIGITS_RE = re.compile(r"\d+$")


def canonical_name(image_path: str) -> str:
    stem = _SUFFIX_RE.sub("", Path(image_path).stem)
    return _TRAIL_DIGITS_RE.sub("", stem)


def load_tilesets(tmj_dir: Path, raw: dict) -> dict:
    g2tile = {}

    def add(first, tile, img_dir):
        g2tile[first + tile["id"]] = {
            "img": (img_dir / tile["image"]).resolve(),
            "nw": int(tile.get("imagewidth", 0)),
            "nh": int(tile.get("imageheight", 0)),
            "shapes": tile.get("objectgroup", {}).get("objects", []),
        }

    for ts in raw.get("tilesets", []):
        first = ts["firstgid"]
        if ts.get("tiles"):
            for tile in ts["tiles"]:
                add(first, tile, tmj_dir)
        elif "source" in ts:
            ext_path = (tmj_dir / ts["source"]).resolve()
            ext = json.loads(ext_path.read_text(encoding="utf-8"))
            for tile in ext.get("tiles", []):
                add(first, tile, ext_path.parent)
    return g2tile


def resolve_image(recorded: Path, stem: str, name: str):
    for cand in (recorded,
                 BUILDINGS_DIR / f"{stem}.png",
                 BUILDINGS_DIR / f"{name}.png"):
        if cand and cand.exists():
            return cand
    return None


def shape_world_polygon(o, nw, nh, flip_x, flip_y, px0, py0, pw, ph, notes,
                         tag):
    """A Tiled tile-collision object -> a closed polygon in WORLD pixels,
    or None if it carries no usable area."""
    ox, oy = o.get("x", 0.0), o.get("y", 0.0)
    if o.get("polygon") or o.get("polyline"):
        pts = [(ox + p["x"], oy + p["y"])
               for p in (o.get("polygon") or o.get("polyline"))]
    elif o.get("ellipse"):
        notes.append(f"{tag}: ellipse skipped (no ellipse fill) — use "
                      f"rect/polygon for the wall base")
        return None
    else:
        w, h = o.get("width", 0.0), o.get("height", 0.0)
        if w <= 0.0 or h <= 0.0:
            return None
        if nw > 1 and nh > 1 and w * h >= 0.95 * nw * nh:
            notes.append(f"{tag}: collision rect covers the whole sprite "
                          f"— this blocks walk-behind; trace the wall base")
        pts = [(ox, oy), (ox + w, oy), (ox + w, oy + h), (ox, oy + h)]

    sx, sy = pw / nw, ph / nh
    out = []
    for vx, vy in pts:
        if flip_x:
            vx = nw - vx
        if flip_y:
            vy = nh - vy
        out.append((px0 + vx * sx, py0 + vy * sy))
    return out


def fill_polygon(grid, poly):
    """Even-odd scanline fill of a world-space polygon into a SIZE*SIZE
    bytearray (1 = solid). Pixel centres are sampled at y+0.5."""
    n = len(poly)
    ys = [p[1] for p in poly]
    y0 = max(0, int(min(ys)))
    y1 = min(SIZE - 1, int(max(ys)) + 1)
    for y in range(y0, y1 + 1):
        yc = y + 0.5
        xs = []
        for i in range(n):
            ax, ay = poly[i]
            bx, by = poly[(i + 1) % n]
            if (ay > yc) != (by > yc):
                xs.append(ax + (yc - ay) * (bx - ax) / (by - ay))
        xs.sort()
        row = y * SIZE
        for k in range(0, len(xs) - 1, 2):
            xa = max(0, int(xs[k] + 0.5))
            xb = min(SIZE - 1, int(xs[k + 1] - 0.5))
            for x in range(xa, xb + 1):
                grid[row + x] = 1


def fill_rect(grid, rx, ry, rw, rh):
    x0 = max(0, int(rx))
    x1 = min(SIZE - 1, int(rx + rw) - 1)
    y0 = max(0, int(ry))
    y1 = min(SIZE - 1, int(ry + rh) - 1)
    for y in range(y0, y1 + 1):
        row = y * SIZE
        for x in range(x0, x1 + 1):
            grid[row + x] = 1


def fill_ellipse(grid, ex, ey, ew, eh):
    """Tiled ellipse: x,y is the bounding-box top-left, w/h its size."""
    if ew <= 0.0 or eh <= 0.0:
        return
    cx, cy = ex + ew / 2.0, ey + eh / 2.0
    rx, ry = ew / 2.0, eh / 2.0
    y0 = max(0, int(ey))
    y1 = min(SIZE - 1, int(ey + eh - 1))
    for y in range(y0, y1 + 1):
        dy = (y + 0.5 - cy) / ry
        if abs(dy) >= 1.0:
            continue
        half = rx * (1.0 - dy * dy) ** 0.5
        xa = max(0, int(cx - half + 0.5))
        xb = min(SIZE - 1, int(cx + half - 0.5))
        row = y * SIZE
        for x in range(xa, xb + 1):
            grid[row + x] = 1


def collision_layer_world_poly(o):
    """A free shape on the map-coordinate "Collision" layer -> a closed
    world polygon, or None if it is a rect / ellipse (filled directly)."""
    pts = o.get("polygon") or o.get("polyline")
    if not pts:
        return None
    ox, oy = o.get("x", 0.0), o.get("y", 0.0)
    return [(ox + p["x"], oy + p["y"]) for p in pts]


def write_rgb_png(path, grid):
    """Pure-stdlib PNG: 8-bit truecolour. solid(1) -> black, else white.
    RGB (no alpha) so any image editor round-trips it unambiguously."""
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    raw = bytearray()
    white = b"\xff\xff\xff"
    black = b"\x00\x00\x00"
    for y in range(SIZE):
        raw.append(0)  # filter: None
        row = y * SIZE
        raw += b"".join(black if grid[row + x] else white
                        for x in range(SIZE))
    ihdr = struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    path.write_bytes(png)
    return png


def write_guide_best_effort(grid, notes):
    """Optional: dimmed base map + red solid overlay. Pillow-only."""
    try:
        from PIL import Image
    except ImportError:
        notes.append("Pillow absent — collision_mask_guide.png / "
                      "worldmap.png previews skipped (not required)")
        return
    if not BASE_MAP.exists():
        return
    base = Image.open(BASE_MAP).convert("RGB").resize(
        (SIZE, SIZE), Image.NEAREST)
    guide = base.point(lambda v: v // 3)
    px = guide.load()
    for y in range(SIZE):
        row = y * SIZE
        for x in range(SIZE):
            if grid[row + x]:
                px[x, y] = (255, 40, 40)
    guide.save(MASK_GUIDE, optimize=True)


def _read_png_rgb(path):
    """Minimal stdlib PNG reader: 8-bit greyscale / truecolour (+alpha).
    Returns (width, height, channels, pixel_bytes). Mirrors the pure
    writer already in this file so the tool keeps zero dependencies."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise SystemExit(f"{path}: not a PNG")
    pos = 8
    width = height = bit_depth = colour = None
    idat = bytearray()
    while pos < len(data):
        ln = struct.unpack(">I", data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + ln]
        pos += 12 + ln
        if tag == b"IHDR":
            width, height, bit_depth, colour = struct.unpack(
                ">IIBB", body[:10])
        elif tag == b"IDAT":
            idat += body
        elif tag == b"IEND":
            break
    if bit_depth != 8 or colour not in (0, 2, 6):
        raise SystemExit(f"{path}: unsupported PNG (depth {bit_depth}, "
                         f"colour {colour}) — need 8-bit grey/RGB/RGBA")
    channels = {0: 1, 2: 3, 6: 4}[colour]
    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    out = bytearray(height * stride)

    def paeth(a, b, c):
        p = a + b - c
        pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
        return a if pa <= pb and pa <= pc else (b if pb <= pc else c)

    src = 0
    prev = bytearray(stride)
    for y in range(height):
        ft = raw[src]
        src += 1
        line = bytearray(raw[src:src + stride])
        src += stride
        if ft == 1:
            for x in range(channels, stride):
                line[x] = (line[x] + line[x - channels]) & 255
        elif ft == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif ft == 3:
            for x in range(stride):
                a = line[x - channels] if x >= channels else 0
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 255
        elif ft == 4:
            for x in range(stride):
                a = line[x - channels] if x >= channels else 0
                c = prev[x - channels] if x >= channels else 0
                line[x] = (line[x] + paeth(a, prev[x], c)) & 255
        out[y * stride:(y + 1) * stride] = line
        prev = line
    return width, height, channels, bytes(out)


def bake_water_from_base(grid):
    """Mark every clearly-blue pixel in the top RIVER_MAX_Y rows of
    worldmap_base.png solid, so the river + its bridge gap are
    pixel-identical to the painted art. Returns the solid count."""
    if not BASE_MAP.exists():
        return 0
    w, h, ch, px = _read_png_rgb(BASE_MAP)
    n = 0
    y_max = min(RIVER_MAX_Y, h, SIZE)
    for y in range(y_max):
        base_row = y * w * ch
        grid_row = y * SIZE
        for x in range(min(w, SIZE)):
            i = base_row + x * ch
            if is_water_rgb(px[i], px[i + 1], px[i + 2]):
                grid[grid_row + x] = 1
                n += 1
    return n


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: tiled_to_world.py <map.tmj>")
    tmj = Path(sys.argv[1]).resolve()
    raw = json.loads(tmj.read_text(encoding="utf-8"))
    g2tile = load_tilesets(tmj.parent, raw)

    grid = bytearray(SIZE * SIZE)  # 0 walkable, 1 solid
    placed = []
    notes = []
    n_walls = 0
    n_props = 0

    for layer in raw.get("layers", []):
        if layer.get("type") != "objectgroup":
            continue

        if layer.get("name") == "Collision":
            for o in layer.get("objects", []):
                if o.get("gid"):
                    continue  # a stray tile on the wrong layer — ignore
                if o.get("ellipse"):
                    fill_ellipse(grid, o.get("x", 0.0), o.get("y", 0.0),
                                 o.get("width", 0.0), o.get("height", 0.0))
                    n_props += 1
                elif o.get("polygon") or o.get("polyline"):
                    poly = collision_layer_world_poly(o)
                    if poly and len(poly) >= 3:
                        fill_polygon(grid, poly)
                        n_props += 1
                else:
                    w, h = o.get("width", 0.0), o.get("height", 0.0)
                    if w > 0.0 and h > 0.0:
                        fill_rect(grid, o.get("x", 0.0), o.get("y", 0.0),
                                  w, h)
                        n_props += 1
            continue

        for obj in layer.get("objects", []):
            raw_gid = obj.get("gid")
            if not raw_gid:
                continue

            gid = raw_gid & GID_MASK
            flip_x = bool(raw_gid & FLIP_H)
            flip_y = bool(raw_gid & FLIP_V)

            tile = g2tile.get(gid)
            if tile is None:
                notes.append(f"gid {gid}: no tileset entry — skipped")
                continue
            recorded = tile["img"]
            name = canonical_name(str(recorded))

            if raw_gid & FLIP_D:
                raise SystemExit(
                    f"FATAL: {name} (gid {gid}) has a DIAGONAL flip. "
                    f"Un-rotate it in Tiled before re-exporting.")

            pw = float(obj["width"])
            ph = float(obj["height"])
            px0 = float(obj["x"])
            py0 = float(obj["y"]) - ph        # Tiled tile objects: BL anchor
            placed.append((name, int(round(px0)), int(round(py0)),
                           int(round(pw)), int(round(ph)), flip_x, flip_y))

            if name in NO_TILE_OVERLAY:
                continue

            nw, nh = tile["nw"], tile["nh"]
            had_shape = False
            if tile["shapes"] and nw > 0 and nh > 0:
                for s in tile["shapes"]:
                    poly = shape_world_polygon(
                        s, nw, nh, flip_x, flip_y, px0, py0, pw, ph,
                        notes, name)
                    if poly and len(poly) >= 3:
                        fill_polygon(grid, poly)
                        had_shape = True
                        n_walls += 1
            if not had_shape:
                notes.append(f"{name}: no wall base traced — contributes "
                              f"NO collision (trace it, or paint it into "
                              f"collision_mask.png)")

    n_water = bake_water_from_base(grid)

    png = write_rgb_png(MASK_SHIP, grid)
    MASK_BASE.write_bytes(png)  # byte-identical pristine fallback

    write_guide_best_effort(grid, notes)

    n_solid = sum(grid)
    print(f"✓ baked {MASK_SHIP.name} (+ {MASK_BASE.name} fallback): "
          f"{n_walls} wall bases + {n_props} Collision-layer props + "
          f"{n_water} river px (from base art) → {n_solid} solid px "
          f"({100.0 * n_solid / (SIZE * SIZE):.1f}% of map)")
    for n in notes:
        print(f"  ! {n}")

    print()
    print(f"// {len(placed)} buildings — paste into include/Buildings.h "
          f"kAll[]:")
    for name, x, y, w, h, fx, fy in placed:
        print(
            f'    {{"{name}", '
            f'{{{float(x):7.1f}f, {float(y):7.1f}f, '
            f'{float(w):6.1f}f, {float(h):6.1f}f}}, '
            f'{str(fx).lower()}, {str(fy).lower()}}},'
        )


if __name__ == "__main__":
    main()
