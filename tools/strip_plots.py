#!/usr/bin/env python3
"""Strip the beige building-plot rectangles from worldmap_base.png and
re-tint them as grass, leaving the river, plaza, paths, track, road and
gate untouched.

Method:
  1. Mask = pixels whose RGB falls inside the beige plot range.
  2. Spatially exclude the regions where legitimate beige features live
     (riverbank top band, central plaza disc, gate posts, road).
  3. Replace the remaining mask pixels with a sampled grass tile so the
     texture blends with the surrounding lawn.

Run twice if needed — once with WRITE_MASK_PREVIEW=True to see what
gets touched, then with False to commit.
"""
from PIL import Image
import numpy as np
import sys

INP = "resources/assets/maps/worldmap_base.png"
OUT = INP
MASK_PREVIEW = "tools/_plot_mask_preview.png"

WRITE_MASK_PREVIEW = "--preview" in sys.argv

# Beige plot RGB range (sampled from existing image)
BEIGE_LO = np.array([170, 145, 110], dtype=np.int32)
BEIGE_HI = np.array([225, 200, 165], dtype=np.int32)

# Regions to KEEP (exclude from mask) — protect legit beige features.
# All in image pixel coords, 2048x2048.
def build_keep_mask(H: int, W: int) -> np.ndarray:
    keep = np.zeros((H, W), dtype=bool)

    # 1. Top riverbank band (sandy strip + river itself)
    keep[0:430, :] = True

    # 2. Central plaza disc — radial plaza of beige stones
    yy, xx = np.ogrid[:H, :W]
    cx, cy, r = W // 2, H // 2, 280
    keep |= (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r

    # 3. Bottom road + gate band
    keep[1850:, :] = True

    # 4. Gate posts (stone pillars between road and plaza paths)
    keep[1780:1900, 880:1180] = True

    return keep


def main():
    img = np.array(Image.open(INP).convert("RGB"))
    H, W, _ = img.shape
    print(f"loaded {W}x{H}")

    # Beige mask
    rgb = img.astype(np.int32)
    in_range = np.all((rgb >= BEIGE_LO) & (rgb <= BEIGE_HI), axis=2)

    keep = build_keep_mask(H, W)
    plot_mask = in_range & ~keep
    print(f"plot pixels to repaint: {plot_mask.sum():,} "
          f"({100 * plot_mask.sum() / (H * W):.1f}% of image)")

    if WRITE_MASK_PREVIEW:
        preview = img.copy()
        preview[plot_mask] = [255, 0, 255]   # bright magenta = will repaint
        preview[keep & in_range] = [0, 255, 255]  # cyan = protected beige
        Image.fromarray(preview).save(MASK_PREVIEW)
        print(f"preview written -> {MASK_PREVIEW}")
        return

    # Scan the lawn for STRICTLY-clean grass patches: every pixel must be
    # green-dominant and within a tight brightness band — rejects patches
    # that contain trees, paths, plots, or shadow pixels.
    GS = 32
    candidates = []
    for ay in range(400, 1750, 32):
        for ax in range(30, 2000, 32):
            p = img[ay:ay + GS, ax:ax + GS]
            if p.shape[:2] != (GS, GS):
                continue
            r = p[..., 0].astype(np.int32)
            g = p[..., 1].astype(np.int32)
            b = p[..., 2].astype(np.int32)
            green_dom = (g > r + 15) & (g > b + 15)
            bright_ok = (g > 130) & (g < 210)
            if green_dom.all() and bright_ok.all():
                candidates.append(p.copy())
    print(f"found {len(candidates)} strict-clean grass patches")
    if not candidates:
        raise SystemExit("no strict-clean grass anchor patches found")

    # Random 8-way orientations per patch to break repetition.
    rng = np.random.default_rng(seed=42)
    pool = []
    for p in candidates[:64]:
        for k in range(4):
            r = np.rot90(p, k=k)
            pool.append(r)
            pool.append(np.fliplr(r))
    print(f"texture pool size: {len(pool)}")

    # Repaint masked pixels by sampling a random patch per tile cell — tile
    # cells equal the patch size so they butt-join cleanly (no overlap, no
    # gap). Each cell picks a random orientation from the pool, so no two
    # adjacent cells share the same micro-texture.
    out = img.copy()
    TILE = GS
    plot_pixels = np.argwhere(plot_mask)
    ty = plot_pixels[:, 0] // TILE
    tx = plot_pixels[:, 1] // TILE
    tile_ids = ty * 10000 + tx
    unique_tiles, inverse = np.unique(tile_ids, return_inverse=True)
    pick = rng.integers(0, len(pool), size=len(unique_tiles))
    for i in range(len(unique_tiles)):
        mask_i = inverse == i
        idx = plot_pixels[mask_i]
        ys = idx[:, 0]
        xs = idx[:, 1]
        ly = ys % TILE
        lx = xs % TILE
        out[ys, xs] = pool[pick[i]][ly, lx]

    # Light dilation pass to swallow the dark plot outline pixels (1-px border).
    # Approach: for each masked pixel, check its 8 neighbours; if any neighbour
    # is in_range, repaint it too. One pass is enough for a 1-px border.
    # Use a coarser approach: dilate plot_mask by 1 px then redo replacement.
    from scipy.ndimage import binary_dilation
    try:
        dil = binary_dilation(plot_mask, iterations=4)
        dil &= ~keep
        border = dil & ~plot_mask
        bpx = img[border]
        # Border pixel test: not strongly green-dominant (excludes pure grass).
        r = bpx[:, 0].astype(np.int32)
        g = bpx[:, 1].astype(np.int32)
        b = bpx[:, 2].astype(np.int32)
        bright = (r + g + b) // 3
        # Either dark border (low brightness) OR beige border outline
        # (G not dominant + warm tones).
        is_border = (
            (bright < 150)
            | ((g <= r + 5) & (r > 140) & (r < 220))
        )
        idx = np.where(border)
        sel_y = idx[0][is_border]
        sel_x = idx[1][is_border]
        # Sample from random pool patches per pixel to avoid a uniform tile
        bp_pick = rng.integers(0, len(pool), size=len(sel_y))
        # Group by tile to amortise
        ty2 = sel_y // GS
        tx2 = sel_x // GS
        tids = ty2 * 10000 + tx2
        u, inv = np.unique(tids, return_inverse=True)
        upick = rng.integers(0, len(pool), size=len(u))
        for i in range(len(u)):
            m = inv == i
            ys = sel_y[m]
            xs = sel_x[m]
            out[ys, xs] = pool[upick[i]][ys % GS, xs % GS]
        print(f"swallowed {is_border.sum():,} border pixels")
    except ImportError:
        print("scipy not available — skipping border dilation pass")

    Image.fromarray(out).save(OUT, optimize=True)
    print(f"saved -> {OUT}")


if __name__ == "__main__":
    main()
