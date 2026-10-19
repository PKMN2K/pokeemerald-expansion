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
