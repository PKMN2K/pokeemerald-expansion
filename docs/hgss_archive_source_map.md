# HGSS source archive mapping for the Emerald UI conversion

Source repository: `PKMN2K/pokeheartgold-slop`, branch `mainline`.
Verified mapping location: `filesystem.mk`. This document identifies archive paths, **not verified extracted PNGs**.

| HGSS source archive | Native file location | Intended adaptation |
| --- | --- | --- |
| `files/application/pokegear/pgear_gra.narc` | `files/a/1/4/3` | Pokégear shell |
| `files/application/pokegear/map/pgmap_gra.narc` | `files/a/1/4/4` | Pokégear map |
| `files/application/pokegear/phone/pgphone_gra.narc` | `files/a/1/4/6` | Pokégear phone |
| `files/application/pokegear/radio/pgradio_gra.narc` | `files/a/1/4/7` | Pokégear radio |
| `files/graphic/plist_gra.narc` | `files/a/0/2/1` | Candidate Pokémon list graphics; **not validated for ribbons** |

**Important:** There is no proven native Pokégear ribbon app in these mappings. The ongoing ribbon app is a custom Emerald adaptation. It is inaccurate to call generated ribbon panels “original HGSS ribbon artwork.” For each converted element, identify its actual original screen and archive or explicitly designate it custom.

## Asset acceptance path

1. Extract each candidate archive from a lawfully obtained HGSS ROM/source worktree.
2. Identify graphics and palettes inside the archive (NCGR/NCLR and accompanying layouts as appropriate).
3. Record the exact archive member and source-screen identity; verify visually against original HGSS screenshots.
4. Convert extracted source to a PNG with suitable GBA palette/tile constraints; retain unmodified source evidence and import manifest.
5. Run `tools/validate_hgss_ribbon_asset.py` only after valid PNGs and actual checksums exist. That script checks manifest consistency, **not whether a claimed source is genuinely HGSS**.
6. Integrate the asset in live code, build, visually test, then remove the corresponding legacy artwork.

No ribbon artwork has been imported or authenticated by this step.
