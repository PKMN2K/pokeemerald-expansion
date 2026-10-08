# HGSS UI asset authenticity audit — ribbon inspection

Status: **requires authentic-source replacement** (2026-10-07).

## Canonical conversion rule

1. Identify and preserve the original HeartGold/SoulSilver source asset, its palette, dimensions, and origin.
2. Import the authentic graphic via the existing project conversion pipeline.
3. Wire the imported asset into the live Emerald screen and verify rendering.
4. Only after successful verification, remove the corresponding Emerald legacy asset and any temporary drawn imitation.

Do **not** label manually generated panels, gradients, rectangles, or recolors as authentic HGSS assets. Do not delete working graphics to satisfy the conversion checklist before an authentic replacement is operational.

## Current ribbon screen findings

Audited from current `master` at commit `0327ae05ad4b033654a327d2c789324a8f32fc6b`.

| Screen | Current implementation | Authenticity | Required next action |
| --- | --- | --- | --- |
| Ribbon summary grid | `src/pokenav_ribbons_summary.c:DrawPokeGearRibbonGridFrame` draws a custom bezel and slots via `FillWindowPixelRect` | **Not authentic artwork** | Locate HGSS-native ribbon case/background art, import its tiles and palette, then render behind ribbon icons |
| Ribbon selection focus | `DrawPokeGearRibbonSelectionFocus` draws custom gold corner brackets and cyan tick | **Not authentic artwork** | Extract original HGSS selection indicator; replace drawing with its native converted sprite/tiles |
| Ribbon detail and count cards | `DrawPokeGearRibbonDetailPanel`, `DrawPokeGearRibbonMonCard`, `DrawPokeGearRibbonIndexCard` paint cards programmatically | **Not authentic artwork** | Match each widget to an original HGSS source, or mark as a custom adaptation rather than authentic |
| Ribbon owner list | `src/pokenav_ribbons_list.c:DrawPokeGearRibbonOwnerRow` and `DrawPokeGearRibbonCountCard` paint rectangles | **Not authentic artwork** | Import native graphics and swap the live drawing path after validation |
| Ribbon summary background | `graphics/pokenav/ribbons/summary_bg.png` modified in previous commit | **Provenance unverified** | Compare against a source HGSS extract and record origin before asserting authenticity |

## Acceptance criteria for the next asset conversion

- Asset source and exact source path/asset reference documented.
- Original palette and sprite/tile geometry preserved where GBA display constraints permit; any necessary conversion is identified.
- Gameplay remains functional: ribbon selection, zoom-in/zoom-out, owner list, and return navigation.
- Newly imported asset is referenced by runtime code before deleting any previous graphic.
- CI build succeeds; verify visual correctness separately in an emulator or screenshots (CI compilation alone cannot prove pixel fidelity).
- No undocumented hand-drawn `HGSS-style` art is introduced as a substitute.

## Blocker

An authenticated ribbon-screen graphics extract has not yet been identified in the audited files. **Do not invent one or repurpose an unrelated asset and call it native.** The live interface should remain working while authentic sourcing is resolved.
