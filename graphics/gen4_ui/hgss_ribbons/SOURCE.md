# Retail HGSS ribbon asset provenance — extraction gate

Source: `pret/pokeheartgold/src/ribbon.c` (`sRibbonInfo` and `GetRibbonAttr`).
Retail source inspected against the upstream repository on 2026-10-10.

## Verified data contract

`sRibbonInfo` associates each ribbon with its Pokémon save-data field,
an **NCGR member ID**, an **NCLR member ID**, name message ID, and description
message ID. The `GetRibbonAttr` function exposes these as
`RIBBONDAT_NCGR` and `RIBBONDAT_NCLR`.

Examples checked directly in retail source:
- Champion Ribbon: NCGR member **72**, NCLR member **0**.
- Cool Ribbon: NCGR member **73**, NCLR member **0**.
- Cool Super Ribbon: NCGR member **74**, NCLR member **0**.
- Beauty Ribbon: NCGR member **73**, NCLR member **1**.
- Legend Ribbon: NCGR member **112**, NCLR member **0**.
- Premier Ribbon: NCGR member **119**, NCLR member **0**.

The exact parent NARC/archive and its runtime presentation layout have
**not yet been verified**. These IDs must not be interpreted as a path
under an arbitrary archive. In particular, this is **not** proof that the
current Emerald `summary_bg.png` has an HGSS ribbon-screen equivalent.

## Live fork references pending authentic replacements

`src/pokenav_ribbons_summary.c` still loads:
- `gPokenavRibbonsSummaryBg_Gfx`
- `gPokenavRibbonsSummaryBg_Tilemap`
- `gPokenavRibbonsSummaryBg_Pal`
- `graphics/pokenav/ribbons/icons.png` and `icons_big.png`
- `graphics/pokenav/ribbons/icons1.pal` through `icons5.pal`

Do not remove these functional resources before authentic HGSS equivalents
are extracted, provenance-checked, converted as needed, and wired live.
Do not generate synthetic ribbon icons, borders, or background substitutes.

## Next gate

Identify the specific retail HGSS graphics archive referenced by the
ribbon-summary screen, extract its NCGR/NCLR and screen geometry,
verify the binary source provenance, then import assets before altering
the existing functional ribbon presentation.


## Retail ribbon sprite consumer verified (2026-10-10)

The HeartGold decomp's `asm/unk_0208B1AC.s`, function `sub_0208C250`,
calls `GetRibbonAttr(ribbon, 1)` for the NCGR member and passes that
member to `SpriteSystem_ReplaceCharResObj`. It separately calls
`GetRibbonAttr(ribbon, 2)` for the NCLR member and passes the result
to `thunk_Sprite_SetPaletteOverride`.

This verifies the **runtime use** of the ribbon NCGR/NCLR IDs and the
sprite-resource replacement approach—not the containing archive.
The immediate value `0x27` in the call is a sprite-resource argument;
do not assume it identifies a NARC. The parent sprite resource loader
must be traced before extracting members 72–119.

Source inspected: `pret/pokeheartgold/asm/unk_0208B1AC.s`,
`sub_0208C250`. Existing Emerald ribbon graphics remain untouched.


## Sprite resource database cross-check (2026-10-10)

The same retail ribbon consumer `asm/unk_0208B1AC.s` embeds a
resource-database list at `_02103A2C`:
`NARC_resdat_resdat_00000054` (character),
`NARC_resdat_resdat_00000055` (palette),
`NARC_resdat_resdat_00000053` (cell), and
`NARC_resdat_resdat_00000052` (animation).

Their corresponding files are
`files/data/resdat/resdat_00000054.json` (kind `char`),
`resdat_00000055.json` (kind `pltt`),
`resdat_00000053.json` (kind `cell`), and
`resdat_00000052.json` (kind `anim`).
These are **resource-description tables containing mappings to multiple
different graphics archives**, not a verified single parent ribbon NARC.
For example, resdat character records reference both `NARC_a_0_3_9`
and `NARC_a_1_6_2`. A resource ID, character member ID, and NARC file
index must not be conflated.

This narrows the archival tracing target to the ribbon scene's resource
registration/override path; it does not yet authorize asset extraction.
