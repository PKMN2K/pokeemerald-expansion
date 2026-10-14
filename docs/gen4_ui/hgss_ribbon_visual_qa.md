# HGSS Ribbon Grid — Emulator Visual Acceptance

**Branch:** `feature/gen4-ui-renderer`
**Scope:** Emerald's PokéNav ribbon summary only. Trainer card is not part
of this UI migration. Use only the already-imported, retail HGSS assets.

## Automated checks already implemented

- The dedicated `gen4-ui` CI job validates the 32 ribbon/retail palette
  combinations and 207 nonempty ribbon-ownership layouts.
- The native 32x32 sheet consists of 12 retail NCGR blocks (16 4bpp tiles
  apiece), with five original HGSS palette banks. No redrawing or scaling.
- The static validator confirms the selected ribbon's BG cell, zoom OBJ,
  and four corner sprites share a common page-relative center.
- `hgss-ribbon-grid-offline-previews` contains five 128x64-pixel PNGs
  generated from retail data. **They are not in-game screenshots.**
- Successful builds and battle tests establish compilation/runtime
  compatibility, **not** correct PokéNav rendering.

## Step A — isolated real-emulator artwork check

1. Build the Emerald target from this feature branch and launch it in
   a GBA emulator such as mGBA, with the project's debug menu enabled.
2. In Debug > Utilities, select **HGSS ribbon grid test**.
3. Confirm the first page shows champion and contest ribbons in their
   original colors, at the expected native pixel size.
4. Press Right repeatedly to inspect pages two through five, including
   all five palette families and the gap before the gift ribbons.
5. Press Left to revisit previous pages; confirm wrapping in both
   directions and no displaced or corrupted tiles. Press B to exit.
6. Capture screenshots showing pages 1, 4 and 5. Compare only the
   **ribbon icon pixels** with the offline preview PNGs, accounting for
   the viewer's transparent background.

This test does **not** exercise the live PokéNav navigation or selected
ribbon zoom and must not be treated as proof of those behaviors.

## Step B — actual PokéNav ribbon summary (required)

Use a test save with ribboned Pokémon; preserve the original save.
Multiple test saves are acceptable. The listed counts refer to
**owned ribbon entries**, not total ribbons in the species data.

| Save fixture | Checks |
|---|---|
| 1 normal ribbon | Single icon at top left; A expands; B shrinks |
| 8 normal ribbons | Exactly two rows; corners/zoom centered on each icon |
| 9 normal ribbons | Down/right can move from page 1 to page 2 |
| 25 normal ribbons | Last normal icon at virtual slot 24, page 4 |
| Gift-only Pokémon | First gift at virtual slot 28; no invalid normal icon |
| 5–7 gift ribbons | Navigation reaches second gift row on page 5 |
| Normal + gifts | Traverses the gap and both sections without losing selection |

For each fixture, enter PokéNav > Ribbons > Pokémon > ribbon summary.
Check ordinary navigation before pressing A, selected-ribbon details
and palette after A, Left/Right/Up/Down while expanded, and B to shrink.
Switch Pokémon to confirm the new icon page resets correctly.
Also check that the larger icons do not cover the ribbon-count text,
Pokémon name, description, other sprites or edge of the display.

**Special boundary:** On page 4 the first normal icon is virtual slot
24 and the gift icons start at slot 28 (row 2). No nonexistent
normal-ribbon icon should appear at slots 25–27.

Capture a screenshot of the full live PokéNav screen, plus a screenshot
of the expanded selected ribbon on the last page. Check the actual
ribbon labels/descriptions and 32x32 selection corners.

## Completion criteria / cleanup gate

Do **not** delete `graphics/pokenav/ribbons/icons.png`, `icons_big.png`,
or the five legacy `icons*.pal` files solely because the compiler or
the offline converter passes. Verify the actual screen first, then
audit all remaining references in the source/graphics build rules.
Only delete files that are truly unreferenced and no longer needed,
followed by another complete CI run.

Record screenshot references and the test save/fixture in the migration
notes before marking this screen visually verified. Never check
saves or proprietary ROM dumps into the repository.

## Headless mGBA verification of live C navigation

The test runner now includes `test/pokenav_ribbons.c`, which calls
`Test_HgssRibbonNavigate` (a `TESTING`-only adapter around the actual
`TrySelectRibbonUp/Down/Left/Right` functions). It exhaustively checks
reachability and valid targets across all **207 nonempty** normal/gift
ownership combinations. A second test verifies the shared live cursor/
zoom center for every grid cell across all five pages.

The runtime cursor and selected-ribbon sprite now obtain their location
from the **same** `GetHgssRibbonScreenPosition` helper, eliminating
independent coordinate calculations. These tests run in a real headless
GBA emulator, but do not capture the full PokéNav screen or evaluate
pixel appearance. Visual acceptance remains mandatory.

## Step C — automated isolated-viewer mGBA screenshots (added 2026-10-14)

The separate [HGSS ribbon emulator screenshots workflow](../../.github/workflows/hgss-ribbon-emulator.yml)
builds a **temporary CI-only Emerald ROM** with its initial callback
repointed to `CB2_HgssRibbonGridTest` by locally patching the linker
script during CI. This change is never committed or used in release ROMs.
A pinned, MIT-licensed headless mGBA screenshot utility captures all five
pages as real 240x160 GBA framebuffer screenshots, and a pixel verifier
compares each opaque icon pixel to the authentic HGSS tile/palette source.
It uploads **screenshots only**, not the temporary ROM.

Artifact: `hgss-ribbon-real-emulator-screenshots`. The reviewer must
inspect the images and CI result before checking Step A. This is stronger
evidence than offline PNGs but **does not verify the full live PokéNav
screen**, the overlay/chrome, zoom, selection corners or all runtime input.
Step B remains mandatory before legacy asset deletion.
