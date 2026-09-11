# Master palette

**Status:** approved 2026-09-11 (`tools/palette_sample.py --config yellow12`).
Every PNG in `assets/` is quantized to it; `python tools/quantize.py --check assets/`
is the acceptance test and must stay at 100 %.

## Working with it

- **The palette is [palette.json](palette.json).** The table below is a copy for
  reading; the JSON is what `quantize.py` reads.
- **Adding a sheet:** `python tools/quantize.py <src.png> assets/<name>.png`, then
  reconfigure cmake. Sources stay outside `assets/` (the packs, or a generated
  file) so a later re-quantize starts from the original, not from a
  quantized copy.
- **Hand-editing a sheet:** paint with palette colours only, then `--check`.
  A stray colour is a build defect, not a style choice.
- **Changing the palette** — a new hex, a moved step: edit `palette.json`, then
  re-quantize every sheet from its source and reconfigure. The sources are
  listed in the "Provenance" section below. To *re-derive* the palette from
  new material (a new pack): add it to `SOURCES` in `palette_sample.py`,
  re-run, and compare the swatch against this one before adopting it — the
  sampler proposes, it does not decide. Adding an entry is cheap; renumbering
  a ramp is not once the §16.6 lighting depends on step indices.
- **Generated art** (`/generate-asset`, Phase 7): PixelLab takes a forced
  palette, then the result goes through `quantize.py` anyway so the check
  cannot be argued with.

### Provenance of the shipping sheets

| Sheet | Source |
|---|---|
| `BCKRMlv1_*_set.png` | `../Asset packs/Backrooms Lvl1 {Tileset}/` |
| `PostApoc_Workshop_16px.png` | `tools/downsample_sheet.py` over `../Asset packs/PostApoc_Workshop/PostApoc_Workshop_WithShadow.png` |
| `PostApoc_Workshop_Icons.png` | `../Asset packs/PostApoc_Workshop/` |
| `Spritesheet_TheDarkRitual_BigWander.png` | `../Asset packs/BigWander_TheRitual/` |
| `guard_yellow_spritesheet.png`, `inspector_spritesheet.png` | `../Asset packs/sci-fi-facility-asset-pack/` |
| `mushrooms_pixel_asset.png` | PixelLab pass over the `mushrooms` pack (Phase 5); the pre-quantize file is the git version at `d82ab87` |
| `workshop_prop_icons.png` | cut from the workshop furniture sheet (Phase 5); pre-quantize at `d82ab87` |

Evidence from the gate:

- `artifacts/palette_swatch.png` — the ramps, with the per-world windows bracketed.
- `artifacts/palette_preview/side_by_side.png` — three scenes rendered through
  the current sheets and through sheets quantized to each candidate palette.
- `artifacts/palette_preview/carpet_zoom.png` — 4× of the carpet and a table,
  which is where the candidates differ most.

## The palette (56 colours, 7 ramps)

Every hex is a colour that already exists on a sheet — the sampler never
invents one. Steps run dark → light, step 0 first.

| Ramp | Steps | Hexes |
|---|---|---|
| neutral | 8 | `#141517` `#2e2e2e` `#464646` `#626262` `#7e7e7e` `#9c9c9c` `#bdbdbd` `#d9d9da` |
| grey | 8 | `#1d1c27` `#383847` `#474858` `#576477` `#717d8f` `#829da5` `#a6b7c6` `#bdd5de` |
| brown | 8 | `#2f190b` `#4a2a19` `#663b27` `#7d4c2e` `#936340` `#b17a4e` `#cc9770` `#f4a568` |
| yellow | 12 | `#382e16` `#483a20` `#544527` `#665932` `#74653c` `#87784a` `#96894e` `#a79757` `#b4ac6a` `#ccbe68` `#e8d282` `#f6e998` |
| green | 8 | `#0f390f` `#164e19` `#3c6723` `#3d7f41` `#76963c` `#69a754` `#91ca51` `#b7f074` |
| blue | 6 | `#152d5c` `#2f4876` `#3b6590` `#4e91af` `#6ea7c6` `#91d6e8` |
| accent | 6 | `#6b2643` `#063ee6` `#b02a2a` `#f43636` `#cc99ff` `#95da41` |

Machine-readable copy: [palette.json](palette.json) (OKLab L/C/h per entry).

### Per-world windows

Steps a world may draw, inclusive, 0-based, on every ramp except `accent`
(the top quarter is overworld-only, the bottom quarter underworld-only; `palette.json` carries them per ramp):

| World | Steps | Gives up |
|---|---|---|
| overworld | 2–7 (of 8) / 3–11 (of 12) / 2–5 (of 6) | the two deepest shadows: no true black on the surface, night sells itself by hue and light radius |
| underworld | 0–5 (of 8) / 0–8 (of 12) / 0–3 (of 6) | the brightest highlights: a torch cannot blow out to white, "bright" is contrast against a compressed neighbourhood |

Accents are shared. Nothing enforces the windows yet — that is the §16.6
lighting work; the palette just has to make it possible.

## How it was built

1. **Sample.** Every PNG in `assets/` and in seven packs (`Backrooms Lvl1`,
   `Backrooms Items`, `PostApoc_Workshop`, `BigWander_TheRitual`,
   `sci-fi-facility`, `Mobs`, `Pixel Crawler`), pixels with alpha ≥ 128.
   `assets/` carries 50 % of the weight; the packs split the rest evenly, so
   Pixel Crawler's 2.6 M pixels cannot outvote the 115 k that ship. Promo
   renders (thousands of colours) are skipped. 2 984 distinct colours in.
2. **OKLab.** Distances and "evenly spaced" are measured there, not in sRGB,
   because sRGB distance does not match what the eye sees.
3. **Families** by chroma and hue: neutral (C < 0.02), grey (cool tint,
   C < 0.06), then hue bands brown 20–75°, yellow 75–120°, green 120–185°,
   blue 185–290°. Anything with C ≥ 0.17, or in the red/magenta gap, is an
   accent.
4. **Ramps** are straight lines through OKLab, fitted per family by weighted
   least squares of (a, b) on L, sampled at equal L steps, each sample snapped
   to the nearest real sheet colour. Accents are a weighted k-means (fixed
   seed), medoid-snapped.

## What the sampler found

**Yellow is the game.** In `assets/`, hue 75–120° (the wallpaper and carpet
ochres) is a third of every chromatic pixel; green and blue together are under
5 %. The plan's table had no yellow ramp — it folded ochre into brown. Tried
that (`--config plan`, 46 colours): the brown line then runs from red-brown to
ochre and is right for neither, and the carpet quantizes brown. The proposal
splits yellow out and gives blue 6 steps instead of 8 to pay for it.

**8 steps flatten the carpet.** The Backrooms floor texture is built from
shades 0.02–0.04 L apart (`#6e5d3a` `#796a3e` `#817042` `#87784a` `#8a7e4b`).
An 8-step ramp across L 0.31–0.89 is 0.08 per step, so those five collapse to
one and the weave disappears — `carpet_zoom.png`, columns 2 and 3. This is the
"quantization can flatten intentional gradients" risk from the plan, and it
is real on the single most visible tile in the game. 12 yellow steps (0.05
each) keep the weave and most of the table grain. Hence 56, not 46.

**Quantization error, `assets/` only** (mean OKLab distance to nearest palette
entry; 0.02 ≈ just noticeable):

| Config | Colours | Mean ΔE | Pixels > 0.05 | Pixels > 0.10 |
|---|---|---|---|---|
| plan | 46 | 0.041 | 23 % | 12 % |
| yellow | 52 | 0.046 | 20 % | 12 % |
| yellow12 | 56 | 0.045 | 18 % | 12 % |

The 12 % over 0.10 is the same in all three: it is the workshop sheet's
saturated oranges and the sci-fi guards' uniforms, which no 50-colour palette
built from ochre and grey will hold. They will shift; that is the point.

**Accents are the data's, not the plan's.** The k-means picked maroon, pure
blue, dark red, red, lavender, lime. The plan asked for fire, radiation, UI
alert, item highlight. Lime (`#95da41`) is the radiation green; red is the
alert; there is no fire orange and the blue/lavender come from the elevator
set and the ritual sheet. Hand-picking the six accents is reasonable — they
are semantic, and six colours is not a sampling problem.

**Telemetry is unchanged** across all preview runs (12/12 checkpoint files
byte-identical to the current build), as a presentation-only change must be.

## Gate outcome

Approved as proposed after a hand-play with the quantized sheets swapped into
`build/assets/`: 7 ramps / 56 colours, sampled accents kept. Headroom to 64 is
reserved for a specific asset that shows it needs a step (the table grain is
the first candidate), not spent up front.
