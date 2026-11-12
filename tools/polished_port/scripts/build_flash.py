#!/usr/bin/env python3
"""Verify the pinned Flash contract and generate its field text/provenance."""
import argparse
import base64
import hashlib
import json
import re
from build_rock_smash import ROOT, sources as rock_sources
from build_new_bark import PIN


def sources():
    result = rock_sources()
    folder = ROOT / 'tools/polished_port/fixtures/flash'
    manifest = json.loads((folder / 'sources.json').read_text())
    assert manifest['commit'] == PIN
    for entry in manifest['files']:
        data = base64.b64decode((folder / entry['fixture']).read_text())
        assert hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == entry['git_blob_sha1']
        result[entry['path']] = data
    return result


def build():
    f = {p:b.decode() for p,b in sources().items() if p.endswith('.asm')}
    flow = f['engine/events/overworld.asm']
    prompt = flow.split('TryFlashOW::', 1)[1].split('OWFlash:', 1)[0]
    assert 'cp DARKNESS_PALSET' in prompt and 'lb de, FLASH, TM_FLASH' in prompt
    assert 'call CheckPartyMove' in prompt and 'yesorno' in prompt
    use = flow.split('OWFlash:', 1)[1].split('UseFlashTextScript:', 1)[0]
    assert 'CheckBadge' not in use and 'cp DARKNESS_PALSET' in use
    assert 'SpecialAerodactylChamber' in use  # Explicitly deferred unported map.
    assert use.index('FieldMovePokepicScript') < use.index('callasm BlindingFlash')
    party = flow.split('CheckPartyMove:', 1)[1].split('Has', 1)[0]
    assert party.index('.loop1') < party.index('CheckTMHM') < party.index('.loop2')
    assert 'MON_IS_EGG' in party and 'MON_HP' not in party and 'MON_PP' not in party
    light = f['engine/events/field_moves.asm'].split('BlindingFlash::', 1)[1].split('ShakeHeadbuttTree:', 1)[0]
    assert light.index('FadeOutPalettes') < light.index('set 2, [hl]') < light.index('FadeInPalettes')
    collision = f['engine/overworld/events.asm'].split('TryTileCollisionEvent:', 1)[1].split('RandomEncounter::', 1)[0]
    assert collision.index('CheckFacingTileForStdScript') < collision.index('TrySurfOW') < collision.index('TryFlashOW')
    reset = f['engine/overworld/warp_connection.asm'].split('ResetOWMapState:', 1)[1].split('EnterMapConnection:', 1)[0]
    assert 'cp ROUTE' in reset and 'cp TOWN' in reset and 'res 2, [hl]' in reset
    output = ['@ Generated from pinned Polished Crystal field text.']
    for source, target in [('AskFlash', 'Ask'), ('BlindingFlash', 'Used')]:
        body = f['data/text/common.asm'].split('_' + source + 'Text::', 1)[1].split('SECTION', 1)[0]
        text = ''.join({'text':'', 'line':'\\n', 'para':'\\p', 'cont':'\\l'}[op] + value for op,value in re.findall(r'^\s*(text|line|para|cont) "(.*)"', body, re.M))
        output += ['PolishedFlash_' + target + 'Text:', '    .string "' + text + '$"']
    report = dict(source_commit=PIN, source_variant='default release',
        source_map='DARK_CAVE_VIOLET_ENTRANCE', target_map='MAP_POLISHED_DARK_CAVE_VIOLET_ENTRANCE',
        source_move='FLASH', target_move='MOVE_FLASH', source_item='TM70', target_item='ITEM_TM70', legacy_item='ITEM_HM_FLASH',
        rules=['A-button fallback after objects, backgrounds, metatiles and Surf.',
               'First non-Egg knower; otherwise first source-compatible non-Egg if the Flash machine is owned.',
               'No HP, PP or badge gate. No teaching, item consumption or PP deduction.',
               'A-button confirmation can be declined; party menu requires the selected Pokemon to know Flash and skips confirmation.',
               'White fade then full-map lighting. Battle/menu reload and Save/Continue preserve illumination; outdoors resets it.'],
        gaps=['Full source TM/HM inventory remains incomplete; TM70 is imported separately.',
              'Darkness before use, Pokemon picture/cry, Flash sound, message styling and exact fade timing retain native presentation.',
              'The Aerodactyl chamber exception is deferred until its map is ported.'])
    return {ROOT/'data/text/polished_flash.inc': ('\n'.join(output)+'\n').encode(),
            ROOT/'tools/polished_port/manifests/flash.json': (json.dumps(report,indent=2)+'\n').encode()}


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    for path,data in build().items():
        if args.check: assert path.read_bytes()==data, f'Regenerate {path}'
        else: path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    print('PASS: pinned Flash eligibility, script priority, lighting/reset contract and text')


if __name__=='__main__': main()
