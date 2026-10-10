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


## Ribbon scene resource ID 39 traced to its initial archive (2026-10-10)

Retail `asm/unk_0208B1AC.s` function `sub_0208B1AC` initializes its
sprite system with the resource database set `_02103A2C`, then calls
`sub_0200D294` to register those resources. Later,
`sub_0208C250` passes **resource ID 0x27 (39)** to
`SpriteSystem_ReplaceCharResObj`, with the selected ribbon's NCGR
number obtained from `GetRibbonAttr(ribbon, RIBBONDAT_NCGR)`.

Crucially, `pret/pokeheartgold/files/data/resdat/resdat_00000054.json`
maps character-resource IDs **39, 40, 41, and 42** to
**`NARC_a_1_6_2`, file 76** as the *initial* character resource.
This provides a concrete archive candidate for the ribbon graphics:
`NARC_a_1_6_2` (filesystem archive `a/1/6/2`).

**Remaining verification:** the replace function's archive-selection
semantics must be confirmed before equating the initial member 76's
archive with the replacement NCGR members 72–119. The corresponding
palette source and retail ribbon-summary background are also not yet
verified. Do not import arbitrary members or replace live assets yet.


## Confirmed retail ribbon-icon archive — correction (2026-10-10)

**The live ribbon replacement archive is `NARC_a_0_3_9` (NARC ID
`0x27` = 39, filesystem `a/0/3/9`), NOT `NARC_a_1_6_2`.**
The previous section's `a/1/6/2` mapping applies only to the
*initial sprite-resource record* in `resdat_00000054.json`.
It must not be used to extract ribbon icons.

This is confirmed by the function signature in
`pret/pokeheartgold/src/sprite_system.c`:

`SpriteSystem_ReplaceCharResObj(spriteSystem, spriteManager, narcId,
fileId, compressed, resId)`

At `sub_0208C250` in `asm/unk_0208B1AC.s`, the ARM calling
convention binds `r2 = 0x27` to **narcId**, `r3 = GetRibbonAttr(ribbon,
RIBBONDAT_NCGR)` to **fileId**, stack argument 1 to `compressed = 0`,
and stack argument 2 to **resId = argument3 + 0x19**. Therefore
ribbon character members 72–119 are loaded **uncompressed** from
`NARC_a_0_3_9` by this routine.

The palette value from `GetRibbonAttr(ribbon, RIBBONDAT_NCLR)`
is used as a **palette override index (value + 7)**; this routine does
not load a new NCLR per ribbon. The corresponding loaded palette
resource must be traced separately before mapping NCLR IDs to
archive files.

**Extraction gate reached for NCGR source identification only.**
No BG tilemap or palette source has yet been proven by this call.


## Retail ROM extraction gate completed (2026-10-10)

A user-provided USA HeartGold NDS was parsed via the NitroFS file table.
File `a/0/3/9` is a valid NARC containing **137 members**.
Members **72–119** (48 files) were extracted without changing bytes.
Their Nintendo DS character graphics signatures begin with `RGCN`.
An independently packaged extraction contains all 48 originals and a
JSON manifest recording file indices, sizes, and SHA-256 checksums.

This confirms the original ribbon-character archive and member range
against an actual retail ROM, in addition to the source-code loader trace.
The source archive and PNG conversion are not yet committed to the
repository. **Do not wire these NCGRs to the live GBA interface until
the retail palette-source mapping, conversion constraints, and ribbon
layout are verified.** The initial character-resource archive `a/1/6/2`
is distinct from the ribbon replacement archive `a/0/3/9`.
