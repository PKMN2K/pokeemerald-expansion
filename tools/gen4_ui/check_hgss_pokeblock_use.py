#!/usr/bin/env python3
"""Check source provenance, feeding VRAM allocation and retired asset bindings."""
import re
from pathlib import Path

from make_hgss_pokeblock_use import OUT, validate
from make_hgss_condition_graph import OUT as GRAPH, validate as validate_graph
from make_hgss_pokegear_list_arrows import build_tiles, build_palette
from check_hgss_condition_graph import fields

ROOT = Path(__file__).resolve().parents[2]


def check():
    validate(); validate_graph()
    arrows = ROOT / 'graphics/gen4_ui/hgss_pokegear'
    assert (arrows/'list_arrows.4bpp').read_bytes() == build_tiles()
    assert (arrows/'list_arrows.gbapal').read_bytes() == build_palette()
    source = (ROOT/'src/use_pokeblock.c').read_text()
    bg_block = re.search(r'sBgTemplates\[\].*?\n};',source,re.S)[0]
    bgs = {e['bg']:e for e in (fields(b) for b in re.findall(r'\{([^{}]+)\}',bg_block))}
    assert [bgs[i]['charBaseIndex'] for i in range(3)] == [1,2,3]
    assert [bgs[i]['priority'] for i in range(3)] == [0,3,2]
    regions = [('transparent text tile',0x4000,0x4020),
               ('HGSS background',0x8000,0x8000+(GRAPH/'graph_bg.4bpp').stat().st_size),
               ('data fill',0xC020,0xC040)]
    for bg,e in bgs.items():
        regions.append((f'BG{bg} map',e['mapBaseIndex']*2048,(e['mapBaseIndex']+1)*2048))
    windows = re.search(r'sWindowTemplates\[.*?\n};',source,re.S)[0]
    for name,block in re.findall(r'\[(WIN_\w+)\]\s*=\s*\{([^{}]+)\}',windows):
        e = fields(block)
        assert e['bg'] == 0 and e['paletteNum'] == 1, name
        assert e['tilemapLeft']+e['width'] <= 30 and e['tilemapTop']+e['height'] <= 20, name
        start = 0x4000+e['baseBlock']*32
        regions.append((name,start,start+e['width']*e['height']*32))
        if name in ('WIN_TEXT','WIN_CONFIRM'):
            asset = 'footer.4bpp' if name == 'WIN_TEXT' else 'confirmation.4bpp'
            assert (OUT/asset).stat().st_size == e['width']*e['height']*32
    regions.sort(key=lambda r:r[1])
    for left,right in zip(regions,regions[1:]):
        assert left[2] <= right[1], f'VRAM collision: {left[0]} / {right[0]}'
    assert regions[-1][2] <= 0x10000
    for asset in ('hgss_condition/graph_bg.4bpp','hgss_condition/graph_bg.bin',
                  'hgss_pokeblock_use/footer.4bpp','hgss_pokeblock_use/confirmation.4bpp',
                  'hgss_pokegear/list_arrows.4bpp','hgss_pokegear/list_arrows.gbapal'):
        assert f'"graphics/gen4_ui/{asset}"' in source, asset
    for legacy in ('gUsePokeblock','LoadConditionSelectionIcons','LoadConditionSparkle(',
                   'graphics/pokeblock/use_screen/','graphics/pokenav/condition/',
                   'DrawTextBorderOuter','CreateYesNoMenu'):
        assert legacy not in source, legacy
    assert not list((ROOT/'graphics/pokeblock/use_screen').glob('*'))
    for name in ('cancel.pal','cancel.png','graph_data.pal','pokeball.png','pokeball_placeholder.png','text.pal'):
        assert not (ROOT/'graphics/pokenav/condition'/name).exists(), name
    shared = (ROOT/'src/menu_specialized.c').read_text()
    assert 'LoadConditionSelectionIcons' not in shared
    assert 'LoadHgssConditionSparkle' in source and 'CreateHgssConditionSparkleSprites' in source
    # The separate SWSH summary still owns its generic sparkle; do not delete it.
    assert 'LoadConditionSparkle(' in (ROOT/'src/swsh_summary_screen.c').read_text()
    assert (ROOT/'graphics/pokenav/condition/sparkle.png').exists()
    print('Verified HGSS feeding panels, arrows, shared sheen, disjoint VRAM and complete legacy removal')


if __name__ == '__main__':
    check()
