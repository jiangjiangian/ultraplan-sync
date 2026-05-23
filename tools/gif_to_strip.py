#!/usr/bin/env python3
"""Convert an animated GIF into a horizontal sprite-STRIP PNG for the game.

The engine does NOT play .gif files at runtime. Instead each animated
decoration (the Ch2 羅馬廣場 chiikawa "statue", the Ch3 校慶 操場 cat) is a
single PNG holding all N frames laid out left-to-right in one row:

    +--------+--------+--------+ ... +--------+
    | frame0 | frame1 | frame2 |     | frameN-1|
    +--------+--------+--------+ ... +--------+
      <- frame_w ->                   total = frame_w * N  x  frame_h

The game loads that PNG as ONE texture and animates it by slicing a
per-frame source rectangle with DrawTexturePro (see include/gfx/SpriteStrip.h:
StripSourceRect), ping-ponging the frame index for a 放大縮小 / breathing
look (FrameAt). Frames are assumed equal width — every GIF frame is pasted
into its own equal-width cell, transparency preserved (RGBA).

FRAME-COUNT CONVENTION
----------------------
The engine reads the frame count N from a CONSTANT in code
(include/gfx/Decorations.h -> DecorationDef.frameCount), NOT from the
filename or a sidecar file. This is the simplest robust convention: there
is no filename to parse (a rename can't break it) and no sidecar .txt that
can go missing on a fresh clone. The output PNG therefore uses a FIXED
name the engine expects, and this script PRINTS the frame count it wrote so
you can confirm / update the constant if your GIF has a different N.

  default engine paths (drop the converted strips here):
    resources/assets/decorations/chiikawa_strip.png   (frameCount 8)
    resources/assets/decorations/cat_strip.png        (frameCount 8)

If your GIF's frame count differs from the default in Decorations.h, edit
that one constant to match what this script reports. (Equal-width frames is
the only layout the slicer assumes; frame WIDTH/HEIGHT are derived by the
engine from the texture size / frameCount, so any resolution is fine.)

These decorations are third-party fan art credited in CREDITS.md; the
output binaries are user-managed and MUST NOT be committed to the repo
(CLAUDE.md §5 — no new committed binaries under resources/).

USAGE
-----
    python3 tools/gif_to_strip.py <input.gif> <output_strip.png> [--max-frames N]

    # the two decorations this game ships hooks for:
    python3 tools/gif_to_strip.py chiikawa.gif \\
        resources/assets/decorations/chiikawa_strip.png
    python3 tools/gif_to_strip.py 511211cat.gif \\
        resources/assets/decorations/cat_strip.png

    # cap the frame count (e.g. a long GIF you want trimmed to 8 frames):
    python3 tools/gif_to_strip.py cat.gif out.png --max-frames 8

Requires Pillow (PIL):  pip install Pillow
"""
import argparse
import sys
from pathlib import Path

try:
    from PIL import Image, ImageSequence
except ImportError:
    sys.exit("error: Pillow is required — install with: pip install Pillow")


def gif_to_strip(src: Path, dst: Path, max_frames: int | None = None) -> int:
    """Read every frame of the GIF at `src` and write a horizontal strip
    PNG (RGBA) to `dst`. Returns the number of frames written.

    Each frame is composited onto the GIF canvas (so partial/disposal
    frames are handled by Pillow), converted to RGBA, and pasted into its
    own equal-width cell. The cell size is the GIF's canvas size, so every
    frame lands at the same position and the strip animates without jitter.
    """
    with Image.open(src) as im:
        # Coalesce frames: ImageSequence + convert('RGBA') asks Pillow to
        # render each frame fully (it applies GIF frame disposal), so a
        # frame that only stored a changed region still becomes a complete
        # image — otherwise pasted cells would show holes.
        frames = []
        for frame in ImageSequence.Iterator(im):
            frames.append(frame.convert("RGBA"))
            if max_frames is not None and len(frames) >= max_frames:
                break

        if not frames:
            sys.exit(f"error: no frames decoded from {src}")

        # Equal-width cells sized to the GIF canvas. Pillow gives every
        # coalesced frame the canvas size, but guard anyway by taking the
        # max so a stray odd frame can't clip.
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
    ap = argparse.ArgumentParser(
        description="Convert an animated GIF to a horizontal sprite-strip PNG.")
    ap.add_argument("input", type=Path, help="source animated .gif")
    ap.add_argument("output", type=Path, help="destination strip .png")
    ap.add_argument("--max-frames", type=int, default=None,
                    help="cap the number of frames written (default: all)")
    args = ap.parse_args()

    if not args.input.is_file():
        sys.exit(f"error: input not found: {args.input}")
    if args.max_frames is not None and args.max_frames < 1:
        sys.exit("error: --max-frames must be >= 1")

    gif_to_strip(args.input, args.output, args.max_frames)


if __name__ == "__main__":
    main()
