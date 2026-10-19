# HGSS retail ribbon-screen background trace — investigation checkpoint

Target branch: `feature/gen4-ui-renderer`

## Source-backed observations

- The retail ribbon sprite setup routine `sub_0208B1AC` in
  [pret/pokeheartgold/asm/unk_0208B1AC.s](https://github.com/pret/pokeheartgold/blob/master/asm/unk_0208B1AC.s)
  initializes sprite systems, resource databases and sprite VRAM. This
  **does not establish** the ribbon screen's BG tilemap, BG palette or
  window-frame source. In particular, the ribbon icon archive
  `a/0/3/9` is not evidence for the screen background.
- The current fork's `src/pokenav_ribbons_summary.c` still initializes
  the live summary from Emerald's
  `gPokenavRibbonsSummaryBg_Gfx`,
  `gPokenavRibbonsSummaryBg_Tilemap` and
  `gPokenavRibbonsSummaryBg_Pal`.
- The geometry audit `tools/audit_hgss_ribbon_gba_layout.py`
  describes screen occupancy, not original retail background provenance.
  Passing geometry checks does not authorize replacing graphics.

## Exact next extraction gate

1. Trace **the caller / owning overlay** of `sub_0208B1AC`, not just
   its sprite resource list; locate initialization of BG layers and
   window resources for the actual retail ribbon-details scene.
2. Record the loader's archive constant, member index, compression,
   palette assignment, tile dimensions and resource identifiers.
3. Cross-check each claimed background/window resource against original
   HeartGold NitroFS bytes and the overlay's consumption path. Record
   SHA-256 hashes and do not include proprietary ROM dumps.
4. Only after this mapping is verified, extract **original** retail
   tiles/palettes, convert without painting new UI art, adapt layout
   to 240x160, and replace live Emerald BG bindings.
5. Inspect native mGBA captures of the full live PokéNav summary
   (including selected ribbons and text). Audit all legacy build/runtime
   references before removing superseded Emerald assets.

**Status:** Source BG/window archive and members are still unverified.
Do not mark HGSS ribbon-screen background as ported. This checkpoint
adds no artwork and changes no game behavior.
