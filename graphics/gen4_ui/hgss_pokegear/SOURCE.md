# Verified HGSS PokéGear sources

## Main app-switch surface — phase 1

This directory starts the authenticity-first replacement of the current
PokéGear/PokéNav presentation.

The live GBA PokéGear currently contains several programmatically drawn
"HGSS-style" panels. Those are not accepted as final production artwork under
the project's authenticity-only policy. Before replacing them, this phase
imports the exact retail HeartGold/SoulSilver source used by the PokéGear
app-switch layer.

Retail mapping is verified against `pret/pokeheartgold` revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`, in
`src/application/pokegear/main/overlay_100_021E5900.c`.

For default skin 0, `PokegearApp_LoadSkinGraphics` selects:

- member 48 character graphics for the main PokéGear BG,
- member 30 BG palette,
- member 54 screen/tilemap data for the app-switch layer.

`PokegearApp_DrawAppButtons` then copies that member-54 screen data into the
live app-switch strip.

The three files in `verified/` are copied byte-for-byte from the retail HGSS
decomp/extraction. No pixels, palette entries, tile indices, or geometry have
been redrawn or synthesized.

This is **phase 1 only**. Nothing is wired into the GBA PokéGear yet, and no
legacy PokéNav/PokéGear equivalent is removed in this commit.

## Main app-switch surface — phase 2

The verified retail HGSS app-switch source is now live in the GBA PokéGear.

`tools/gen4_ui/make_hgss_pokegear_app_switch.py` reconstructs the normal
member-54 app-switch state from the exact member-48 tile pixels and member-30
palette. The DS source is 256x32. Its only adaptation is removing the empty
8-pixel margin at each side, yielding a 240x32 GBA-width strip. Source pixels
remain 1:1; there is no scaling, redrawing, recoloring, or interpolation.

The live strip is loaded on BG2 at tile base `0x100` using dedicated palette
bank 14 and replaces only tilemap rows 0..3. Existing option sprites, cursor
movement, menu input, descriptions, transitions, and submenu logic are
unchanged.

The previous PokéNav device chrome intentionally remains loaded underneath the
retail strip for this phase-2 verification cycle. It is not removed until CI
confirms the live integration.

### Phase-2 build portability correction

CI #620 showed that the general ROM/test jobs do not install Pillow. The
app-switch generator therefore no longer depends on `PIL.Image`.

The generator now decodes the committed 4-bit indexed member-48 PNG and writes
the 240x32 indexed output using only Python's standard library
(`struct`, `zlib`, and CRC32). The retail source members, NSCR reconstruction,
BGR555 colors, 1:1 geometry, and empty-margin-only crop are unchanged.

No legacy PokéGear artwork is removed by this correction.

## Main app-switch surface — phase 3

CI #621 passed the corrected phase-2 integration across Gen 4 UI validation,
Emerald, FireRed, LeafGreen, release, docs, and the general test suite.

The superseded legacy app-switch region has now been removed from
`graphics/pokenav/device_outline_map.bin`. Tilemap rows 0..3 are permanently
blanked with the map's canonical blank entry (`0x2000`) before the authentic
HGSS strip is installed.

This means the verified retail HGSS strip is now the **only live background**
for the app-switch surface rather than an overlay hiding old artwork. Seven
legacy tiles that were exclusive to those rows (4, 8, 9, 12, 16, 17, and 20)
are no longer referenced by the live tilemap.

The shared legacy device character sheet is not deleted yet because other tiles
still render the lower PokéNav shell. That lower shell is a separate surface
and will be replaced through its own authentic-HGSS asset → wire-live → remove
legacy-equivalent sequence.

The app-switch surface sequence is therefore complete:

**authentic HGSS asset → wire live → remove legacy equivalent**

## Fixed screen shell — phase 1

With the app-switch strip complete, the next legacy surface is the remaining
PokéNav device shell below it.

Retail HGSS provides an exact default-skin source set for the fixed PokéGear
screen layer in `PokegearApp_LoadSkinGraphics`:

- member 36 character graphics,
- member 24 BG palette,
- member 42 NSCR screen/tilemap.

For skin 0, those are loaded together on `GF_BG_LYR_SUB_0` in the retail
PokéGear. Member 36 is a 256x32 indexed source sheet and member 42 defines a
256x192 screen.

The three files in `verified/` are copied byte-for-byte from
`pret/pokeheartgold` revision `9d8b7591f09b65804da2fb2dfd56f320633e0d36`.

This is **phase 1 only** for the fixed shell. The current lower PokéNav device
artwork remains live until the verified retail shell is adapted and wired in a
separate phase.

## Fixed screen shell — phase 2

The verified retail default-skin screen shell is now live on the GBA PokéGear
without scaling or redrawing.

`tools/gen4_ui/make_hgss_pokegear_screen_shell.py` adapts member 42 at tile
granularity. It keeps NSCR columns 1..30, so the 256-pixel DS canvas becomes
the 240-pixel GBA viewport by omitting one 8-pixel edge column on each side.
Vertically it keeps source rows 0..11 and 16..23; rows 12..15 are a uniform
32-pixel spacer in the retail screen map, so removing only that spacer converts
192 pixels to 160 while every retained artwork pixel remains 1:1.

The member-36 NCGR-derived indexed PNG is compiled directly as 4bpp tiles at
BG2 tile base `0x40`. The adapted tilemap preserves the retail tile identity,
horizontal/vertical flip bits, and palette-bank semantics while offsetting tile
IDs by that GBA tile base. Member 24 palette bank 13 is emitted directly from
the verified NCLR and loaded into BG palette bank 13.

The authentic app-switch strip is loaded afterward and continues to own rows
0..3. The old PokéNav device shell still loads first underneath the HGSS shell
for this phase-2 verification cycle; its code and assets are not removed yet.


## Fixed screen shell — phase 3

CI #624 passed the phase-2 fixed-shell integration across the full workflow,
including Gen 4 UI validation, Emerald, FireRed, LeafGreen, release, docs, and
the general test suite.

The superseded PokéNav device shell has therefore been removed from
`src/pokenav_menu_handler_gfx.c`. Its palette/tile/tilemap declarations and
the state-1 BG2 load are gone. State 1 remains only as a sequencing gate so the
existing asynchronous load cadence is preserved before later layers are
installed.

A branch-wide check of the PokéNav source found `device_outline` referenced
only by that main-menu graphics module. With its final live use removed, the
now-unreferenced legacy source files are deleted as well:

- `graphics/pokenav/device_outline.png`
- `graphics/pokenav/device_outline_map.bin`

The authentic retail HGSS member-36/member-24/member-42 shell is now the only
live BG2 device-shell artwork. The already-authentic app-switch strip still
loads afterward on rows 0..3. No other PokéGear/PokéNav surfaces are changed by
this cleanup.

The fixed screen-shell sequence is therefore complete:

**authentic HGSS asset → wire live → remove legacy equivalent**


## Main UI sprite layer — phase 1

With the authentic app-switch background and fixed screen shell complete, the
remaining launcher still draws its interactive option layer from the legacy
PokéNav sprite sheet and palettes in `src/pokenav_menu_handler_gfx.c`
(`gPokenavOptions_Gfx` / `gPokenavOptions_Pal`).

Retail HeartGold/SoulSilver has a distinct common PokéGear UI sprite resource
set. At pret/pokeheartgold revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`,
`PokegearUIManager_LoadInitialSkinGfx` in
`src/application/pokegear/main/overlay_100_021E6914.c` loads, for default
skin 0:

- member 6 character graphics (the committed source PNG compiled to NCGR),
- member 0 OBJ palette,
- member 12 NCER cell layout,
- member 13 NANR animation data.

`PokegearApp_LoadGraphics` in
`src/application/pokegear/main/overlay_100_021E5900.c` then creates the
shared PokéGear UI sprites from that resource set, including the app-switch
cursor sprites and the clock/day/status widgets.

The four verified files in `verified/` are copied byte-for-byte from that
retail HGSS source revision. They are source evidence only in this phase:
nothing is wired into the GBA launcher yet, and no legacy PokéNav option
sprite/palette is removed.

The next phase will adapt only the retail cells/frames that are useful on the
240x160 GBA launcher and wire those authentic pixels live while retaining the
legacy sprite layer underneath until CI passes.


## Main UI selection cursor — phase 2

Decoding the verified member-13 NANR and member-12 NCER narrows the direct
retail equivalent for this phase to the PokéGear's four-corner selection
cursor, rather than the functional PokéNav text labels themselves.

Retail animation sequences 4, 5, 6, and 7 resolve to NCER cells 20, 21, 22,
and 23. Each cell is one 16x16 OBJ using member-6 character tile 208 and
member-0 palette bank 1. Cells 21..23 are only the V-flipped, H-flipped, and
H+V-flipped forms of cell 20.

`tools/gen4_ui/make_hgss_pokegear_cursor.py` validates that exact
NANR/NCER relationship at build time. It then emits only the four consecutive
8x8 tiles used by base cell 20 (128 bytes of GBA 4bpp) and the exact 16-color
BGR555 palette bank used by the retail cell. No source pixel or palette color
is redrawn, recolored, interpolated, or scaled.

The live GBA launcher now instantiates four 16x16 sprites from that authentic
cell and applies the same flip pattern as the four retail sequences. The
corners are separated spatially to frame the existing 128x16 functional
launcher label target; their pixels remain 1:1. The cursor is shown only on
the top-level PokéGear menu and follows the live menu selection.

For this phase-2 validation cycle, the previous scanline/lighten selection glow
remains active underneath the authentic cursor. The legacy option-label
graphics also remain untouched because they are a separate surface, not a
pixel-equivalent of the retail cursor. If CI passes, phase 3 will remove the
superseded top-level legacy selection glow while preserving submenu behavior.


## Main UI selection cursor — phase 3

CI #627 passed commit `9901d58c30c200fef677470bcac5c1724f6eb3ea`,
validating the authentic retail HGSS cursor integration.

The superseded **top-level** PokéNav scanline/lighten selection effect has now
been removed from the live PokéGear menu. On top-level PokéGear menu types,
the code clears the scanline highlight buffers, disables OBJ lighten blending
and WIN0, holds `BLDY` at zero, and stops the old pulse task from changing the
blend amount. The authentic member-6/member-0 cursor corners are therefore the
only live top-level selection indicator.

The scanline/lighten implementation itself is intentionally retained for the
Condition and Condition Search submenus because those are separate legacy
surfaces that have not yet gone through their own authenticity replacement
sequence. Entering those submenus re-enables the existing window/blend effect;
returning to the top-level PokéGear disables it again.

The legacy PokéNav option-label graphics are also retained. Decoding the retail
NCER/NANR established that they are not the pixel-equivalent of this HGSS
cursor and must be migrated as a separate surface rather than being deleted
under the cursor sequence.

The cursor sequence is therefore complete:

**authentic HGSS asset → wire live → remove legacy equivalent**


## Synthetic description strip — authenticity cleanup

CI #628 passed commit `46d72676a3dd2ce95f6a5e388cc2c32a603bcda8`,
completing validation of the retail HGSS cursor sequence.

The remaining `DrawPokeGearDescriptionPanel()` surface was audited against
the retail PokéGear main loader at the pinned pret/pokeheartgold revision.
Retail HGSS does not provide or draw a separate beveled description strip in
that layer. The GBA function was therefore an invented "HGSS-style" overlay,
not an authentic asset that should be preserved or imitated.

No substitute artwork is generated. The synthetic bevel/fill routine has been
removed, and the functional description window is now cleared to transparent
4bpp index 0 before text is printed. The text printer also uses transparent
background index 0. This exposes the already-verified retail HGSS fixed shell
(member 36 character graphics, member 24 palette, member 42 screen data) on
BG2 underneath the text instead of covering it with fabricated pixels.

This cleanup reuses an authentic HGSS asset that was already imported and
wired live by the completed fixed-shell sequence; it does not create a new
"HGSS-style" replacement.


## App-switch selected state — live wiring

CI #629 passed commit `dfe7d3f9eb6e8b1d1ca3623925eec8c8cd6fa6f7`,
validating removal of the synthetic description panel.

The already-verified retail member-54 NSCR contains three four-tile-high
app-button state bands. Retail `PokegearApp_UpdateAppSwitchButtonBGState`
uses rows 0..3 for normal buttons and rows 4..7 for the selected state.

Auditing the retail copy coordinates also exposed a flaw in the earlier GBA
width adaptation. The retail five-button layout is:

- tile column 0: 8px left margin,
- columns 1..24: four contiguous 48px buttons,
- column 25: 8px spacer,
- columns 26..31: the full 48px Cancel button.

The previous outer-edge crop omitted column 31 and therefore discarded the
rightmost eight pixels of Cancel. The generator now removes only source
columns 0 and 25. This preserves every pixel of all five retail 48px buttons
and packs them exactly into the GBA's 240px width: 5 × 48px.

`make_hgss_pokegear_app_switch.py` now emits the retail normal and selected
states separately. Each state is reconstructed from the exact member-48 tile
pixels, member-30 BGR555 palette values, and member-54 tile/flip/palette
semantics; there is no scaling, redrawing, recoloring, or interpolation.

Both states are live on BG2. The normal state uses palette bank 14 and tile
base `0x100`; selected-state tiles use palette bank 15 and tile base
`0x178`. Moving the top-level PokéGear cursor restores the full normal strip
then substitutes only the six authentic selected-state tiles for the mapped
48px button.

For this validation phase, the existing PokéNav option-label sprites remain
live. They are not deleted until the authentic button-state integration passes
CI and the separate label/function presentation is handled deliberately.


## Top-level launcher labels — legacy removal

CI #630 passed commit `2723bae4ece9dad61a875e7199fcae7a7d9b04ac`,
validating the authentic retail HGSS normal/selected app-button states.

The top-level launcher no longer draws the vertical PokéNav option-label
sprites. Retail HGSS presents this surface through the app-switch buttons
themselves, so retaining a second Emerald/PokéNav vertical launcher over the
verified member-48/member-30/member-54 button strip was a legacy presentation
layer rather than an authentic HGSS component.

The five top-level PokéNav label tile ranges are no longer referenced by
`src/pokenav_menu_handler_gfx.c`. The shared `gPokenavOptions_Gfx` /
`gPokenavOptions_Pal` resource remains loaded only because Condition and
Condition Search still use later ranges from that same legacy sheet; those
submenus are separate migration surfaces and are unchanged by this cleanup.

The authentic four-corner cursor is also re-anchored to the app buttons using
the retail `PokegearCursorManager` geometry: x offsets ±16 and y offsets
±10 around the retail button centers. After the existing lossless GBA width
adaptation, the button centers are 24, 72, 120, 168, and 214 pixels. No cursor
pixel is scaled, redrawn, or recolored.

The current function mapping is intentionally still the one validated in the
selected-state phase: Condition reuses Configure, Ribbons reuses Radio, Map
uses Map, Match Call uses Phone, and Switch Off uses Cancel. Condition and
Ribbons are slot reuse, not claimed direct retail HGSS functional equivalents.

This completes removal of the **top-level legacy label presentation** after
the authentic HGSS app-button state integration. Condition/Search labels remain
pending their own authenticity sequence.


## Match Call notification — phase 1

CI #631 passed commit `8c35d421cbb173218985225acd52c40c202fe23b`,
validating removal of the top-level legacy PokéNav launcher labels.

The next remaining top-level legacy element is the Emerald/PokéNav Match Call
blue light (`graphics/pokenav/blue_light.png`). Its live behavior is implemented
by `CreateMatchCallBlueLightSprite`, `SpriteCB_BlinkingBlueLight`, and
`AreAnyTrainerRematchesNearby`.

Retail HGSS does not contain a direct "nearby rematch blue light." The
authenticity audit instead found the PokéGear's own phone-status indicator in
the already-imported retail common-UI sprite set. In
`PokegearApp_LoadGraphics`, UI sprite 10 uses NANR sequence 3. That sequence
resolves to NCER cells 18 and 19, which are untouched 16x16 square objects using
member-6 tiles 200 and 204 with member-0 palette bank 3. Retail uses frame 0 as
the active/available phone state and switches to frame 1 when
`MapHeader_CanPlacePhoneCalls` is false.

`make_hgss_pokegear_phone_status.py` now validates that exact retail binding
and extracts both 16x16 states byte-for-byte into a two-frame GBA 4bpp asset plus
the original BGR555 palette. There is no scaling, redrawing, recoloring, or
synthetic replacement artwork.

This is a deliberate functional reuse of authentic HGSS phone artwork, not a
claim that HGSS itself used this icon for rematch availability. Phase 2 will
bind the authentic phone-status graphic to the existing Match Call notification
predicate. The legacy blue-light sprite remains live and untouched until that
integration is CI-validated.

This commit is **phase 1 only**:

**authentic HGSS asset ready -> wire live pending -> legacy removal pending**


## Match Call notification — phase 2

CI #632 passed commit `52c3a6f7f008cca20cea1340fb1d57a3f83cf2c3`,
validating the phase-1 extraction of the authentic retail HGSS phone-status
sprite.

The active retail phone-status frame is now live as the top-level nearby-rematch
notification. The existing Emerald predicate, `AreAnyTrainerRematchesNearby`,
is deliberately retained; only its presentation changes.

Retail `PokegearApp_LoadGraphics` places UI sprite 10 at (197, 48) on the
256-pixel DS PokéGear screen. The already-validated fixed-shell GBA adaptation
keeps source columns 1..30, so the live GBA sprite is placed at (189, 48):
exactly one 8-pixel source column is removed from x and y remains unchanged.
The 16x16 sprite pixels and palette are not scaled, redrawn, or recolored.

When a nearby rematch exists, NCER cell 18 / member-6 tile 200 is displayed
with the original notification blink cadence. The indicator is hidden while a
Condition/Search submenu is active and is restored immediately on return to the
top-level PokéGear. The retail disabled frame (cell 19 / tile 204) remains
preserved in the generated asset for future phone-state use.

For this validation phase, the old `graphics/pokenav/blue_light.png` resource,
sprite template, callback, tags, and allocation path are intentionally retained.
The legacy sprite is instantiated but forced invisible, making the authentic
HGSS phone-status graphic the only visible notification while preserving a
rollback-safe equivalent until CI validates the live integration.

This is **phase 2**:

**authentic HGSS asset -> wired live -> legacy removal pending**


## Match Call notification — phase 3

CI #633 passed commit `327470de41cf68d7d756cf17c68f1e398b7d2f0a`,
validating the authentic HGSS phone-status sprite as the live top-level
nearby-rematch notification across Gen 4 UI validation, Emerald, FireRed,
LeafGreen, release, docs, and the general test suite.

The superseded PokéNav blue-light implementation has now been removed completely.
`graphics/pokenav/blue_light.png` is deleted, and
`src/pokenav_menu_handler_gfx.c` no longer contains its graphics/palette tags,
INCGFX bindings, compressed sprite-sheet entry, palette entry, 32x16 OAM
template, hidden rollback sprite, blink callback, allocation path, or cleanup
calls.

The live path now creates and destroys only the authentic 16x16 HGSS
phone-status sprite. Its rematch predicate and validated top-level/submenu
visibility behavior are unchanged from phase 2.

This completes the Match Call notification sequence:

**authentic HGSS asset -> wire live -> remove legacy equivalent**


### Phase-3 build dependency correction

CI #634 exposed one final non-live dependency that the phase-3 audit missed:
`src/pokenav_main_menu.c` still declared
`sBlueLightCopy`, an explicitly unused INCGFX copy of the deleted
`graphics/pokenav/blue_light.png`. Although it was never rendered, Make still
tracked the source PNG through the generated dependency file, causing all normal
ROM/test builds to fail after the asset deletion.

The unused declaration is now removed. No authentic HGSS behavior or live
Match Call notification code changes in this correction. The next UI surface
will not begin until this cleanup passes CI.


## Condition / Condition Search submenu labels — phase 1

CI #635 passed commit `88475be036d413aa531f7b3b954b75f0cd50c673`,
validating the final Match Call blue-light dependency cleanup. The next
remaining shared PokéNav presentation on the launcher path is the
Condition/Condition Search option-label set.

Retail HGSS has no PokéNav Condition feature and therefore no direct retail
**Party / Search / Cool / Beauty / Cute / Smart / Tough / Cancel** graphics.
The authentic analogue chosen here is the HGSS Pokédex list/search
presentation, because it is a retail filter-selection interface with explicit
cancel behavior. This is functional reuse of genuine HGSS presentation, not a
claim that HGSS contained an equivalent Condition application.

The exact retail Pokédex binding in `pret/pokeheartgold` uses
`zukan_gra` member 58 character graphics, member 57 screen data, and member 2
palette data. The project already has a checksum-verified rendered member-57
source raster in `make_hgss_pokedex_stats.py`; its SHA-256 is
`7765c42ee555e158c34e45a8fc380abb0540cbfdbff5508538a986ca543e9698`.

For this phase, `make_hgss_pokegear_condition_search.py` extracts only retail
pixels: the 8x8 pale-blue list interior at source (32, 8), the 8x8
orange/purple list rule at source (32, 24), and the authentic lavender list
pointer already verified from source (120, 12). The chrome preserves source
palette indices 0..14 exactly. The pointer preserves the same visible retail
pixels and authentic lavender color inside an 8x16 transparent GBA OBJ
container.

There is no scaling, redrawing, recoloring, selected-state brightening,
disabled-state desaturation, or other synthetic state.

HGSS renders localized list text dynamically rather than baking these words
into the graphics. The Pokédex allocates font ID 4; `src/font.c` maps that to
`NARC_graphic_font` member 4
(`files/graphic/font/font_00000004.bin`, Git blob
`a4b7623e8061d2a2867510bd5d8b4a7f5db11bf2`). The corresponding authentic
font palette is member 7, already verified in this project for Storage. Later
wiring must therefore render the project menu words from exact font-ID-4 glyph
pixels rather than retain Emerald label pixels or use an approximation.

Two previously existing derivatives are explicitly excluded from this
migration: the Pokédex Search overlay's synthetic selected/disabled palette
variants, and the Pokédex START-menu generator's embedded legacy English label
masks.

The current shared `gPokenavOptions_Gfx` / `gPokenavOptions_Pal` label
ranges remain live and untouched. This is phase 1 only:

**authentic HGSS source package ready -> wire live pending -> legacy removal pending**
