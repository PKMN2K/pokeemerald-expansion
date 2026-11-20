#!/usr/bin/env python3
"""Import pinned hit-check bytes, guarantees and move priority; record their scope."""
import argparse
import json
import re

import build_battle_suite as archive

ROOT = archive.ROOT


def build():
    files = {p: r['text'] for p, r in archive.authenticated_sources().items()}
    source = archive.source.sources()
    rows = [[v.strip() for v in line[5:].split(',')] for line in
            archive.source.default_lines(source['data/moves/moves.asm'].split('Moves::', 1)[1]) if line.startswith('move ')]
    identities = archive.source.move_identities(source)[1:]
    priorities = dict((name, int(value)) for name, value in
                      re.findall(r'\bdb ([A-Z_]+),\s*(-?\d+)', files['data/moves/priorities.asm']))
    assert priorities.pop('NO_MOVE') == 10
    commands = files['engine/battle/effect_commands.asm']
    hit = commands.split('BattleCommand_checkhit:', 1)[1].split('BattleCommand_checkpriority:', 1)[0]
    assert hit.index('call .NoGuardCheck') < hit.index('call .FlyDigMoves') < hit.index('call .LockOn') < hit.index('call .WeatherAccCheck')
    assert hit.index('farcall ApplyAccuracyAbilities') < hit.index('; Check user items') < hit.index('; Check opponent items')
    assert 'call CheckOpponentWentFirst\n\tjr z, .done_user_items' in hit
    assert 'cp 7\n\tjr c, .check_opponent_unaware' in hit
    guaranteed = re.findall(r'cp (EFFECT_\w+)\n\tret z', hit.split('; Perfect-accuracy moves', 1)[1].split('; Immunity', 1)[0])
    assert guaranteed == ['EFFECT_ALWAYS_HIT', 'EFFECT_ROAR', 'EFFECT_COUNTER']
    assert 'DEF percent EQUS "* $ff / 100"' in files['macros/data.asm']
    thunder = files['engine/battle/move_effects/thunder.asm']
    assert 'call GetWeatherAfterOpponentUmbrella' in thunder
    assert 'ld [hl], 50 percent + 1' in thunder and 'ld [hl], 100 percent' in thunder
    sunny_accuracy = 50 * 255 // 100 + 1
    assert sunny_accuracy == 128
    accuracy_abilities = files['engine/battle/abilities.asm'].split('ApplyAccuracyAbilities:', 1)[1].split('RunWeatherAbilities:', 1)[0]
    assert 'dbw SNOW_CLOAK, SnowCloakAbility' in accuracy_abilities
    assert 'ld [hl], 50' in accuracy_abilities.split('WonderSkinAbility:', 1)[1]
    items = files['data/items/attributes.asm']
    assert 'HELD_ACCURACY_BOOST, $ba' in items and 'HELD_ZOOM_LENS, $65' in items and 'HELD_BRIGHTPOWDER, $9a' in items
    speed = files['home/battle.asm'].split('CheckMoveSpeed::', 1)[1].split('CheckSpeed::', 1)[0]
    assert speed.index('cp QUICK_DRAW') < speed.index('cp HELD_QUICK_CLAW') < speed.index('cp HELD_CUSTAP_BERRY') < speed.index('cp HELD_LAGGING_TAIL')
    determine = files['engine/battle/core.asm'].split('DetermineMoveOrder:', 1)[1].split('GetSpeed::', 1)[0]
    assert 'call CompareMovePriority\n\tjr z, .equal_priority' in determine and '.equal_priority\n\tcall CheckMoveSpeed' in determine
    records = []
    for identity, row in zip(identities, rows, strict=True):
        name, effect, power, type_, accuracy, pp, chance, category = row
        assert identity['source_move'] == name
        records.append(dict(move=identity['target_move'], accuracy=int(accuracy) & 255,
                            bypass=effect in guaranteed or name == 'STRUGGLE', priority=priorities.get(name, 0)))
    assert len(records) == 255
    banner = '/* Generated from pinned default Polished Crystal by build_accuracy_order.py. */'
    accuracy = [banner, 'static const struct { u8 accuracy; bool8 bypass; } sPolishedAccuracyRules[MOVES_COUNT] = {']
    accuracy += [f'    [{r["move"]}] = {{{r["accuracy"]}, {"TRUE" if r["bypass"] else "FALSE"}}},' for r in records]
    accuracy += ['};', f'#define POLISHED_SUN_THUNDER_ACCURACY {sunny_accuracy}', '']
    priority = [banner, 'static const s8 sPolishedMovePriorities[MOVES_COUNT] = {']
    priority += [f'    [{r["move"]}] = {r["priority"]},' for r in records if r['priority']]
    priority += ['};', '']
    # Independent expected stage outputs for all source raw accuracy values.
    vectors = [banner, 'static const struct { u8 accuracy, attackStage, defenseStage; u16 result; } sPolishedAccuracyVectors[] = {']
    for raw in sorted({r['accuracy'] for r in records} | {sunny_accuracy}):
        for atk in range(13):
            for defense in range(13):
                stage = max(-6, min(6, atk - defense))
                result = raw * (3 + max(0, stage)) // (3 + max(0, -stage))
                vectors.append(f'    {{{raw}, {atk}, {defense}, {result}}},')
    count = len(vectors) - 2
    vectors += ['};', 'static const struct { u16 move; u8 accuracy; s8 priority; bool8 bypass; } sPolishedAccuracyMoves[] = {']
    vectors += [f'    {{{r["move"]}, {r["accuracy"]}, {r["priority"]}, {"TRUE" if r["bypass"] else "FALSE"}}},' for r in records]
    vectors += ['};', '']
    manifest = dict(source_commit=archive.source.PIN, source_variant='default release, FAITHFUL undefined',
        moves=records, stage_vectors=count, sunny_thunder_hurricane_raw_accuracy=sunny_accuracy,
        rules=['Exact combined stages: clamp accuracy minus evasion to -6..6, then multiply by the 3-based rational with flooring.',
               'Keen Eye, Illuminate, Minds Eye and identification ignore only positive evasion; Unaware ignores all opposing stages.',
               'Wonder Skin sets checked status accuracy to 50. User ability, target ability, user item and target item apply in that order.',
               'Thunder/Hurricane weather and guaranteed rain hits use target Utility Umbrella. The pinned sun byte is 128, not 50.',
               'Snow Cloak and Sand Veil use target weather. Zoom Lens follows selected action order, including a prior switch.',
               'Raw 255 is scaled normally unless the effect explicitly bypasses checkhit; it is not itself a guaranteed-hit flag.',
               'Fly/Dig precede Lock-On but follow No Guard and Poison-type Toxic.',
               'Quick Draw/Claw and Custap are considered only when selected action priorities match; Quick Draw overrides Lagging Tail and Unnerve blocks Custap.'],
        source_paths=['engine/battle/effect_commands.asm','engine/battle/abilities.asm','engine/battle/core.asm','engine/battle/move_effects/thunder.asm','home/battle.asm','home/random.asm','data/moves/priorities.asm','data/moves/moves.asm','data/items/attributes.asm','macros/data.asm'],
        limits=['Scoped execution evidence, not complete action, accuracy, status or move-effect parity.',
                'Exact RNG stream/call count, Lock-On lifetime/consumption, external Future Sight, optional affection, obedience and complete cancellation order remain open.',
                'Native AI accuracy estimates remain separate. Unmapped moves retain prior accuracy/priority rules; shared quick-item rules apply throughout ordinary source singles. Doubles and active gimmicks retain native behavior.',
                'Global damage/residual/faint order, capture/rewards, facility/link and all other whole-battle domain obligations remain open.'])
    return {ROOT/'src/data/polished_accuracy.h': '\n'.join(accuracy),
            ROOT/'src/data/polished_priorities.h': '\n'.join(priority),
            ROOT/'test/battle/polished_accuracy_vectors.h': '\n'.join(vectors),
            ROOT/'tools/polished_port/manifests/accuracy_order.json': json.dumps(manifest, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: source accuracy bytes, exact stage vectors, modifiers, weather, hit exceptions and move priorities')
