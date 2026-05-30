#!/usr/bin/env python3
"""把一張動態 GIF 轉成遊戲用的水平 sprite strip（橫向連續幀）PNG。

引擎執行期並不直接播放 .gif；每個動態裝飾（第二章羅馬廣場的 chiikawa
「雕像」、第三章校慶操場的貓）都是一張把全部 N 幀由左到右排在同一列的
單張 PNG：

    +--------+--------+--------+ ... +--------+
    | frame0 | frame1 | frame2 |     | frameN-1|
    +--------+--------+--------+ ... +--------+
      <- frame_w ->                   total = frame_w * N  x  frame_h

遊戲把這張 PNG 當成「一張」材質載入，再以 DrawTexturePro 逐幀切出來源
矩形（見 include/gfx/SpriteStrip.h 的 StripSourceRect），並讓幀索引來回
彈跳，做出放大縮小／呼吸般的效果（FrameAt）。所有幀視為等寬——每一幀都
貼進各自等寬的格子，並保留透明度（RGBA）。

幀數約定：
引擎是從程式碼中的常數（include/gfx/Decorations.h 的 DecorationDef.frameCount）
讀取幀數 N，而非從檔名或附屬檔讀取；因此輸出 PNG 使用引擎預期的固定檔名，
本腳本也會把寫出的幀數印出來，方便你核對／更新該常數。預設輸出路徑為
resources/assets/decorations/chiikawa_strip.png 與 cat_strip.png（預設
frameCount 皆為 8）。若 GIF 的實際幀數與預設不同，請把 Decorations.h 內該
常數改成本腳本回報的數字即可。

這些裝飾屬於第三方同人創作，已在 CREDITS.md 標註；輸出的二進位檔由使用者
自行管理，不得提交進版本庫。

用法：
    python3 tools/gif_to_strip.py <input.gif> <output_strip.png> [--max-frames N]

    # 本遊戲內建掛鉤的兩個裝飾：
    python3 tools/gif_to_strip.py chiikawa.gif \\
        resources/assets/decorations/chiikawa_strip.png
    python3 tools/gif_to_strip.py 511211cat.gif \\
        resources/assets/decorations/cat_strip.png

    # 限制幀數（例如把很長的 GIF 縮減成 8 幀）：
    python3 tools/gif_to_strip.py cat.gif out.png --max-frames 8

需要 Pillow (PIL)：pip install Pillow
"""
import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageSequence
except ImportError:
    sys.exit("error: Pillow is required — install with: pip install Pillow")


def _key_corner(img: "Image.Image", tol: int) -> "Image.Image":
    """把純色背景去成透明。

    以左上角像素作為背景色，將與其曼哈頓 RGB 距離在 `tol` 以內的每個
    像素 alpha 歸零（純 PIL，不用 numpy）。對於背景為單一平塗色的 GIF
    （例如 chiikawa 的天藍底）能把主體從底框上摳出來；若 GIF 本身已帶
    alpha 則應略過此步驟。

    參數：
        img：來源 RGBA Image。
        tol：判定為背景的曼哈頓 RGB 容差。
    回傳：
        背景已透明化的同一張 Image。
    """
    px = list(img.getdata())
    r0, g0, b0 = px[0][0], px[0][1], px[0][2]
    img.putdata([(r, g, b, 0)
                 if abs(r - r0) + abs(g - g0) + abs(b - b0) <= tol
                 else (r, g, b, a) for (r, g, b, a) in px])
    return img


def gif_to_strip(src: Path, dst: Path, max_frames: int | None = None,
                 height: int | None = None, key_bg: bool = False,
                 key_tol: int = 60) -> int:
    """讀取 `src` GIF 的每一幀，輸出水平 strip PNG（RGBA）到 `dst`。

    每幀都會先合成到 GIF 畫布上（局部更新／disposal 幀交由 Pillow 處理）、
    轉成 RGBA，再貼進各自等寬的格子。格子大小取 GIF 畫布尺寸，因此每幀
    都落在同一位置，播放時不會抖動。

    參數：
        src：來源動態 .gif 的路徑。
        dst：輸出 strip .png 的路徑。
        max_frames：限制幀數；以「整段 GIF 均勻取樣」（而非只取開頭）方式
            縮減，讓裁短後的 strip 仍涵蓋完整動作。None 表示不限制。
        height：把每幀等比例縮放到此像素高度；對於很長／高解析度、整張
            strip 寬度會超過 GL 最大材質寬（約 16384）而載入失敗的 GIF 來說
            不可或缺。None 或非正值表示不縮放。
        key_bg：是否在縮放前把純色背景去成透明（GIF 已帶 alpha 時應為 False）。
        key_tol：key_bg 的曼哈頓 RGB 容差。
    回傳：
        實際寫出的幀數。
    """
    with Image.open(src) as im:
        # 合幀：ImageSequence 搭配 convert('RGBA') 會請 Pillow 完整算繪每一幀
        # （套用 GIF disposal），這樣即使某幀只存了變動區域也會還原成完整影像
        # ——否則貼出來的格子會出現破洞。
        all_frames = [f.convert("RGBA") for f in ImageSequence.Iterator(im)]
        if not all_frames:
            sys.exit(f"error: no frames decoded from {src}")

        # 均勻取樣縮減到 max_frames（保留完整動作，而非只取開頭）。
        if max_frames is not None and max_frames < len(all_frames):
            total = len(all_frames)
            if max_frames == 1:
                idxs = [0]
            else:
                idxs = [round(i * (total - 1) / (max_frames - 1))
                        for i in range(max_frames)]
            frames = [all_frames[j] for j in idxs]
        else:
            frames = all_frames

        # 選用的背景去色（在縮放「之前」做，平塗的內部才能乾淨去掉）。
        # 若 GIF 本身已帶 alpha（例如那隻貓）則略過。
        if key_bg:
            frames = [_key_corner(f, key_tol) for f in frames]

        # 選用的等比例縮放到目標高度。GIF 各幀都是畫布尺寸（一致），縮放後
        # 寬度仍維持相等。
        if height is not None and height > 0:
            frames = [f.resize(
                (max(1, round(f.width * height / f.height)), height),
                Image.LANCZOS) for f in frames]

        # 以 GIF 畫布尺寸建立等寬格子。Pillow 會把每張合幀後的影像設成畫布
        # 尺寸，但仍取最大值保險，避免偶發的異常幀被裁切。
        frame_w = max(f.width for f in frames)
        frame_h = max(f.height for f in frames)
        n = len(frames)

        strip = Image.new("RGBA", (frame_w * n, frame_h), (0, 0, 0, 0))
        for i, f in enumerate(frames):
            strip.paste(f, (i * frame_w, 0))

        dst.parent.mkdir(parents=True, exist_ok=True)
        strip.save(dst, "PNG")
        print(
            f"wrote {dst}  ({n} frames, each {frame_w}x{frame_h}, "
            f"strip {frame_w * n}x{frame_h})"
        )
        print(
            f"  -> set DecorationDef.frameCount = {n} in "
            f"include/gfx/Decorations.h if it differs from the default"
        )
        return n


def main() -> None:
    """CLI 進入點：解析參數、驗證輸入後呼叫 gif_to_strip 產生 strip PNG。

    無參數、無回傳值。負責檢查輸入檔存在、--max-frames／--height 至少為 1，
    再把各旗標轉交給 gif_to_strip。
    """
    ap = argparse.ArgumentParser(
        description="Convert an animated GIF to a horizontal sprite-strip PNG.")
    ap.add_argument("input", type=Path, help="source animated .gif")
    ap.add_argument("output", type=Path, help="destination strip .png")
    ap.add_argument("--max-frames", type=int, default=None,
                    help="cap the frame count (even-sampled across the GIF)")
    ap.add_argument("--height", type=int, default=None,
                    help="downscale each frame to this pixel height "
                         "(aspect-preserved); keeps the strip under the GL "
                         "max texture width for long/high-res GIFs")
    ap.add_argument("--key-bg", action="store_true",
                    help="make the flat top-left backdrop colour transparent "
                         "(for a GIF that has a solid background, not alpha)")
    ap.add_argument("--key-tol", type=int, default=60,
                    help="Manhattan RGB tolerance for --key-bg (default 60)")
    args = ap.parse_args()

    if not args.input.is_file():
        sys.exit(f"error: input not found: {args.input}")
    if args.max_frames is not None and args.max_frames < 1:
        sys.exit("error: --max-frames must be >= 1")
    if args.height is not None and args.height < 1:
        sys.exit("error: --height must be >= 1")

    gif_to_strip(args.input, args.output, args.max_frames, args.height,
                 args.key_bg, args.key_tol)


if __name__ == "__main__":
    main()
