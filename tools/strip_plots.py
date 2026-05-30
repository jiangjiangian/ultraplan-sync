#!/usr/bin/env python3
"""把 worldmap_base.png 上米色的建築基地矩形清掉、改塗成草地，同時保留
河流、廣場、步道、跑道、道路與大門不動。

做法：
  1. 遮罩 = RGB 落在米色基地範圍內的像素。
  2. 在空間上排除真正屬於米色地物的區域（頂端河岸帶、中央廣場圓盤、
     大門柱、道路）。
  3. 用取樣到的草地 tile 取代剩下的遮罩像素，讓紋理與周圍草坪融合。

需要時可跑兩次——先以 --preview 旗標（WRITE_MASK_PREVIEW=True）檢視會被
動到的區域，確認後再不帶旗標執行以實際寫入。

用法：
    python3 tools/strip_plots.py [--preview]
"""
from PIL import Image
import numpy as np
import sys

INP = "resources/assets/maps/worldmap_base.png"
OUT = INP
MASK_PREVIEW = "tools/_plot_mask_preview.png"

WRITE_MASK_PREVIEW = "--preview" in sys.argv

# 米色基地的 RGB 範圍（從現有影像取樣而來）
BEIGE_LO = np.array([170, 145, 110], dtype=np.int32)
BEIGE_HI = np.array([225, 200, 165], dtype=np.int32)

# 要「保留」（排除於遮罩之外）的區域——保護真正屬於米色的地物。
# 全部以影像像素座標表示，2048x2048。
def build_keep_mask(H: int, W: int) -> np.ndarray:
    """建立保留遮罩，標出不可被重塗成草地的米色地物區域。

    參數：
        H：影像高度（像素）。
        W：影像寬度（像素）。
    回傳：
        形狀為 (H, W) 的 bool 陣列，True 代表該像素需保留。
    """
    keep = np.zeros((H, W), dtype=bool)

    # 1. 頂端河岸帶（沙色條帶 + 河流本身）
    keep[0:430, :] = True

    # 2. 中央廣場圓盤——以米色石材鋪成的放射狀廣場
    yy, xx = np.ogrid[:H, :W]
    cx, cy, r = W // 2, H // 2, 280
    keep |= (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r

    # 3. 底部道路 + 大門帶
    keep[1850:, :] = True

    # 4. 大門柱（道路與廣場步道之間的石柱）
    keep[1780:1900, 880:1180] = True

    return keep


def main():
    """主流程：把米色基地像素重塗成草地紋理並覆寫 worldmap_base.png。

    無參數、無回傳值。先算出米色遮罩並扣除保留區域，若帶 --preview 旗標
    則只輸出遮罩預覽圖；否則在草坪上取樣乾淨草地圖塊，逐格隨機填補遮罩
    像素，再做一輪膨脹把基地外框線一併吞掉，最後存回原檔。
    """
    img = np.array(Image.open(INP).convert("RGB"))
    H, W, _ = img.shape
    print(f"loaded {W}x{H}")

    # 米色遮罩
    rgb = img.astype(np.int32)
    in_range = np.all((rgb >= BEIGE_LO) & (rgb <= BEIGE_HI), axis=2)

    keep = build_keep_mask(H, W)
    plot_mask = in_range & ~keep
    print(f"plot pixels to repaint: {plot_mask.sum():,} "
          f"({100 * plot_mask.sum() / (H * W):.1f}% of image)")

    if WRITE_MASK_PREVIEW:
        preview = img.copy()
        preview[plot_mask] = [255, 0, 255]   # 亮洋紅 = 會被重塗
        preview[keep & in_range] = [0, 255, 255]  # 青色 = 受保護的米色
        Image.fromarray(preview).save(MASK_PREVIEW)
        print(f"preview written -> {MASK_PREVIEW}")
        return

    # 在草坪上掃描「嚴格乾淨」的草地圖塊：每個像素都必須綠色為主、且亮度
    # 落在很窄的區間內——藉此排除含有樹木、步道、基地或陰影像素的圖塊。
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

    # 每個圖塊隨機產生 8 種方位，打散重複感。
    rng = np.random.default_rng(seed=42)
    pool = []
    for p in candidates[:64]:
        for k in range(4):
            r = np.rot90(p, k=k)
            pool.append(r)
            pool.append(np.fliplr(r))
    print(f"texture pool size: {len(pool)}")

    # 以「每個圖塊格隨機取一個草地圖塊」的方式重塗遮罩像素——圖塊格大小
    # 等於圖塊尺寸，因此能整齊對接（不重疊、不留縫）。每格都從池中隨機挑
    # 一個方位，使相鄰兩格不會共用同一塊微紋理。
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

    # 輕量膨脹一輪，把基地深色外框線（1 像素邊框）吞掉。
    # 思路：對每個遮罩像素檢查其 8 個鄰居，只要有鄰居落在米色範圍內就一併
    # 重塗；1 像素邊框跑一輪即可。這裡採較粗略的做法：先把 plot_mask 膨脹
    # 1 像素，再重做一次取代。
    from scipy.ndimage import binary_dilation
    try:
        dil = binary_dilation(plot_mask, iterations=4)
        dil &= ~keep
        border = dil & ~plot_mask
        bpx = img[border]
        # 邊框像素判定：綠色並非明顯主導（藉此排除純草地）。
        r = bpx[:, 0].astype(np.int32)
        g = bpx[:, 1].astype(np.int32)
        b = bpx[:, 2].astype(np.int32)
        bright = (r + g + b) // 3
        # 深色邊框（亮度低）或米色外框線（G 不主導 + 暖色調）兩者擇一。
        is_border = (
            (bright < 150)
            | ((g <= r + 5) & (r > 140) & (r < 220))
        )
        idx = np.where(border)
        sel_y = idx[0][is_border]
        sel_x = idx[1][is_border]
        # 每個像素都從池中隨機取圖塊，避免整片變成同一塊紋理
        bp_pick = rng.integers(0, len(pool), size=len(sel_y))
        # 依圖塊格分組以攤平成本
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
