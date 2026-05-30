#!/usr/bin/env python3
"""把所有建築 tile 合成到世界地圖底圖上。

讀入 worldmap_base.png 底圖與 buildings_topdown/ 內各建築 tile，去背後
依 BUILDINGS 設定的位置貼上，輸出合成後的 resources/assets/maps/worldmap.png，
同時印出可貼進 include/Buildings.h kAll[] 的觸發框資料。

用法：
    python3 tools/composite_worldmap.py

輸出：resources/assets/maps/worldmap.png

要調整建築位置，請編輯下方的 BUILDINGS dict；其值為 2048x2048 輸出畫布中的
(x 中心, y 中心, 目標像素高度)。
"""
from PIL import Image, ImageDraw
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "resources" / "assets"
BASE_MAP = ASSETS / "maps" / "worldmap_base.png"
# 從 buildings_3d（斜 3/4 視角）改用 buildings_topdown（純俯視平面圖）。
# 俯視的外框緊貼可見佔地，因此觸發框與碰撞框會對齊玩家所見，不會像斜視角
# 那樣因屋頂屋簷外伸而超出牆面。
BUILDINGS_DIR = ASSETS / "buildings_topdown"
OUTPUT = ASSETS / "maps" / "worldmap.png"

OUTPUT_SIZE = 2048
GRAY_TOLERANCE = 35

# 2048x2048 畫布中的 (cx, cy, target_height_px)。
#
# 版面對齊新版底圖 (resources/assets/maps/worldmap_base.png)：
#   - 靛藍色河流沿北側內緣由東向西流（y 約 400-600）
#   - 四維大道橫向道路約在 y 約 1340-1400
#   - 指南路南側步道在 y 約 1820-1900
#   - 中央廣場含噴水池繪於 (~1000, 1080)
#   - 跑道橢圓繪於東北象限 (~1400, 760)
#
# 俯視 tile 的長寬比不一，故每棟建築各自挑選 target_h，使貼上後的寬度
# 維持在約 330 px 以內，並讓相鄰建築之間留出可通行的走道。錨點位置依照
# 真實的政大山下地圖：
#   西北 -> 游泳館 / 樂活館；東北 -> 操場 / 體育館；
#   中央 -> 中正圖書館 + 羅馬廣場；東南 -> 大字學術群；
#   西南角 -> 緊鄰指南路的正門。
BUILDINGS = {
    # ===== 北區（介於河流 y~560 與廣場列 y~1100 之間）=====
    # 偏北的西側列，由西到東
    "游泳館":        ( 200,  720, 180),
    "樂活館":        ( 520,  720, 180),
    "研究大樓":      ( 820,  720, 160),
    # 圖書館位於西側建築群之南
    "中正圖書館":    ( 360,  990, 200),
    # 東北：疊在底圖的跑道 + 體育館那一列上
    "體育館":        (1750,  610, 180),
    # 東側中段：政治 / 民族學群的替代物
    "綜合院館":      (1700,  900, 180),
    "校友服務中心":   (1880, 1100, 180),

    # ===== 南側第 1 列（緊鄰四維大道之南，y~1430）=====
    "行政大樓":      ( 180, 1430, 200),
    "樂活小舖":      ( 440, 1430, 180),
    "商學院":        ( 720, 1430, 180),
    "新聞館":        ( 985, 1430, 180),
    "資訊大樓":      (1280, 1430, 180),
    "風雩樓":        (1600, 1430, 180),
    "大仁樓":        (1920, 1430, 140),

    # ===== 南側第 2 列（y~1620），大字學群向東延伸 =====
    "法學院":        ( 110, 1620, 200),
    "井塘樓":        ( 330, 1620, 180),
    "四維堂":        ( 580, 1620, 180),
    "果夫樓":        ( 870, 1620, 160),
    "集英樓":        (1140, 1620, 180),
    "大勇樓":        (1430, 1620, 180),
    "學思樓":        (1730, 1620, 180),

    # ===== 南側第 3 列（y~1770），容納溢位建築 + 長廊 =====
    "風雩走廊":      ( 900, 1770, 100),
    "志希樓":        (1330, 1770, 140),
    "大智樓":        (1620, 1770, 140),

    # ===== 西南側大門，獨立於指南路上 =====
    "正門":         ( 180, 1890, 140),
}

# 這些建築的 tile 不會被合成（底圖已畫出其外觀——東北的跑道橢圓、中央的
# 噴水池廣場），但仍需要一個給 BuildingTracker 用的觸發框。它們會與被合成的
# 建築一起輸出到 Buildings.h kAll[] 區塊；其 (cx, cy, w, h) 描述的是世界像素
# 中的觸發佔地，而非 tile 的擺放位置。
TRIGGER_ONLY = {
    # 跑道內側場地，位於 Obstacles.h 那些外圍條帶之內。
    # 玩家可從 x [1380, 1480] 的南側缺口進入場地，觸發操場進場事件。
    "操場":      (1445, 810, 390, 160),
    # 圍繞底圖所繪噴水池徽章的中央廣場。
    "羅馬廣場":  (1000, 1080, 210, 200),
}


def remove_background(tile_path: Path) -> Image.Image:
    """以四個角落的 flood-fill 去除灰底背景。

    參數：
        tile_path：來源 tile PNG 的路徑。
    回傳：
        去背並裁到前景外框的 RGBA Image。
    """
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
    """依目標高度等比例縮放，採最近鄰取樣（保留像素風）。

    參數：
        img：來源 Image。
        target_h：縮放後的目標像素高度。
    回傳：
        縮放後維持原長寬比的 Image。
    """
    w, h = img.size
    new_w = max(1, round(w * target_h / h))
    return img.resize((new_w, target_h), Image.NEAREST)


def main():
    """主流程：讀底圖、合成所有建築 tile、輸出 worldmap.png 並印出 Buildings.h 觸發框。

    無參數、無回傳值。依 BUILDINGS 設定逐棟去背、縮放後貼到底圖上，存檔後
    再把實際貼上的矩形（以及 TRIGGER_ONLY 的純觸發框）格式化成可貼進
    include/Buildings.h kAll[] 的資料印到 stdout。
    """
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

    # 用「實際貼上的矩形」輸出 Buildings.h kAll[] 條目，而非由 target_h
    # 推出的正方形。合成時每張 tile 會先裁到前景外框再縮放，所以寬扁的
    # 俯視建築會得到寬扁的貼圖；若兩軸都用 target_h，會留下玩家可踩進去的
    # 「觸發框／視覺」落差。TRIGGER_ONLY 條目（操場、廣場）則原樣附在後面
    # ——它們的外觀已畫在底圖上，不會再貼任何 tile。
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
