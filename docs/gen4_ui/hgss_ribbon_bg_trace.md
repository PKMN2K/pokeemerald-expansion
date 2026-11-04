# HGSS ribbon summary — source and live layout

Branch: `feature/gen4-ui-renderer`.

The summary now uses original HGSS artwork throughout its background,
header, portrait/index cards, medal case and detail panel. The existing
native 32×32 icons, five original palettes and selection corners are
retained. This is a 240×160 adaptation of original source pieces, not a
claim to reproduce both 256×192 DS screens on one GBA screen.

## Corrected source decoding

The uploaded USA HeartGold ROM's NitroFS `a/1/6/2` (file ID 291) is
90,916 bytes, SHA-256
`003c80069fcd1f9c9eb842ae0294ecbdeffabbff40593cc471236e81239d9eed`.
It matches the archive tracked in
[pret/pokeheartgold](https://github.com/pret/pokeheartgold/tree/e1d5a4606640e31c44d36714ad0b5cdd9c8763fd).

Two errors in the earlier diagnostic path are corrected:

- NCGR member 1 starts its character payload at byte **48**, followed
  by 16 bytes of SOPC metadata. The old `ncgr[64:]` skipped half a tile
  and included metadata as graphics, causing the striped previews.
  The reader now follows RAHC's size/offset fields and checks bounds.
- In `include/gf_gfx_planes.h`, global layer 3 is **main BG3** and
  global layer 6 is **sub BG2**. They belong to separate DS screens.
  The earlier BG3-over-BG6 candidate composites do not represent retail
  layer ordering and are not used by the live exporter.

`PokemonSummary_Init` → `sub_020887C4` loads NCGR **1** into global
layer **6** and NCLR **0** into sub BG palette memory. The Ribbons tab
(selector **2**, verified in `sub_0208C2A0`) selects NSCR **9** in
`sub_02089F98`. The decoded yellow medal case, circles, header and party
portrait frames match the retail touch-screen reference visible in
[this HGSS playthrough](https://www.yumekichi-blog.com/entry/pokemon-ribbon7).
No main-screen layer is composited over this sub-screen map.

## GBA adaptation

`tools/gen4_ui/make_hgss_ribbon_summary.py` reads the exact hash-verified
archive. Each output tile is byte-identical to an original NCGR tile;
only tile placement, hardware flip bits and palette-bank addresses change.
The script repeats original interior/edge cells to fit the GBA layout.

| Element | Retail cells / adaptation | GBA bounds |
|---|---|---|
| Header | NSCR 9 columns 19/20/31, rows 0–3 | x=0–239, y=0–31 |
| Medal case | Yellow fill and original right edge, mirrored for left | x=80–231, y=24–95 |
| Eight circle slots | Original x=16–47, y=8–39, repeated at native size | icons x=88–215, y=32–95 |
| Index, portrait, details | Original party-frame corners/edges at x=160, y=40, repeated interiors; hardware flips close opposite edges | index y=32–63; portrait y=64–127; details x=80–239, y=96–127 |
| Shared help bar | Existing authentic HGSS Pokégear bar on BG0 | begins y=128; text y=136 |

BG2 owns the static background in character block 1. BG1 keeps the
193 icon tiles (including transparent tile zero) followed by nonoverlapping
text allocations at 0x100, 0x148, 0x162 and 0x170 in character block 3.
Text uses transparent backgrounds, so clearing old text cannot erase panel
art. Ribbon paging clears only the icon rectangle, preserving text maps.
The portrait center is (40,96), inside the space above the help bar.

Original NCLR bank **0 → GBA BG bank 1** supplies panel/text colors;
bank **4 → GBA BG bank 7** supplies the yellow case. Only those two
32-byte banks are loaded. Ribbon palettes remain in banks 2–6; shared
PokéNav BG0/help-bar colors and OBJ palettes are untouched. The owner-list
entry reloads the same original banks 1/7 on return. Live exit/re-entry capture checks
that the palette transition works.

The exporter records original tile IDs, per-tile hashes and output hashes
in `graphics/gen4_ui/hgss_ribbons/summary_bg.source.json`. CI verifies
those hashes, tilemap bounds, palette ownership and disjoint text/icon
VRAM. The proprietary ROM and raw archive remain outside the repository.

## Verification and cleanup

See [live visual acceptance](hgss_ribbon_visual_qa.md) for the framebuffer
checks and recorded screenshots. Remove only superseded summary resources
after those checks pass. The historical candidate/audit utilities remain
available for lossless tile-operation tests; their main/sub combinations
are diagnostic experiments, not a retail screen compositor.

Selection corners retain their original 16×16 tiles. Their opaque pixels
span 2–12, so centers at ±10 produce a true 32×32 visible frame. This
keeps second-row selection marks above the detail text.

## Owner-list adaptation and memory ownership

`make_hgss_ribbon_list.py` reuses the verified summary tiles and palettes.
The top three tile rows preserve the yellow header; the party-frame cells
become 200×16 owner cards by repeating edge interiors and baking their
hardware flips losslessly. `owner_list/source.json` links the output hashes
and source cell attributes to the original summary provenance manifest.

| Resource | GBA allocation |
|---|---|
| Shared help bar | BG0, character block 0, screen block 5, priority 0 |
| Original header and viewport mask | BG1, character block 1, screen block 6, priority 2 |
| Circular owner-row window | BG2, character blocks 2–3, screen block 7, priority 3; 802 tiles including the two base tiles |
| Transparent title and index text | BG3, character block 1, screen block 4, priority 1; tiles 64–105 |

The BG1 mask is transparent only at tile rectangle (3,3), 25×12 tiles.
It hides offscreen entries as BG2 scrolls. The BG3 text map is separate so
clearing the title/index cannot punch holes through that mask. The asset
validator rejects overlapping VRAM regions and a viewport that differs
from the live C list geometry. No shared BG0 palette or sprite resources
are replaced by the owner-list adapter.
