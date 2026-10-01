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


## Condition / Condition Search submenu labels — phase 2

CI #636 passed commit `98f554561be9a23252650c415174a337fa26611b`,
validating the phase-1 authentic HGSS source package.

The two Condition submenus now use the authentic replacement path. BG2 is
framed with the exact member-057 list interior/rule pixels; selection uses the
exact member-057 lavender pointer; and Party/Search/Cancel plus the five
Condition categories are rendered from exact retail HGSS font-ID-4 glyph bytes
at 1:1 scale with the verified member-7 font palette.

The project-specific words are not claimed to be baked retail HGSS graphics:
HGSS renders list text dynamically. Their glyph pixels are retail HGSS font
pixels, while the words continue to express the existing Emerald gameplay
semantics. Font background/no-draw classes remain transparent so the authentic
member-057 chrome is visible beneath them.

Existing menu IDs, cursor indices, descriptions, feature launches and input
logic are unchanged. The description strip moves down one tile to row 18 to
leave the six-row Search panel clear.

For rollback safety, the legacy `gPokenavOptions_Gfx` /
`gPokenavOptions_Pal` submenu ranges, sprite allocation and glow code remain
compiled, but the legacy labels are forced invisible while either Condition
submenu is active. They will not be removed until CI validates this live path.

This is **phase 2**:

**authentic HGSS asset -> wired live -> legacy removal pending**


## Condition / Condition Search submenu labels — phase 3

CI #641 passed the phase-2 authentic HGSS Condition/Search integration at
`8714a052d77a340e0331e3f3979b61edcd0639f1`, so the rollback-only Emerald
option-label presentation has now been removed.

The cleanup removes the legacy `gPokenavOptions_Gfx` /
`gPokenavOptions_Pal` bindings, the old option-label OBJ allocation and
slide/zoom/blend animation path, and the scanline/lighten selection-glow path.
The superseded source label PNGs and `graphics/pokenav/options/options.pal`
are deleted as well. The live submenu remains the phase-2 authentic HGSS path:
member-057-derived chrome and pointer plus exact font-ID-4 glyph pixels.

`graphics/pokenav/options/options.bin` is deliberately retained. It is not
part of the removed label sprite sheet: `src/pokenav_conditions_gfx.c` still
uses `gPokenavOptions_Tilemap` from that file when the Condition detail screen
is opened in search mode.

This completes the implementation side of:

**authentic HGSS asset → wire live → remove legacy equivalent**

The phase-3 cleanup head must still pass CI before moving to the next UI surface.


## Shared PokéNav list arrows — phase 1

CI #646 passed `2cc264e8c6793e74d03ca4887d9cc2f4f7d68491`, completing
validation of the Condition/Search cleanup before this next surface.

The shared list used by Match Call, Condition search results, and Ribbon lists
still loads `graphics/pokenav/list_arrows.png`. Its replacement is prepared by
`make_hgss_pokegear_list_arrows.py`:

- Tiles 0–1: exact 8x16 lavender pointer from Pokédex member 057, source
  x=120..124 / y=12..19, with transparent padding from the verified START
  cursor builder.
- Tiles 2–3: vertical reflection of the exact 16x8 member-000 scroll control,
  source x=240..255 / y=0..7.
- Tiles 4–5: the same member-000 scroll control in its original orientation.

The existing source builders verify the retail-derived raster checksums.
The arrow extraction additionally verifies SHA-256
`ea5b9b2ac4141931be38eb7ac8e9ae1866ec26b4b2e252fe64f8aaeded5d6a36`
on its remapped source pixels before this adapter combines the sheets.
The combined OBJ palette preserves member-057 lavender and member-000
pink/white/black source RGB values through the existing RGB555 conversion.
No pixels are drawn, scaled, or recolored; only indices are merged and the
up/down orientation is reflected. The six-tile layout matches the live shared
list's current allocation and tile offsets, ready for the next wiring step.

Build targets are `hgss_pokegear/list_arrows.4bpp` (192 bytes) and
`hgss_pokegear/list_arrows.gbapal` (32 bytes), both covered by the Gen 4 UI
asset CI job. No C binding or legacy asset is changed in this phase.

**authentic HGSS asset prepared → wire live pending → legacy removal pending**


## Shared PokéNav list arrows — phase 2

CI #647's Gen 4 UI asset/renderer job and Emerald/FireRed/LeafGreen ROM
builds passed the phase-1 source commit `0d09d140f24698c492cedaa02644cc4a15f0a90e`.
The general Test job was still running when this integration was prepared.

`src/pokenav_list.c` now loads the authentic six-tile sheet with
`LoadSpriteSheet`, followed by the combined authentic OBJ palette through the
existing PokéNav palette allocator. This shared path supplies Match Call,
Condition search results, and Ribbon lists.

The 8x16 pointer and 16x8 vertical-arrow OAM sizes, tile offsets 0/2/4,
positions, selected-row tracking, scrolling visibility checks, bob animation,
hide/show callbacks, tags, and resource cleanup remain unchanged. The Gen 4 UI
CI compile job explicitly includes `pokenav_list.o` to validate the new binding.

The legacy compressed sheet, palette, and PNG remain as a temporary
allocation-failure fallback while the live integration is validated. Success is
checked using `GetSpriteTileStartByTag`: `LoadSpriteSheet` returns zero on
allocation failure, and zero is also a valid tile start, so its return value
must not be treated as a failure sentinel. Legacy removal is the next phase
after this live commit passes CI.

**authentic HGSS asset → wired live → legacy removal pending**


## Shared PokéNav list arrows — phase 3

CI #648 passed the live integration commit
`86913f26cd18fd7c7714b27fddf789a39682804e`, including all ROM builds,
the Gen 4 UI asset/renderer job, and the general Test job.

The shared list now unconditionally loads its verified HGSS six-tile sheet and
combined OBJ palette. Removed the legacy compressed-sheet and palette
bindings/descriptors, allocation-failure fallback loop, and superseded
`graphics/pokenav/list_arrows.png`. No active code or build rule references the
old graphic. Historical phase notes above describe the staging sequence only.

The authentic pointer/scroll source generators and provenance, sprite sizes,
positions, tile offsets, tags, callbacks, and destruction path remain intact.
This completes the implementation sequence for the shared arrows used by
Match Call, Condition search results, and Ribbon lists:

**authentic HGSS asset → wire live → remove legacy equivalent**

The cleanup commit must pass CI before starting the next UI surface.


## Match Call options-menu cursor — phase 1

CI #649's Gen 4 UI and ROM build jobs passed the shared-arrow cleanup at
`a807fc5895cc9f1b5c9fb71d9c35a5f49f608e5e`; its general Test job was still
running when this source package was prepared.

Match Call has a separate options-menu cursor, independent of the shared list
arrows. It still uses `graphics/pokenav/match_call/options_cursor.png` through
`sOptionsCursorSpriteSheets` in `src/pokenav_match_call_gfx.c`.

The authentic replacement reuses the exact lavender Pokédex member-057 pointer
already verified by `make_hgss_pokedex_start_cursor.py`. The visible pixels at
x=120..124 / y=12..19 are copied 1:1 into an 8x16 OBJ with transparent padding.
There is no scaling, redrawing, or recoloring. This is explicit reuse of an HGSS
Pokédex control for Emerald's Match Call semantics, not a claim that it is a
retail HGSS phone-menu cursor.

Build targets `hgss_pokegear/match_call_options_cursor.4bpp` (64 bytes) and
`hgss_pokegear/match_call_options_cursor.gbapal` (32 bytes) fit the current
8x16 OAM size and two-tile allocation. The JSON role record beside the verified
sources records source/member details and output hashes. The targets reuse the
existing checksum-verifying source generator and are covered by the Gen 4 UI
asset CI job.

No C binding, cursor position, row spacing, bob callback, or legacy source is
changed in this phase. Next: wire this prepared sheet/palette live, validate,
then remove the superseded legacy cursor and its binding.

**authentic HGSS asset prepared → wire live pending → legacy removal pending**


## Match Call options-menu cursor — phase 2

CI #650's Gen 4 UI asset/renderer job and all ROM builds passed the source
package at `2c6bf290d28a35e1fe650100bc09dc36ed7897d6`. The general Test job
was still running when this live integration was prepared.

`AllocMatchCallSprites` now loads the verified member-057 lavender pointer
using `LoadSpriteSheet` and its authentic palette using the existing PokéNav
palette allocator. The options cursor keeps its current 8x16 OAM size, two-tile
allocation, priority, tags, (8,80) origin, 16-pixel row offsets, horizontal bob
callback, show/hide lifecycle, and resource cleanup. Trainer picture allocation
is unchanged. The Gen 4 UI compile job now includes `pokenav_match_call_gfx.o`.

The legacy compressed sheet and palette remain only as a temporary
allocation-failure fallback. Allocation success is determined by the cursor tag
lookup, because a zero return from `LoadSpriteSheet` can mean either failure or
a valid tile start. No legacy asset is removed until the live commit passes CI.

**authentic HGSS asset → wired live → legacy removal pending**


## Match Call options-menu cursor — phase 3

The cleanup is prepared against the live integration at
`8002458487910f59c64bd3cd757ccc4c34ace420`. CI #651's ROM builds and Gen 4
UI checks passed. Its general Test job was still running at preparation time;
this cleanup must remain off the active branch until that live run passes.

The cleanup removes the legacy options-cursor PNG, compressed-sheet/palette
bindings and descriptors, allocation-failure fallback, and obsolete loop
variable. `AllocMatchCallSprites` loads only the verified HGSS member-057
pointer and authentic palette. The 8x16 geometry, tags, position, row offsets,
bob callback, resource cleanup, and trainer-picture allocation remain intact.
The verified role record describes the resulting cleaned tree.

When this cleanup is applied after CI #651 succeeds, it completes:

**authentic HGSS asset → wire live → remove legacy equivalent**


## Match Call rematch badge — phase 1

This source package is prepared off the active branch while cleanup CI #652
finishes general tests. Its ROM builds and Gen 4 UI checks already passed.
Apply this package only after the cleanup run succeeds.

The existing rematch badge uses an 8x16 legacy Poké Ball plus two interpolated
palettes. Its replacement explicitly reuses authentic HGSS phone-status art
for Emerald's per-contact rematch semantics. The verified retail binding is
recorded in `verified/phone_status_indicator_skin0.json`: NANR sequence 3,
NCER cells 18/19, member-6 tiles 200/204, member-0 palette bank 3.

`make_hgss_pokegear_rematch_badge.py` validates the retail animation/cell
binding and PNG CRCs, then copies both 16x16 frames exactly. Tiles 0..3 are
active, 4..7 disabled, and tile 8 is transparent padding for absent badges.
The 288-byte tile stream and original 32-byte palette are recorded with output
hashes in `verified/rematch_badge.json` and added to the Gen 4 UI asset CI job.
No visible source pixel is scaled, redrawn, or recolored.

Native 16x16 artwork requires a two-column BG tilemap binding in the live
phase; the current one-column 8x16 rematch binding cannot load it unchanged.
Runtime predicates, contact rows, flashing code, and legacy files remain
unchanged in this source-only package.

**authentic HGSS asset prepared → wire live pending → legacy removal pending**


## Match Call rematch badge — phase 2

Prepared against source commit `8d79eb91430935b6487bf5f751f1d2f6ef413357`.
CI #653's Gen 4 UI job passed; this live change remains off the active branch
until that source run finishes successfully.

The live path loads the exact native 16x16 HGSS phone-status frames on BG3
with their original palette in bank 5. Graphics tiles 0..8 are reserved for the
two frames and transparent clearing tile. List fill tiles move to 9/10 and
its 512-tile text window starts at 11, avoiding badge/text graphics overlap.
The text window ends at tile 522, below the BG2 artwork at the address
corresponding to BG3 tile 640.

The contact list moves left from tile 13 to tile 12 while retaining its full
16-tile text width. The badge occupies columns 28/29 (pixels 224..239), so the
complete native image fits on the GBA screen without clipping the text window.
Badge drawing and clearing write all four cells of the 2x2 tile block.
The existing rematch predicate still controls presence. The alert task switches
between the unchanged retail active/disabled frames every 16 frames rather than
interpolating palette colors. It scans the 16 circular list rows, changes only
existing badge cells, and ignores cleared rows. Newly drawn rows use the
current animation phase. Existing show/hide task controls remain intact.

For this phase, the legacy source loads are retained in initialization; the
HGSS load occurs only after their decompression completes and replaces them
before the list is created. The old source files and bindings are removed only
after the live replacement passes CI. The Match Call renderer is already an
explicit target in the Gen 4 UI CI compile job.

**authentic HGSS asset → wired live → legacy removal pending**


## Match Call rematch badge — phase 3

CI #654 passed the live native-HGSS badge integration at
`99ffba706f3042b485ba4bd2b726b0af38eb1a2a`. The rollback-only Emerald rematch badge is now removed.

`src/pokenav_match_call_gfx.c` no longer binds or decompresses
`graphics/pokenav/match_call/pokeball.png`, and no longer loads
`graphics/pokenav/match_call/pokeball.pal`. The native 16x16 HGSS
phone-status frames and their authentic palette are loaded directly during
Match Call initialization; the later list-creation state no longer performs a
replacement pass.

The superseded `pokeball.png` and `pokeball.pal` source files are deleted.
Rematch predicates, 2x2 badge placement, retail-frame animation, list geometry,
and show/hide behavior remain unchanged from phase 2.

This completes the rematch-badge sequence:

**authentic HGSS asset → wire live → remove legacy equivalent**

The cleanup head must pass CI before the next UI surface begins.


## Match Call contact surface — phase 1

CI #655 passed the completed native-HGSS rematch-badge cleanup at
`b9a2e8945b4e2e6e0635b349b0e136f9d8b3d64b`, so the next authenticity surface can begin.

The remaining Match Call contact presentation is not yet acceptable as final
HGSS artwork: `src/pokenav_match_call_gfx.c` still loads the legacy Emerald
`ui.png/ui.bin` presentation and legacy call/list palettes, while
`DrawPokeGearPhonePanel`, `DrawPokeGearPhonePortraitPanel`, and
`DrawPokeGearPhoneCallPanel` construct HGSS-style chrome procedurally.

The exact retail HGSS Phone app binding has now been verified against
`pret/pokeheartgold` revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`. In
`src/application/pokegear/phone/overlay_101_021F017C.c`, skin 0 loads:

- Phone member 28 character graphics onto the contact-list BG,
- Phone member 10 main-BG palette,
- Phone member 34 screen/tilemap data for that BG.

Those three retail source files are now copied byte-for-byte into
`graphics/gen4_ui/hgss_pokegear/verified/`. Their Git blob IDs are identical
to the source repository blobs, so this phase contains no redraw, recolor,
rescale, or synthesized replacement artwork.

Nothing is wired live in this commit. The existing Emerald assets and
procedural panels remain untouched until an exact GBA adaptation of these
retail Phone resources is prepared and separately validated.

**authentic HGSS asset prepared → wire live pending → legacy removal pending**


## Match Call contact surface — phase 2

CI #656 passed the phase-1 verified retail source package at
`fe37c46c3d9d31f678367c40937f37619e7b2a38`.

The exact HGSS Phone contact screen is now wired live on Match Call BG2.
`make_hgss_pokegear_match_call_contact.py` verifies the three copied retail
files by Git blob identity before producing any output. It uses member 34's
first 20 tile rows—the exact screen rectangle copied by retail HGSS—and keeps
source columns 1..30 as the centered 240-pixel GBA viewport.

Member-28 pixel indices and member-34 flip bits are preserved exactly. Source
tile IDs are relocated into unused BG2 character space beginning at tile
`0x180`; member-10 palette bank 0 is copied without color changes into
dedicated GBA BG palette bank 6. There is no scaling, redrawing, recoloring, or
interpolation.

For rollback safety, the legacy Emerald `ui.png/ui.bin`, call/list palettes,
and the procedural HGSS-style panel helpers remain present during this phase.
Initialization first allows the old compressed BG to finish, then installs the
authentic HGSS Phone contact surface before the Match Call windows/list are
created. No legacy asset is deleted until this live path passes CI.

**authentic HGSS asset → wired live → legacy removal pending**


### Match Call contact phase-2 CI correction

CI #657 exposed a workflow-only error before the new asset build ran. The
`python3 -m py_compile` line ended after
`make_hgss_pokegear_rematch_badge.py`, so the shell invoked
`make_hgss_pokegear_match_call_contact.py` separately with no output
arguments and received its usage error.

The missing line-continuation is now restored. No generator logic, retail
source data, runtime wiring, layout, palette, or legacy Match Call resource is
changed by this correction. Phase 2 remains the active gate; phase 3 must not
start until the corrected live integration passes CI.


## Match Call contact surface — phase 3

CI #658 passed the corrected live integration at
`50ee0e60f2b9344f2b3f09e6a3861bde83e2eeea`.

The legacy Emerald static Match Call background is now removed. The
`sMatchCallUI_Pal`, `sMatchCallUI_Gfx`, and
`sMatchCallUI_Tilemap` bindings are gone, and
`graphics/pokenav/match_call/ui.png` plus
`graphics/pokenav/match_call/ui.bin` are deleted. Match Call BG2 now loads
the verified HGSS Phone member-28/member-34/member-10 adaptation directly as
its only static background path.

The old UI palette also fed the left location/info windows. To avoid retaining
that hidden dependency, those windows now use the authentic HGSS contact
palette bank and are transparent, exposing the retail Phone background instead
of drawing the synthetic `DrawPokeGearPhonePanel` chrome. That helper is
removed.

This cleanup is intentionally limited to the static contact background.
`DrawPokeGearPhoneContactRow`, `DrawPokeGearPhoneActionPad`,
`DrawPokeGearPhonePortraitPanel`, `DrawPokeGearPhoneCallPanel`,
`call_window.pal`, and `list_window.pal` remain because they belong to
dynamic list rows, options, the trainer-check page, and call-message UI. The
retail member-34 background does not directly replace those surfaces; each
must go through its own authenticity sequence.

This completes the static contact-background sequence:

**authentic HGSS asset → wire live → remove legacy equivalent**


## Match Call dynamic contact rows — phase 1

CI #659 passed the completed static contact-background cleanup at
`fd91b629d01cde90fc8b940101268bfdb18ab818`.

The next surface is the dynamic contact list drawn over BG3. Retail HGSS does
not use a painted approximation for these rows: its Phone code draws each row
programmatically with exact geometry and palette indices. The authoritative
sources at the locked HGSS revision are:

- `overlay_101_021F017C.c` blob
  `320be2869ebb277fe85bac6021bf01b50bfc8f7a`: contact-list window is
  MAIN_3, 27 tiles wide by 24 tiles high, palette bank 2.
- `overlay_101_021F0F48.c` blob
  `e303f58126397a8f28b0a4dd496bbea3d2c12c88`: exact row rectangles,
  alternating color sets, text-color assignments, and six-contact paging.
- Verified member-10 NCLR blob
  `85b29945c3b9e8250b119922d64111ded55b1563`: the row window's exact
  retail palette source.

Retail rows are 216×24 pixels, alternate two documented color sets, maintain
eight buffered rows for scrolling, and display six contacts. The current GBA
PokéNav list path is hard-wired to 16-pixel rows (`row << 4`) and currently
shows eight contacts, so silently squeezing HGSS into that cadence would not
be authentic.

`make_hgss_pokegear_contact_rows.py` now verifies the retail member-10 blob
and extracts palette bank 2 byte-for-byte into
`match_call_contact_rows.gbapal`. The exact row geometry and palette-index
mapping are recorded in `verified/match_call_contact_rows.json`.

Nothing is wired live in this phase. The existing
`DrawPokeGearPhoneContactRow` remains untouched until a Match Call-specific
24-pixel list path is implemented and passes CI.

**authentic HGSS row source prepared → wire live pending → legacy removal pending**


## Match Call dynamic contact rows — phase 2

CI #660 passed phase 1 at `4b4cf4f65bba9f6220dae2f328db4f33a73a6c07`.

The live Match Call list now opts into a dedicated HGSS Phone-row path rather
than changing the generic PokéNav list behavior. Other PokéNav lists still use
the existing 16-pixel `CreatePokenavList` path unchanged.

The HGSS Phone path uses the retail buffering model: eight 24-pixel row slots,
six visible contacts in slots 1–6, one row buffered above and below, and
`ScrollWindow` in three 8-pixel steps for each one-row move. This mirrors the
retail `PhoneContactListUI_ScrollStep` behavior and avoids forcing 24-pixel
rows into the GBA BG's old 16-pixel ring.

Rows use the exact member-10 palette-bank-2 indices documented in phase 1,
loaded into dedicated GBA BG palette bank 7. The exact
`PhoneContactListUI_DrawNameSlotBG` rectangles are used. Because the GBA
Match Call contact pane remains 128 pixels wide while the retail row is 216
pixels, the runtime keeps source x=0..127 as a direct crop; coordinates,
palette indices, and pixel dimensions are not scaled or recolored.

The list now displays six contacts at a 24-pixel cadence. Existing rematch
badge tiles were adjusted to the three-tile-row stride and are hidden during
the three scroll steps, then redrawn in their correct rows after the movement.

The old synthetic `DrawPokeGearPhoneContactRowLegacy` remains compiled only
as a phase-2 safety fallback for an unexpected contact window narrower than
91 pixels. It is not removed until this live integration passes CI.

**authentic HGSS row source ✅ → wired live ✅ → legacy removal pending**


### Match Call contact-row phase-2 CI correction

CI #661 reached the shared C build and exposed three compile-only issues in
the phase-2 integration:

- `src/pokenav_list.c` used `MAX`, which is not available in that
  translation unit.
- `src/pokenav_match_call_gfx.c` used `MIN`, likewise unavailable there.
- `InitPokenavListWindow` became unused after the new initialization path was
  inlined, and the project's `-Werror` policy rejected it.

The correction replaces `MIN/MAX` with explicit bounds checks and removes
only the now-unused helper. It does not change the verified HGSS palette,
24-pixel geometry, six-contact viewport, eight-row buffering model,
three-step scrolling design, or fallback policy.

Phase 2 remains active. No legacy contact-row asset or renderer is removed
until the corrected integration passes CI.


## Match Call dynamic contact rows — phase 3

CI #662 passed the corrected live 24-pixel HGSS Phone-row integration at
`a0a2d4469e5c6a7dc72c2f66d58d4ce4f7aa7540`.

The phase-2 safety fallback is now removed. The synthetic
`DrawPokeGearPhoneContactRowLegacy` renderer and its narrow-window fallback
branch are gone, leaving `DrawHgssPhoneContactRow` as the sole dynamic
Match Call contact-row renderer.

The old Emerald `list_window.pal` dependency is also removed. Its
`sListWindow_Pal` binding and BG palette-bank-3 load were no longer used by
the live contact window, which explicitly uses the verified HGSS member-10
palette bank 2 mapped to GBA BG palette bank 7. The file
`graphics/pokenav/match_call/list_window.pal` is deleted.

This cleanup is intentionally limited to the dynamic contact rows. The
procedural action/options pad, trainer check-page chrome, and call-message
panel remain separate pending surfaces and are not treated as equivalents of
the retail contact-row renderer.

This completes the dynamic contact-row sequence:

**authentic HGSS row source ✅ → wired live ✅ → legacy equivalent removed ✅**


## Match Call action/options menu — phase 1

CI #663 passed the completed dynamic-contact-row cleanup at
`32fd8cba314ea86b0a9fc615f8c5ce41c80c9152`.

The retail audit found a direct HGSS visual counterpart for the current
Match Call action pad. Selecting a Phone contact in HGSS opens context-menu
ID 0, defined in `overlay_101_021F017C.c` as a three-item
**Call / Sort / Quit** menu with width 16 tiles at retail origin (13, 9).
`overlay_101_021F0880.c` instantiates that menu through the shared
`TouchscreenListMenu` renderer.

The menu chrome is not part of the Phone background. The renderer loads
HGSS's shared `data/sbox_gra` resource. Its retail source
`files/data/sbox_gra/sbox_gra.png` is now copied byte-for-byte to
`verified/sbox_gra.png` (Git blob
`177f5c242ba0eacc1f74dead410c526bc5a46267`, SHA-256
`5cd355b6456624d9657def9d65b190ff2bedba789e7e44606f1df4affb6d8643`).

The source is a 24×72, 4-bit indexed sheet: exactly 27 8×8 tiles. Retail
`TouchscreenListMenu_DrawButtons` uses all 27 tiles as the normal and
selected top, side, separator, and bottom pieces. The new generator verifies
the source identity and emits every tile unchanged in row-major GBA 4bpp form,
plus the complete 16-color source palette in source order. Output checksums are
recorded in `verified/match_call_action_menu.json`.

There is one intentional functional distinction. Retail Phone's labels/actions
are Call / Sort / Quit, while this project's existing Match Call behavior is
CALL / CHECK / CANCEL (or CALL / CANCEL when no check page exists). Phase 2
will preserve those game semantics while replacing their visual container with
the authentic HGSS context-menu chrome; this is a direct visual migration, not
a claim that CHECK is retail SORT or that CANCEL is retail QUIT.

The complete three-item retail menu is 18×10 tiles (144×80 px including its
one-tile side borders). The planned GBA adaptation is translation only:
retail origin (13, 9) becomes (12, 9), moving the complete menu eight pixels
left so it fits exactly within the 240-pixel screen. No scaling, redrawing,
recoloring, or geometry resampling is planned.

Nothing is wired live in this phase. `DrawPokeGearPhoneActionPad`, its
current labels, and the existing cursor path remain untouched until the new
HGSS menu assets pass CI and are wired in phase 2.

**authentic HGSS action-menu source prepared → wire live pending → legacy removal pending**


## Match Call action/options menu — phase 2

CI #664 passed the source-preparation commit `6b326bec` in full.
`DrawHgssPhoneActionMenu` now renders the verified sbox_gra tiles live on BG1.
The complete 18x10-tile allocation begins at (12, 9), preserving the planned
one-tile translation, 16-tile option interiors and 24-pixel row cadence.
Two-option contacts render the retail two-row grammar with a transparent tail.

BG1 character tiles 256–435 are separate from the message window (10–121)
and its border (1–8). BG palette bank 8 carries the unchanged source palette;
contact, row, badge and message palettes remain in their existing banks.
The selected top, bottom, side and both separator variants follow
`TouchscreenListMenu_DrawButtons` from retail source blob
`a74013e670f964ca994bc822d57577731f6a61c0`. Text uses source indices
1/2/3 normally and 4/5/6 when selected, reordered only for Emerald's
background/foreground/shadow API. Labels retain the current game font and
CALL/CHECK/CANCEL semantics.

Selection changes redraw the menu and wait for DMA completion. CALL,
nearby-trainer messages, CHECK, CANCEL and teardown clear and free the overlay;
repeated close calls are safe. The old pointer is no longer created. Its assets,
helpers and the old procedural action-pad function remain for phase 3, with
unused entry points explicitly marked pending successful integration CI.

Local validation: generated tiles and palette pass their identity/checksum
guards. A host-compiled harness of the actual C draw functions passed all five
selection states across two- and three-option menus, checking retail border
indices, fill/text palette indices, screen bounds and the transparent tail.
`git diff --check` passed. Full ARM compilation and ROM checks are delegated to
CI because the local workspace has no ARM compiler. Runtime emulator visual
validation has not yet been performed.

**authentic HGSS source validated → wired live (CI pending) → legacy removal pending**


## Match Call action/options menu — phase 3

CI #665 passed the live wiring at `af57be65` in full, including General Test.
The procedural `DrawPokeGearPhoneActionPad` and superseded pointer sprite
resources, helpers, allocation and release paths are now removed. The pointer's
Match Call build rules, CI targets and obsolete role manifest are removed too.
Shared HGSS Pokédex pointer source/generators remain in use by other surfaces.
Earlier pointer sections above are historical records, not active bindings.

The selected sbox_gra borders are the sole action-menu selection indicator.
The DMA wait helper is now named `WaitForActionMenu`. CALL/CHECK/CANCEL behavior,
retail geometry, palette and the live renderer are unchanged from phase 2.

Validation: the actual C draw-function harness still passes all five selection
states; no obsolete pad/pointer symbols remain in the runtime or build paths;
`git diff --check` passes. Full cleanup integration CI is pending.

**authentic HGSS asset → wired live and validated → legacy equivalent removed**


## Match Call call-message surface — phase 1

CI #666 passed the completed Match Call action/options-menu cleanup at
`8514943660616c94a667469a15ddffdc5df58f32`, so the next direct retail
Phone surface can begin.

The remaining call-message presentation is still not acceptable as final HGSS
artwork. `src/pokenav_match_call_gfx.c` draws
`DrawPokeGearPhoneCallPanel` procedurally and still depends on
`graphics/pokenav/match_call/call_window.pal`. Those remain live in this
phase; nothing is replaced before the retail source is verified.

Retail HeartGold/SoulSilver already has a dedicated Phone call-screen source
set. At the locked `pret/pokeheartgold` revision
`9d8b7591f09b65804da2fb2dfd56f320633e0d36`,
`src/application/pokegear/phone/overlay_101_021F017C.c` loads, for skin 0:

- member 16 character graphics onto `GF_BG_LYR_SUB_3`,
- member 22 NSCR screen/tilemap data onto that same layer,
- member 4 NCLR into the SUB BG palette.

The same retail module defines the Phone's two SUB_2 call/text windows, and
`overlay_101_021F1D74.c` binds the first of those windows as
`phoneCallMsgWindow` for the live Phone conversation text. This makes the
member-16/member-22/member-4 set a direct retail source for the call-message
screen rather than an invented HGSS-style analogue.

The exact retail files are now copied byte-for-byte into `verified/`:

- `pgphone_skin0_call_tiles.png` — source blob
  `f1c86ec62e6764d295678d2714de8d942253160c`,
- `pgphone_skin0_call_tilemap.NSCR` — source blob
  `46641999f72d5f1b7348a1042a7e9b8b02be766c`,
- `pgphone_skin0_call_palette.NCLR` — source blob
  `932d99a855a27912da0f074acf3b42642a2c4254`.

No pixel, palette entry, tile index, or geometry is changed in this phase.
The existing GBA call-message panel, message-window behavior, text semantics,
and legacy palette remain untouched. Phase 2 will adapt and wire the verified
retail call-screen source while keeping the current panel available only as a
rollback path until CI validates the integration.

**authentic HGSS asset prepared → wire live pending → legacy removal pending**


## Match Call call-message surface — phase 2

CI #667 completed successfully for phase-1 commit
`42f11a993c94cc78779c7a278859affd2ef9a406`, clearing the source-preparation
gate.

The verified retail HGSS Phone member-16/member-22/member-4 set is now adapted
and wired into the live Match Call call-message path.

The GBA adaptation does not redraw, scale, recolor, or synthesize any source
artwork:

- member 22 is 32x24 tiles (256x192); source rows 0..3 are verified blank and
  are omitted, preserving source rows 4..23 as the GBA's 20 visible rows;
- the retail 27x4 Phone conversation window occupies source x=2..28 and
  y=19..22; after the blank-row omission its live GBA position is exactly
  x=2, y=15 with the same 27x4 dimensions;
- source columns 29 and 30 are verified repeated filler on every nonblank
  retained row, so only those two filler columns are omitted; source columns
  0..28 and the unique right edge at column 31 are preserved;
- source tile indices 0..19, H/V flip bits, and palette-bank-0 pixel indices
  are preserved; the only GBA-side remapping is to BG1 tile base 0x80 and
  palette bank 9.

`tools/gen4_ui/make_hgss_pokegear_match_call_call.py` verifies all three
retail Git blob identities before producing
`match_call_call.4bpp`, `match_call_call.tilemap.bin`, and
`match_call_call.gbapal`. CI's Gen-4 UI job now syntax-checks the generator
and builds all three outputs.

`DrawMsgBoxForMatchCallMsg` and `DrawMsgBoxForCloseByMsg` now enter
`DrawHgssPhoneCallSurface`, which loads the authentic member-16 graphics,
adapted member-22 tilemap, and member-4 palette on BG1, then overlays the
27x4 live text window at the retail-derived coordinates. Retail HGSS itself
fills that Phone text window with pixel index 0 before printing; the live path
now mirrors that behavior.

The previous `DrawPokeGearPhoneCallPanel`, original 28x4 window template, and
`graphics/pokenav/match_call/call_window.pal` are intentionally retained
only behind the phase-2 rollback switch until CI validates this integration.
They are no longer the default live call-message surface.

**authentic HGSS asset → wired live, CI pending → legacy removal pending**
