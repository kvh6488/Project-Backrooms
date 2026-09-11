#!/usr/bin/env python3
"""Snap every pixel of a PNG to the master palette, or check that it already is.

Usage:
    python tools/quantize.py <in.png> <out.png> [--palette assets/palette.json]
    python tools/quantize.py --check <png|dir> ... [--palette assets/palette.json]

Why: the palette in assets/palette.json is only a palette if every sheet uses
nothing else. This is the one tool that enforces it - quantize writes a sheet
that conforms, --check proves that it does (exit 1 and a per-file report of
stray pixels otherwise). Run --check on assets/ as the Phase 6 acceptance test.

How: nearest palette entry by Euclidean distance in OKLab (see
palette_sample.py for why not sRGB). No dithering - pixel art wants flat
fills, and a dither would put noise into the 2x2 texel blocks the grid work
just made uniform. Alpha is preserved unchanged; a pixel with alpha 0 is
written as transparent black so identical sheets compare byte-for-byte.
Colour snapping applies to partially transparent pixels too (drop shadows),
so a shadow's tint is a palette member like everything else.

Requires numpy and Pillow. Deterministic.
"""
import argparse
import json
import os
import sys

import numpy as np
from PIL import Image

from palette_sample import srgb_to_oklab

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load_palette(path):
    with open(path) as f:
        doc = json.load(f)
    rgb = np.array([c["rgb"] for r in doc["ramps"] for c in r["colours"]], dtype=np.uint8)
    return rgb, srgb_to_oklab(rgb)


def quantize(img, pal_rgb, pal_lab):
    px = np.array(img.convert("RGBA"))
    flat = px.reshape(-1, 4)
    cols, inv = np.unique(flat[:, :3], axis=0, return_inverse=True)
    nearest = np.argmin(np.linalg.norm(srgb_to_oklab(cols)[:, None] - pal_lab[None], axis=2), axis=1)
    out = flat.copy()
    out[:, :3] = pal_rgb[nearest][inv.ravel()]
    out[out[:, 3] == 0] = 0
    return Image.fromarray(out.reshape(px.shape), "RGBA")


def check(path, pal_rgb):
    px = np.array(Image.open(path).convert("RGBA")).reshape(-1, 4)
    px = px[px[:, 3] > 0][:, :3]
    members = {tuple(c) for c in pal_rgb.tolist()}
    cols, n = np.unique(px, axis=0, return_counts=True)
    stray = [(tuple(c), int(k)) for c, k in zip(cols.tolist(), n.tolist()) if tuple(c) not in members]
    bad = sum(k for _, k in stray)
    return bad, len(px), stray


def pngs_under(paths):
    for p in paths:
        if os.path.isdir(p):
            for name in sorted(os.listdir(p)):
                if name.lower().endswith(".png"):
                    yield os.path.join(p, name)
        else:
            yield p


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--palette", default=os.path.join(ROOT, "assets", "palette.json"))
    a = ap.parse_args()
    pal_rgb, pal_lab = load_palette(a.palette)

    if a.check:
        failed = False
        for p in pngs_under(a.paths):
            bad, total, stray = check(p, pal_rgb)
            status = "ok  " if bad == 0 else "FAIL"
            print(f"{status} {os.path.relpath(p, ROOT):45s} {total - bad}/{total} in palette"
                  + (f", {len(stray)} stray colours" if stray else ""))
            failed |= bad > 0
        return 1 if failed else 0

    if len(a.paths) != 2:
        ap.error("quantize needs <in.png> <out.png>")
    src, dst = a.paths
    quantize(Image.open(src), pal_rgb, pal_lab).save(dst)
    bad, total, _ = check(dst, pal_rgb)
    print(f"wrote {dst}: {total - bad}/{total} opaque pixels in palette")
    return 0


if __name__ == "__main__":
    sys.exit(main())
