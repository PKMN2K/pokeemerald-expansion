# HGSS Ribbon BG — verified source and remaining integration gate

Branch: `feature/gen4-ui-renderer`.

## Already completed (do not repeat extraction)
- Original retail HeartGold `a/1/6/2` NARC, NitroFS file ID 291, has been examined; it has 79 members.
- Member 0 supplies an original NCLR with sixteen 16-colour palette banks.
- BG3 character resource: NCGR member 2, with NSCR members 10 or 11 depending on retail state.
- BG6 character resource: NCGR member 1, with NSCR member 9 or fallback 78 depending on retail state.
- Original BG6 NSCR 9 visibly includes the nine-circle ribbons panel.
- ROM-derived source-layer previews, reproducible renderer, and 240x160 viewport experiments already exist. See `graphics/gen4_ui/hgss_ribbons/SOURCE.md` and `tools/render_hgss_ribbon_backgrounds.py`.

## Actual unresolved integration work
1. Establish which **retail layer combination, priority, and origin** corresponds to the ribbon summary, including windows and sprite overlays.
2. Adapt **original extracted graphics pieces**, without drawing or recolouring, to fit the live Emerald 240x160 screen. A simple source crop fails: y=0 cuts lower controls, whereas lowering the viewport clips the upper grid.
3. Respect verified live window footprints: ribbon icons x=88..215, y=32..95; BG2 grid window x=80..239, y=24..103; name x=112..215, y=8..23; count x=96..223, y=96..127; list index x=8..63, y=40..55; Pokémon sprite anchor (40,104).
4. Wire the authentic BG, inspect the real mGBA framebuffer and navigation states, then remove only unused Emerald graphics after checking all references.

**Current status:** retail sources and renderers verified; the final authentic BG composition and live wiring are not completed. No invented HGSS-style graphics.

This file corrects an earlier stale checkpoint which incorrectly asserted that the original BG archive and members were unknown.


## Batch source-only composition audit

Once the verified original retail `a/1/6/2` NARC is available locally, run:

```sh
python3 tools/audit_hgss_ribbon_bg_candidates.py /path/to/a_1_6_2.narc build/hgss-ribbon-bg-audit
```

The script verifies the exact known SHA-256 before parsing. It compares all
three provisional BG6 layouts and all tile-aligned 240x160 viewport origins,
writing per-candidate `.audit.json` files and a sortable
`comparison.json`. These contain palette-bank pairs and genuinely
unrepresentable transparent-overlay pixel counts, but **are not proof of the
correct retail arrangement**. The original source archive is not committed.
Visual reference and BG priority/scroll verification remain required before
any live PokéNav wiring or removal of Emerald graphics.

## Live background replacement gate (2026-10-27 audit)

The current live entry point is `LoopedTask_OpenRibbonsSummaryMenu` in
`src/pokenav_ribbons_summary.c`. Its state 0 loads the **Emerald**
`gPokenavRibbonsSummaryBg_Gfx`, `gPokenavRibbonsSummaryBg_Tilemap`,
and `gPokenavRibbonsSummaryBg_Pal` onto BG2. State 1 separately installs
original HGSS 32x32 ribbon tiles and five verified palettes on BG1.
The current blank BG2 window fills for the grid, ribbon description,
Pokémon name and list index are placeholders, not retail HGSS chrome.

### Required evidence before the next live graphics replacement

1. Use the existing original `a/1/6/2` NARC extraction and source-layer
   renderer. Match BG3 NCGR member 2 with NSCR 10/11 and BG6 NCGR member 1
   with NSCR 9/78; document the selected pair, palette banks, origins and
   layer priority using a **retail screenshot** reference.
2. Check a 240x160 composed preview against the existing PokéNav footprints
   listed above, especially the name card, ribbon descriptions, last grid
   row and help bar. Do not treat a cropped 256x192 DS framebuffer or
   isolated BG preview as a verified GBA layout.
3. Import only the verified original source pixels/tilemaps and document
   provenance. Wire the new BG resources into the real opening state while
   preserving the native 32x32 HGSS ribbon icon BG1 and four-corner OBJ.
4. Review uploaded **live** mGBA framebuffer screenshots at normal, gift,
   expanded and empty-ribbon states. Only after this verification search
   all references to the old Emerald BG assets and remove truly unused
   equivalents. Rerun all three workflows.

**Stop condition:** An unverified layer combination, unknown window artwork,
or clipped real ribbon control must block any declaration that the Emerald
background has been replaced. The existing screenshot CI verifies operation
and selected icon pixels, not visual equivalence to the retail HGSS full UI.

## Uploaded HeartGold source audit — provisional viewport (0,8), 2026-10-28

The user-provided USA HeartGold ROM (`IPKE01`) supplied NitroFS
`a/1/6/2` (file ID 291). Its SHA-256 matches the existing exporter
allowlist: `003c80069fcd1f9c9eb842ae0294ecbdeffabbff40593cc471236e81239d9eed`.

A read-only, source-cell audit of BG3 NSCR 10 over BG6 NSCR 9,
with the source viewport at (0,8), found:

| Provisional trial | Source BG palette banks | BG6 cells with visible artwork | Cross-bank BG6 overlay cells needing exact-colour resolution |
| --- | --- | ---: | ---: |
| `header_only` | 0, 2, 4 | 64 | 0 |
| `header_footer` | 0, 2, 4 | 184 | 2 |
| `left_case` | 0, 2, 4 | 210 | 2 |

Counts are from the archive's original 8x8 cells and use the existing
protected-window geometry. They are **not** counts of successfully exported
GBA composite tiles: transparent/partial overlays and palette-bank
translations still require the full exporter diagnostics.

**Implementation gate remains closed:** these are retail-source
measurements, not proof of the actual DS layer priorities, scroll offsets,
or correct GBA layout. All three candidates require a collision-safe
cross-module BG palette mapping; merely using source banks 0, 2 and 4
would overlap currently loaded PokéNav palettes. Do not commit the ROM or
its raw archive, replace Emerald BG resources, or remove legacy art until
the GBA exporter and live framebuffer review establish correctness.
