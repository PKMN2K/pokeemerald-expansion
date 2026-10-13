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


## Retail archive palette inventory (2026-10-11)

Direct inspection of the uploaded USA HeartGold ROM's
`a/0/3/9` archive confirms it contains **137 members**, including
**nine raw NCLR (`RLCN` magic) palette files** at member indices:
`1, 14, 37, 38, 39, 40, 65, 69, 136`.

All nine NCLR member payloads were extracted byte-for-byte to a
separate downloadable verification package with member-number and
SHA-256 manifest. This is an **archive inventory**, not proof that all
nine palettes belong to ribbons or that a particular NCLR is the live
ribbon palette. The precise loaded palette resource and index +7
override mapping remain to be established from the ribbon scene code.

Do not import any of these palette members as the authoritative
ribbon palette until the scene's palette-loader path is verified;
the current functional Emerald ribbon graphics remain unchanged.


## Retail palette resource mapping narrowed (2026-10-11)

The ribbon scene initializes its palette resource collection through
`resdat_00000055.json`, sourced by `_02103A2C` in
`asm/unk_0208B1AC.s`. This database explicitly maps:
- Palette resource **ID 3** -> `NARC_a_0_3_9` member **65**;
- Palette resource **ID 5** -> `NARC_a_0_3_9` member **136**.

These two NCLR members are therefore **confirmed scene-loaded palette
resource candidates**, not merely arbitrary palettes sharing the archive.
The selected ribbon's `RIBBONDAT_NCLR` value is passed to
`thunk_Sprite_SetPaletteOverride` as `value + 7` in
`sub_0208C250`. That is a palette-bank selection, **not a NARC file
index or a separate NCLR load**. Which palette resource supplies the
banks selected by that override requires the scene sprite palette
allocation trace or an original-ROM render check.

`sub_0208B48C` separately calls `SpriteSystem_ReplacePlttResObj`
using NARC ID `0xA2` (162), for a different changing resource; this
must **not** be misclassified as the ribbon palette source.

Keep both 65 and 136 unchanged and unassigned until the active
palette-bank mapping is verified. No synthetic or recolored sprites.


## Runtime ribbon palette-bank calculation verified (2026-10-11)

Retail `asm/unk_0208B1AC.s`, `sub_0208C250`, calls
`GetRibbonAttr(ribbon, RIBBONDAT_NCLR)` then passes `nclrId + 7`
to `thunk_Sprite_SetPaletteOverride` for the corresponding ribbon
sprite. This same function uses a ribbon-dependent NCGR member and
replaces its sprite character resource from `NARC_a_0_3_9`.

Retail `src/sprite.c` implements `Sprite_SetPaletteOverride` by
setting the sprite's `palIndex` and its palette-override flag.
Thus `+7` is the in-memory sprite palette index, **not** a file
number, and must not be read as member `7 + nclrId` of
`a/0/3/9`. The ribbon screen shows at most **nine** ribbon sprites
at a time in the two paths `sub_0208C2A0` and `sub_0208C320`;
those functions call `sub_0208C250` on the displayed ribbon slots.

Remaining verification: map the populated OBJ palette banks used by
indices `7 + nclrId` to the loaded NCLR payload(s) and confirm the
corresponding palettes in the retail ROM. Resource database entries
`a/0/3/9` member 65 and member 136 alone do not establish which
one supplies those banks. Avoid pixel conversion or live replacement
until this mapping is verified.


## Palette loading path and ROM binary structure (2026-10-11)

Retail `sub_0208B1AC` calls `sub_0200D294` with the
`_02103A2C` resdat list. `src/sprite_system.c` defines this
function as a call to `SpriteSystem_LoadResourceDataFromFilepaths`
with character load mode 2 and palette load mode 1. Therefore the
ribbon scene initializes palette resources from the matching PLTT
resdat; it is not loading every NCLR in the graphics NARC.

Direct binary inspection of the extracted original NCLR members **65**
and **136** confirms each has Nintendo DS `RLCN` / `TTLP`
sections, a total size of **552 bytes**, and **512 bytes of palette
payload** (256 15-bit colors, capacity for 16 four-bit palette banks).
That is enough capacity for the runtime ribbon `NCLR attribute + 7`
index but does **not** establish the OBJ palette allocation offsets
or which of the two is used. Do not treat this as proof of a ribbon
palette file-to-bank assignment.

Next verification requires tracing `SpriteSystem_LoadResourceDataFromFilepaths`
through palette VRAM transfer and the ribbon sprite resource header,
or validating a controlled retail render. Do not replace the live
Emerald palette on a capacity inference.


## Extended OBJ palette transfer confirmed (2026-10-11)

The retail source `pret/pokeheartgold/src/sprite_system.c` resolves
`sub_0200D294` to `SpriteSystem_LoadResourceDataFromFilepaths(..., 2, 1)`.
Its actual `loadPlttMode == 1` branch invokes
`SpriteTransfer_CreateAllExtPlttTransferTasks`, rather than
`SpriteTransfer_CreateAllPlttTransferTasks` (mode 0).
The ribbon scene therefore registers its PLTT resdat graphics in the
**extended OBJ palette transfer path**. This is an important portability
constraint: the original DS 16-bank palette data and palette-index
override cannot simply be transplanted into GBA OBJ palette memory,
which lacks equivalent extended OBJ palette behavior.

The loader obtains the `resdat` archives via `NARC_data_resdat`,
initializes sprite resource objects with
`LoadAll2DGfxResObjsFromHeader`, and finally creates the sprite
resource headers. Existing source data identifies PLTT resources 3
(member 65) and 5 (member 136), but does not yet tie the ribbon
sprite's assigned PLTT resource/header to the palette-bank indices.

**Next gate:** inspect the retail resource-header mapping
`resdat_00000085` and resolve which PLTT resource is referenced
by ribbon character resource 39, then determine compatible GBA
palettization without changing the authentic pixels.


## Ribbon sprite header palette assignment confirmed (2026-10-11)

Retail `pret/pokeheartgold/files/data/resdat/resdat_00000085.json`
contains the ribbon scene's sprite resource headers. The entries with
character graphics IDs **38, 39, 40, 41, 42, and 43** each select:
`palette: 8`, `cell: 4`, `anim: 4`, and priority 3.
In particular, the ribbon sprite's **graphics resource 39** uses
**palette resource ID 8**, not palette resource 3 or 5.

The palette resource database `resdat_00000055.json` maps ID **8**
to `NARC_poketool_icongra_poke_icon`, file **0**, uncompressed.
Thus the sprite resource header links ribbon sprites to the
**Pokémon icon graphics archive palette resource**, while
`sub_0208C250` overrides the displayed sprite palette index with
`GetRibbonAttr(ribbon, RIBBONDAT_NCLR) + 7`.

**Correction to prior candidates:** `a/0/3/9` NCLR members 65 and
136 are scene-loaded palette resources 3 and 5, but they are **not**
the palette resource referenced by the ribbon sprite's resdat header.
Do not assume they are the final ribbon palettes.

Remaining verification: determine how palette indices 7+ map to
loaded OBJ extended palettes, and examine the actual colours in
`poketool/icongra/poke_icon` file 0. Verify whether the ribbon
override intentionally selects palette slots outside the linked
resource before deriving GBA-compatible ribbon palettes.


## Sprite-resource palette transfer implementation (2026-10-11)

Retail `src/sprite_transfer.c` confirms that the palette-loader branch
calls `SpriteTransfer_CreateAllExtPlttTransferTasks` for every populated
PLTT resource. Each resource supplies its own `plttData`,
`vram`, `resId`, and `plttNum` to
`ObjPlttTransfer_CreateTaskAndDoTransferFromTemplate_HandleExtPltt`.
The conversion must not assume the ribbon palette indices correspond
directly to palette-file indices or contiguous banks in arbitrary
NCLR members.

The sprite header `resdat_00000085` ties graphics resources 38-43,
including the ribbon's replacement resource 39, to PLTT resource
**8**. `resdat_00000055` identifies resource 8 as
`NARC_poketool_icongra_poke_icon`, member **0**.
This is the explicit header-linked palette to inspect and export next.
It is not enough by itself to conclude that member 0 contains all
ribbon colours: the override index and resource transfer allocation
must be tested against the actual NCLR bytes.

No synthetic palette or artwork has been generated.


## Header-linked icon palette extracted and inspected (2026-10-11)

Retail `NARC_poketool_icongra_poke_icon` member **0**, mapped from
ribbon sprite PLTT resource **8** by `resdat_00000055.json`, was
extracted directly from the supplied HeartGold ROM as an unchanged
552-byte `RLCN`/`TTLP` NCLR resource.

SHA-256 of original member 0:
`96d45b21044626a1975b92065a9a40115975f4bb53d2a64301dc747553549a84`.

Its TTLP section contains 512 bytes / 256 little-endian RGB555
entries, naturally divisible into sixteen 16-colour palettes. First
bank begins with RGB555 values `1e69, 3a10, 5ef7, 7fff`.
No colours were synthesized, recoloured, or palette-quantized.
A separate local extraction archive contains the original NCLR.

This proves the **binary palette asset linked by the retail ribbon
sprite resource header**. However, verifying *which of its 16 banks*
is physically addressed by the runtime `nclrId + 7` override still
requires tracing the palette transfer/proxy offset or checking a
retail render. A header link alone does not prove all ribbon sprite
pixels display using this NCLR's bank `7 + nclrId` without an
offset. Live Emerald ribbon assets remain unchanged.


## Direct retail palette extraction and important bank mismatch (2026-10-11)

The USA HeartGold ROM NitroFS path `pbr/poke_icon.narc`
contains a NARC of **540 members**. Its member **0** was extracted
byte-for-byte; this matches the `poke_icon` resource named by PLTT
resdat resource **8** in the ribbon sprite header.

Extracted member 0 is a **552-byte NCLR** with `RLCN` /
`TTLP` sections and **256 16-bit colour entries**. A SHA-256
manifest accompanies the preserved original binary in the extraction
package `HGSS_ribbon_header_palette_resource8.zip`.

**Crucial finding:** after parsing its 40-byte NCLR header, the
sixteen 16-colour banks have populated nonzero colour values only in
banks **0–2**. Banks **3–15** are entirely zero. Therefore palette
override values `7 + RIBBONDAT_NCLR` **cannot be naively indexed**
into these member-0 banks to obtain the ribbon colours.

Cross-check of `a/0/3/9` NCLR members 65 and 136 likewise finds
nonzero colours only in banks 0 and 0–4 respectively. This supports
the distinction between the sprite header's resource reference and
the ultimate **allocated/combined extended OBJ palette index**.
The override must be resolved through resource allocation and
potentially other palette loads rather than simple direct member
bank lookup.

Do not derive a GBA ribbon palette from empty banks or pretend
the final ribbon colours are confirmed. NCGR character extraction
remains verified; live Emerald art remains unchanged.


## Extended palette bank allocation algorithm verified (2026-10-11)

Retail `src/obj_pltt_transfer.c` establishes **sequential,
32-byte-per-bank palette allocation**, tracked independently for
main and sub screens. `ObjPlttTransfer_InitFromTemplate` reads
`template->plttNum` as each resource's bank count.
`ObjPlttTransfer_ReserveAndTransfer_HandleExtPltt` chooses
`extPosMain` or `extPosSub` for extended palettes, and
`ObjPlttTransferTask_ReserveBlocks` increments the selected
position by `plttSize * 32`. The standard-palette path uses
`posMain`/`posSub` separately, so one must distinguish
extended and standard resources when computing bank numbers.

**Consequence:** The ribbon's runtime palette override index
`7 + RIBBONDAT_NCLR` can refer to the global allocated sprite
palette slots, not banks numbered from zero inside its *own*
NCLR file. The empty banks 7+ in `poke_icon.narc` member 0
therefore do not prove that the source ribbon colours are missing.
To assign exact colours, reconstruct the ordering, per-resource
`plttNum`, extended palette flags, and main/sub VRAM destination
for the palette objects in `resdat_00000055` during scene
initialization, then compare against retail render output.
Do **not** assume a particular palette resource solely from the
sprite header or bank index.

Original sources:
`pret/pokeheartgold/src/obj_pltt_transfer.c`:
`ObjPlttTransfer_InitFromTemplate`,
`ObjPlttTransfer_ReserveAndTransfer_HandleExtPltt`,
`ObjPlttTransferTask_ReserveBlocks`.


## Palette resource order yields a testable ribbon-bank hypothesis (2026-10-11)

`resdat_00000055.json` provides the following ordered entries;
the two `extra` values are the resource's VRAM selector and palette
count used by the sprite resource loader:

| Resource | VRAM selector | Palette count | Archive / member |
| --- | ---: | ---: | --- |
| 0 | 3 | 3 | a/1/6/2:61 |
| 1 | 2 | 3 | a/0/0/8:74 |
| 3 | 1 | 1 | a/0/3/9:65 |
| 4 | 2 | 1 | graphic/plist/gra:23 |
| 5 | 2 | 5 | a/0/3/9:136 |
| 6 | 1 | 1 | a/1/6/2:49 |
| 7 | 1 | 2 | a/1/6/2:65 |
| 8 | 2 | 3 | poketool/icongra/poke_icon:0 |
| 9 | 2 | 1 | a/1/6/2:3 |

**New hypothesis:** if resources 0, 1, 4, and 5 all use the
*same* sub-screen palette allocation pool, their consecutive
bank ranges are 0–2, 3–5, 6, and **7–11**, respectively.
That aligns exactly with the ribbon sprite override
`RIBBONDAT_NCLR + 7` (0–4 variations) and the **five populated
banks** of NCLR `a/0/3/9` member **136**.
This is strong corroboration that member 136 carries the ribbon
colours, **despite** the ribbon sprite header linking resource 8
for its default palette proxy. A palette override can select
another loaded bank.

**Not yet fully proven:** the allocation routine keeps *separate*
standard and extended palette cursors. Inspect each resource's
`bExtendedPlt` flag and loader classification to establish that
these particular resources indeed share the same pool; check
bank-to-pixel colour matching before enabling replacement.
The NCLR format fields differ (member 136 reports 4; icon member
0 reports 3), so do not silently assume a shared pool.


## Palette count and destination originate in resource metadata (2026-10-11)

Retail `src/unk_0200A090.c` confirms that
`GetResourceExtraPlttData` obtains the unpacked NCLR palette pointer
through `NNS_G2dGetUnpackedPaletteData`, and copies the resource
database's VRAM selector and palette count into the palette resource.
`GF2DGfxResObj_GetPlttNum` returns this stored `pltt_num`, not
a count inferred from NCLR binary length. The sprite transfer loader
then passes these metadata values to the transfer task.

For the proposed ribbon colour palette (`a/0/3/9` member 136,
resource ID 5), `resdat_00000055` declares VRAM selector **2** and
palette count **5**. These are confirmed configuration values for
the retail loader, and the same member contains five populated banks.
The remaining question is whether the NCLR's unpacked
`bExtendedPlt` flag routes this through the extended or standard
allocation cursor. The resource ID order alone is not sufficient
to assert the final absolute palette-bank positions.

This finding narrows conversion to five authentic 16-colour palettes
without claiming that the final runtime bank indices are proven.


## NCLR palette-mode fields checked against retail binaries (2026-10-11)

Byte-for-byte extraction archives were re-inspected. In the
Nintendo DS NCLR/TTLP data header, the 32-bit palette format field
at file offset `0x18` reads **4** for `a/0/3/9` member **136**,
and **3** for member **65** and
`pbr/poke_icon.narc` member **0**. The 32-bit extended palette
flag at file offset `0x1C` is **0 for all three**. Consequently,
the candidate ribbon colours in member 136 are in the **standard**
OBJ palette allocation pool, *not* the extended pool. The loader
named `HandleExtPltt` handles both cases and selects the pool
using `bExtendedPlt` (`src/obj_pltt_transfer.c`).

The earlier note describing the ribbon palette as necessarily
"extended OBJ" should be read as identifying the *transfer
function*, not proof that its NCLR is flagged as an extended palette.

The hypothesis of resource 5 starting at standard sub-screen bank
7 remains contingent on the standard/extended flags and VRAM
destinations for the *preceding* palette resources 0, 1, and 4.
Do not promote that bank offset to confirmed until those binary
resources have been examined. No game artwork or colours changed.


## Confirmed ribbon palette allocation from the retail ROM (2026-10-11)

**Extraction gate cleared for the five standard ribbon palette banks.**
The user-provided USA HeartGold ROM was inspected directly through its
NitroFS FAT/FNT and the corresponding NARC member tables. The palette
resources that precede resource 5 in the retail
`resdat_00000055.json` order were checked by their real NCLR binary
headers, not inferred from resource names:

| PLTT resource | NitroFS archive / member | VRAM selector | Palette count | NCLR format field | bExtendedPlt |
| --- | --- | ---: | ---: | ---: | ---: |
| 0 | `a/1/6/2` / 61 | 3 | 3 | 3 | 0 |
| 1 | `a/0/0/8` / 74 | 2 | 3 | 3 | 0 |
| 4 | `pbr/plist_gra.narc` / 23 | 2 | 1 | 3 | 0 |
| 5 | `a/0/3/9` / 136 | 2 | 5 | 4 | 0 |

The integer flags at NCLR offsets `0x18` (format) and `0x1C`
(extended) were read directly. All four are standard, not extended,
palettes; VRAM selector 3 targets both main and sub, while 2 targets
sub. The retail allocator in `src/obj_pltt_transfer.c` assigns these
resources sequentially to **standard sub-screen** palette memory,
32 bytes per 16-colour bank. The resulting allocation is:

- Resource 0: standard sub OBJ banks 0–2
- Resource 1: standard sub OBJ banks 3–5
- Resource 4: standard sub OBJ bank 6
- **Resource 5: standard sub OBJ banks 7–11**

`sub_0208C250` in `asm/unk_0208B1AC.s` sets the visible ribbon
sprite's palette override to `7 + GetRibbonAttr(ribbon, RIBBONDAT_NCLR)`,
an exact match to those five banks. Thus the original colour source for
the five ribbon variants is **NCLR member 136 in `a/0/3/9`**.
The sprite header's default palette resource 8 does not supersede
the explicit runtime palette override.

The next phase may decode the verified NCGR members 72–119 using
member 136's five original 16-colour banks, preserving the original
pixels and colours. GBA tile/palette limits and in-game presentation
still need validation before deleting the Emerald equivalents.


## Shared Gen 3 ribbon NCGR shapes imported (2026-10-13)

Extracted from the user-supplied USA HeartGold ROM, NitroFS `a/0/3/9`
(NARC file 168). The direct NCGR records are 560 bytes each; the
unaltered 512-byte character-data portion at file offsets 48–559 is
staged as GBA-compatible 4bpp tile data:

| NCGR member | Authentic staged file | Emerald ribbons using its tiles | Raw tiles SHA-256 |
| --- | --- | --- | --- |
| 81 | `marine_land_sky_81.4bpp` | Marine, Land, Sky | `c3a7d55673bc680d66640681b569b8b94fbe97d2434cb3a02b5b6c9bf19ac6a2` |
| 82 | `country_national_82.4bpp` | Country, National | `7be5f114e9e5bd92ea0261e57d51bae87038575d98acdd6d5cbfc2ef5ca9d18e` |
| 83 | `earth_world_83.4bpp` | Earth, World | `ead81584d4e756a094f552522c59659b8c4f1d4f108c0dbcfed2b80bc3f64811` |

Emerald-to-retail ribbon IDs, members and palette-bank mappings are
already recorded in `emerald_ribbon_compat.csv`. The five palette banks
remain unchanged, taken byte-for-byte from member 136 of the same NARC.
The validator now checks retail SHA-256 digests for every staged ribbon
tile set and palette. **These are assets only:** no live renderer is
rewired and no Emerald ribbon art is deleted in this commit.

## Live selected-ribbon graphics (2026-10-13)

The zoom sprite in `src/pokenav_ribbons_summary.c` now loads the 32x32
retail HGSS NCGR pixels from `big_icons_hgss.4bpp`: twelve mechanically
concatenated, unchanged tile sets (NCGR 72–76, 78, 77, 79–83)
in the existing sprite frame order. Size: 0x1800 bytes.
SHA-256: `5b78564602edf75e2ceb4d93327310da286cc131fb9422081f07f002713eca4d`.
Its five sprite palettes load the original NCLR 136 banks unchanged.
The existing zoom animation and 16-tile frame offsets are preserved.

The legacy 16x16 BG grid icons and palette remain live until their
layout can support 32x32 retail pixels, without rescaling or redrawing.
The old `icons_big.png` should only be deleted after the replacement
zoom path has been verified in-game; no legacy chrome is removed here.

## Native 32x32 HGSS ribbon grid (2026-10-13)

The summary BG1 grid now loads the same **unmodified** 0x1800-byte
HGSS ribbon tile sheet and the same five original NCLR 136 palettes
used by the expanded-ribbon sprite. Four columns by two rows fill the
existing 128x64-pixel grid viewport: BG1 tiles x11–26, y4–11.
One blank BG tile is reserved at index zero; the source NCGR tile
blocks start at tile one and have 16 tiles each.

Eight icons are shown per page; the ribbon IDs and virtual gift-ribbon
gap are preserved, with pages based on the selected virtual slot.
D-pad navigation continues across ordinary ribbons and, crucially,
multiple gift-ribbon rows. Page changes redraw BG1 before moving the
authentic HGSS selection corners and zoom sprite. The Pokémon card,
ribbon descriptions, other windows, and all save-data semantics remain
unaltered. Legacy `icons.png` and `icons1.pal`–`icons5.pal`
are no longer referenced by this ribbon summary screen, but are kept
on disk until visual testing of the new pagination completes.

This change resizes **the layout**, not the authentic ribbon artwork.
A successful compilation does not replace in-game visual verification.

## Offline ribbon grid verification (2026-10-13)

`tools/gen4_ui/check_hgss_ribbon_grid.py` checks the **live C source**
against all 32 Emerald-to-retail mappings, the full-size BG1 tile
boundaries, and representative D-pad navigation across **207 nonempty**
normal/gift-ribbon ownership combinations (0–25 normal, 0–7 gift).
The gen4-ui CI job builds five transparent, pixel-exact 128x64 PNG
previews from the same untouched HGSS 4bpp data and five original
NCLR color banks. They are uploaded as
`hgss-ribbon-grid-offline-previews` in each workflow run.

**Important:** These are *offline sprite-layer references*, **not**
mGBA screenshots and not in-game proof. No generated preview is linked
into the ROM. Real emulator testing is still required before deleting
unused legacy icon files or declaring the ribbon migration complete.
