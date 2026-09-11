#!/usr/bin/env python3
"""Halve a 1:1 pixel-art sheet so its density matches the 16px world grid.

Usage:
    python tools/downsample_sheet.py <in.png> <out.png>

Why: every sheet in assets/ is authored at 16 art pixels per world cell and
drawn at 2x. The PostApoc Workshop sheet is also a 16px atlas, but its
objects are drawn on 2-4 tiles each (a locker is 2x4), while the game
places them on 1x2 cells - so they were being drawn at ~1x and packed four
times the pixel density of the floor around them. Halving the sheet and
drawing at 2x keeps every object the same size on screen; only the pixel
density changes.

How: a 2:1 box average would blur; nearest sampling drops every other
outline. This does the average, then snaps each result to the nearest
colour already present anywhere on the sheet, so no new colours are
invented and a one-pixel outline survives as a darker mid-tone rather than
a smear of black. (Snapping to the tile's own palette was tried: on tiles
that are mostly outline it turns every edge black.)

Alpha is a majority vote per 2x2 block between three classes - solid,
drop shadow (the sheet's shadows are black at a single partial alpha) and
clear - with ties going to the more solid class. Flattening the shadow to
opaque black was the first version's mistake: it turned the open space
under every table into a slab.

Requires Pillow. Deterministic: the same input always gives the same output.
"""
import sys
from PIL import Image

TILE = 16  # source atlas tile; the sheet must be a whole number of them


def snap(rgb, palette):
    r, g, b = rgb
    return min(palette, key=lambda c: (c[0] - r) ** 2 + (c[1] - g) ** 2 + (c[2] - b) ** 2)


def downsample(src: Image.Image) -> Image.Image:
    src = src.convert("RGBA")
    w, h = src.width // 2, src.height // 2
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px, po = src.load(), out.load()
    palette = sorted({c[:3] for _, c in src.getcolors(1 << 20) if c[3] == 255})

    for y in range(0, src.height, 2):
        for x in range(0, src.width, 2):
            block = [px[x + i, y + j] for j in (0, 1) for i in (0, 1)]
            solid = [c for c in block if c[3] == 255]
            shadow = [c for c in block if 0 < c[3] < 255]
            clear = 4 - len(solid) - len(shadow)
            if len(solid) >= max(len(shadow), clear):
                n = len(solid)
                avg = tuple(sum(c[k] for c in solid) // n for k in range(3))
                po[x // 2, y // 2] = snap(avg, palette) + (255,)
            elif len(shadow) >= clear:
                po[x // 2, y // 2] = shadow[0]
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src = Image.open(sys.argv[1])
    if src.width % (2 * TILE) or src.height % (2 * TILE):
        sys.exit(f"{sys.argv[1]}: {src.size} is not a multiple of {2 * TILE}")
    downsample(src).save(sys.argv[2])
    print(f"{sys.argv[1]} {src.size} -> {sys.argv[2]} ({src.width // 2}x{src.height // 2})")


if __name__ == "__main__":
    main()
