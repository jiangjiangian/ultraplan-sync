#!/usr/bin/env python3
"""一次性前處理：把 resources/assets/buildings_topdown/ 內每張 PNG 的灰底
留白裁掉，並將裁切結果存到 resources/assets/buildings_topdown_trimmed/。

裁切後的 PNG 就是要匯入 Tiled 當作建築 tile 集合的素材。因為灰底已移除，
Tiled 裡看到的圖像正好就是建築的佔地範圍，畫布上對齊好的樣子就等同於
tiled_to_world.py 會貼到 worldmap.png 上的結果。

用法：
    python3 tools/trim_tiles.py
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC  = ROOT / "resources/assets/buildings_topdown"
DST  = ROOT / "resources/assets/buildings_topdown_trimmed"
TOL  = 35


def trim(src_path: Path) -> Image.Image:
    """裁掉單張 tile 的灰底並回傳裁切後的圖像。

    參數：
        src_path：來源 PNG 的路徑。
    回傳：
        去背並裁到前景外框的 RGBA Image；若全圖皆被去背則回傳原圖。

    做法：從四個角落以 floodfill 把灰底填成洋紅色哨兵值，再把該哨兵色
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
    """主流程：批次裁切 SRC 內所有 PNG 並輸出到 DST。

    無參數、無回傳值。建立輸出資料夾，略過檔名含 _thumb 的縮圖，
    逐張裁切後存檔並印出尺寸，最後印出處理總數。
    """
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
