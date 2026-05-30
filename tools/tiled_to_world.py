#!/usr/bin/env python3
"""從 Tiled JSON 地圖輸出 Buildings.h kAll[]，並烘焙地形碰撞遮罩。

工作流程：
  1. 在 Tiled 裡：
       File -> New Map...  正交、64x64 格、每格 32px（= 2048x2048）。
       影像圖層「Base」-> resources/assets/maps/worldmap_base.png。
       Tileset「buildings」-> resources/assets/buildings_3d_trimmed/ 內
         的每張 PNG。
       物件圖層「Buildings」-> Insert Tile，放置每棟建築
         （可水平／垂直翻轉以增加朝向變化）。
       對每張建築素材：在 Tile Collision Editor 中打開該 tile，用多邊形／
         矩形描出「實心牆基」（與地面相接的部分）。屋頂與屋簷要留在形狀
         「之外」，玩家才能從建築後方走過。
  2. File -> Export As... -> Tiled JSON，例如 tools/world.tmj
  3. python3 tools/tiled_to_world.py tools/world.tmj

地形碰撞有兩種編寫方式，兩者皆精準烘焙（像素級對齊，不做格點量化、不退回
佔地外框）：
  a. 建築牆基——在其 tile 上使用 Tile Collision Editor。
  b. 樹木／花台／校園外牆——在名為「Collision」的地圖座標物件圖層上自由
     畫矩形／多邊形／橢圓（已畫進 worldmap_base.png 的地物沒有對應 tile
     可掛形狀）。
河流為硬編碼。未描繪牆基的建築不產生任何碰撞（因此校門口保持可通行——
不會有任何東西橫畫在上面）。

輸出：
  - stdout：include/Buildings.h kAll[] 可貼上的區塊（貼圖矩形 + 翻轉旗標
    ——也就是 BuildingTracker 用來掛章節事件的觸發區）。
  - resources/assets/maps/collision_mask.png——引擎實際載入、隨產品出貨的
    遮罩。每次執行都會從 Tiled 重新產生（牆基 +「Collision」圖層形狀 +
    河流）。白 = 可走、黑 = 實心，採 RGB 讓任何編輯器都能無損往返。可手動
    微調做快速實驗，但再次執行本工具會覆寫它——Tiled 才是唯一真實來源。
  - resources/assets/maps/collision_mask_base.png——位元組完全相同的原始
    副本；若 collision_mask.png 遺失，引擎會退回使用它，地形才不會無聲消失。
  - resources/assets/maps/collision_mask_guide.png——把 worldmap_base 調暗、
    並將實心像素染紅的視覺檢查圖。盡力而為：僅在可匯入 Pillow 時才產生。

碰撞模型：
  引擎會針對玩家佔地下方的每個像素取樣 collision_mask.png（見
  include/CollisionMask.h）：像素為純白或全透明才可走，否則為實心。

無第三方相依：PNG 寫出器與多邊形光柵化皆為純標準函式庫（zlib）。若有
Pillow，也只是加上選用的調暗指引圖／貼圖預覽。

穩健性：
  - 查 tileset 時會剝掉 Tiled 的翻轉位元（gid 高位），再以 flipX/flipY
    重新輸出；碰撞形狀也會跟著鏡像對齊。建築若帶「對角翻轉」會直接報錯。
  - 輸出的名稱為建築的正規名：尾端的素材變體後綴（_nb2、_test、_old、
    _trimmed、_vN）與任何上層資料夾都會被移除（中正圖書館_nb2 -> 中正圖書館）。
  - 若 tileset 記錄的影像路徑已過時，會改以檔名在
    resources/assets/buildings_3d_trimmed/ 內尋找實際 PNG。
  -「Collision」圖層上的橢圓會以真正的橢圓填滿（樹木／花台的自然形狀），
    而非其外接矩形。
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

# 已畫進 worldmap_base.png——不合成貼圖、也不烘焙碰撞（開放地面／廣場）；
# 仍會以「純觸發」條目輸出到 Buildings.h。
NO_TILE_OVERLAY = {"操場", "羅馬廣場"}

# 河流是「畫」在 worldmap_base.png 頂端的一條弧形帶，並留有可通行的橋樑
# 缺口。手寫矩形容易與圖偏離（把草地封住、漏掉真正的橋），因此河流碰撞改為
# 直接從底圖取樣：像素明顯偏藍才算「水」。如此天生與玩家所見對齊，再也不會
# 漂移。掃描範圍限制在頂端 RIVER_MAX_Y 列內：地圖上唯一的水就是這條河
# （已驗證），此上界也能避免日後其他地方的藍色地物被誤判成河流。
RIVER_MAX_Y = 650


def is_water_rgb(r, g, b):
    """判斷給定 RGB 是否屬於河流水色。

    參數：
        r, g, b：像素的紅、綠、藍分量。
    回傳：
        該像素明顯偏藍（視為水）則為 True，否則 False。
    """
    # 此門檻是針對「目前」的 worldmap_base.png 調色盤校正而來。若該素材日後
    # 重新產生（換調色盤／後製／重算繪），請重新取樣幾個已知水色像素並重調
    # 此門檻——否則河流烘焙會無聲漏抓水、或誤抓到非水像素。
    return b > 90 and b > r + 20 and b > g

_SUFFIX_RE = re.compile(r"_(nb\d+|test|old|trimmed|v\d+)$")
_TRAIL_DIGITS_RE = re.compile(r"\d+$")


def canonical_name(image_path: str) -> str:
    """從影像路徑推出建築的正規名稱。

    參數：
        image_path：tile 影像的路徑或檔名字串。
    回傳：
        去掉素材變體後綴與尾端數字後的建築名稱。
    """
    stem = _SUFFIX_RE.sub("", Path(image_path).stem)
    return _TRAIL_DIGITS_RE.sub("", stem)


def load_tilesets(tmj_dir: Path, raw: dict) -> dict:
    """解析 Tiled 地圖的 tileset，建立 gid 對 tile 資訊的對照表。

    參數：
        tmj_dir：.tmj 檔所在目錄，用來解析相對影像路徑。
        raw：已解析的 Tiled JSON 字典。
    回傳：
        以全域 gid 為鍵、值為含影像路徑、原始寬高與碰撞形狀的 dict。
    """
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
    """依序嘗試幾個候選路徑，找出實際存在的 tile 影像。

    參數：
        recorded：tileset 中記錄的影像路徑。
        stem：影像檔主檔名。
        name：建築的正規名稱。
    回傳：
        第一個存在的候選路徑；若皆不存在則回傳 None。
    """
    for cand in (recorded,
                 BUILDINGS_DIR / f"{stem}.png",
                 BUILDINGS_DIR / f"{name}.png"):
        if cand and cand.exists():
            return cand
    return None


def shape_world_polygon(o, nw, nh, flip_x, flip_y, px0, py0, pw, ph, notes,
                         tag):
    """把 Tiled tile 碰撞物件轉成「世界像素座標」中的封閉多邊形。

    會依翻轉旗標鏡像，並由 tile 原始尺寸縮放到貼圖在世界中的實際尺寸。

    參數：
        o：Tiled 物件 dict（多邊形／折線／橢圓／矩形）。
        nw, nh：tile 的原始像素寬高。
        flip_x, flip_y：是否水平／垂直翻轉。
        px0, py0：貼圖左上角在世界中的像素座標。
        pw, ph：貼圖在世界中的像素寬高。
        notes：用來累積警告訊息的清單。
        tag：標示用名稱（通常為建築名），會出現在警告訊息中。
    回傳：
        世界像素座標的頂點清單；若該物件無可用面積（或為橢圓）則回傳 None。
    """
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
    """以奇偶掃描線法把世界座標多邊形填入 SIZE*SIZE 的 bytearray（1 = 實心）。

    像素中心於 y+0.5 取樣。

    參數：
        grid：長度為 SIZE*SIZE 的 bytearray，會就地修改。
        poly：世界像素座標的頂點清單。
    回傳：
        無（就地寫入 grid）。
    """
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
    """把一個世界座標矩形填入 grid（1 = 實心）。

    參數：
        grid：長度為 SIZE*SIZE 的 bytearray，會就地修改。
        rx, ry：矩形左上角的世界座標。
        rw, rh：矩形寬高。
    回傳：
        無（就地寫入 grid）。
    """
    x0 = max(0, int(rx))
    x1 = min(SIZE - 1, int(rx + rw) - 1)
    y0 = max(0, int(ry))
    y1 = min(SIZE - 1, int(ry + rh) - 1)
    for y in range(y0, y1 + 1):
        row = y * SIZE
        for x in range(x0, x1 + 1):
            grid[row + x] = 1


def fill_ellipse(grid, ex, ey, ew, eh):
    """把一個 Tiled 橢圓以真正橢圓形狀填入 grid（1 = 實心）。

    Tiled 橢圓的 x,y 為外接矩形左上角，w/h 為其尺寸。

    參數：
        grid：長度為 SIZE*SIZE 的 bytearray，會就地修改。
        ex, ey：外接矩形左上角的世界座標。
        ew, eh：橢圓的寬高。
    回傳：
        無（就地寫入 grid）。
    """
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
    """把「Collision」圖層上的自由形狀轉成封閉的世界多邊形。

    參數：
        o：地圖座標物件圖層上的 Tiled 物件 dict。
    回傳：
        世界座標頂點清單；若物件為矩形／橢圓（會另行直接填滿）則回傳 None。
    """
    pts = o.get("polygon") or o.get("polyline")
    if not pts:
        return None
    ox, oy = o.get("x", 0.0), o.get("y", 0.0)
    return [(ox + p["x"], oy + p["y"]) for p in pts]


def write_rgb_png(path, grid):
    """以純標準函式庫寫出 8 位元真彩 PNG：實心(1) -> 黑，其餘 -> 白。

    採 RGB（不含 alpha），讓任何影像編輯器都能無歧義地往返。

    參數：
        path：輸出 PNG 的路徑。
        grid：長度為 SIZE*SIZE 的 bytearray（1 = 實心）。
    回傳：
        寫出的 PNG 位元組內容（bytes），方便呼叫端直接重用。
    """
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    raw = bytearray()
    white = b"\xff\xff\xff"
    black = b"\x00\x00\x00"
    for y in range(SIZE):
        raw.append(0)  # 濾波器：None
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
    """選用：輸出「調暗底圖 + 紅色實心疊層」的視覺檢查圖（僅在有 Pillow 時）。

    參數：
        grid：長度為 SIZE*SIZE 的 bytearray（1 = 實心）。
        notes：用來累積警告訊息的清單（缺 Pillow 或缺底圖時會略過）。
    回傳：
        無；成功時寫出 MASK_GUIDE。
    """
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
    """以標準函式庫實作的極簡 PNG 讀取器：支援 8 位元灰階／真彩（含 alpha）。

    與本檔已有的純寫出器相對應，讓本工具維持零相依。

    參數：
        path：要讀取的 PNG 路徑。
    回傳：
        (width, height, channels, pixel_bytes) 四元組。
    """
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
    """把 worldmap_base.png 頂端 RIVER_MAX_Y 列內所有明顯偏藍的像素標為實心。

    如此河流與其橋樑缺口便與所繪素材逐像素一致。

    參數：
        grid：長度為 SIZE*SIZE 的 bytearray，會就地修改。
    回傳：
        本步驟新標為實心的像素數。
    """
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
    """主流程：讀 Tiled 地圖、烘焙碰撞遮罩並輸出 Buildings.h kAll[]。

    從命令列取得 .tmj 路徑，逐圖層處理「Collision」自由形狀與建築 tile 的
    牆基，再加上從底圖取樣的河流，寫出 collision_mask.png（及其備援與指引圖），
    最後把各建築的貼圖矩形與翻轉旗標印到 stdout。無回傳值。
    """
    if len(sys.argv) != 2:
        raise SystemExit("Usage: tiled_to_world.py <map.tmj>")
    tmj = Path(sys.argv[1]).resolve()
    raw = json.loads(tmj.read_text(encoding="utf-8"))
    g2tile = load_tilesets(tmj.parent, raw)

    grid = bytearray(SIZE * SIZE)  # 0 可走、1 實心
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
                    continue  # 誤放在錯誤圖層上的零星 tile——忽略
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
            py0 = float(obj["y"]) - ph        # Tiled tile 物件以左下角為錨點
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
    MASK_BASE.write_bytes(png)  # 位元組完全相同的原始備援

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
