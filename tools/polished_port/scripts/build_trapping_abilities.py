#!/usr/bin/env python3
"""Verify pinned mutual trapping rules and generate type/ability test vectors."""
import argparse
import itertools
import json

import build_pokemon_learnsets as source


def build():
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    trap = commands.split('\nCheckIfTrappedByAbility:', 1)[1].split('\nSetBattleDraw:', 1)[0]
    assert 'call .CheckTrap\n\tret nz\n\tcall .CheckOpponentTrap\n\tjr z, .not_trapped' in trap
    assert '.trapped\n\txor a\n\tret\n.not_trapped\n\tor 1\n\tret' in trap
    assert '.CheckOpponentTrap:\n\tcall StackCallOpponentTurn\n.CheckTrap:' in trap
    gates = ['call CheckIfUserIsGhostType', 'call GetOpponentAbility', 'cp MAGNET_PULL', 'cp ARENA_TRAP', 'cp SHADOW_TAG']
    offsets = [trap.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    assert 'jmp CheckIfUserIsSteelType' in trap
    assert 'ld d, 0\n\tcall CheckAirborne\n\tjr nz, .not_trapped' in trap
    assert 'GetTrueUserAbility' not in trap  # No blanket same-ability exemption.
    airborne = commands.split('\nCheckAirborne:', 1)[1].split('\nBattleCheckTypeMatchup:', 1)[0]
    assert 'call GetTrueUserAbility' in airborne
    gates = ['sub HELD_IRON_BALL\n\tret z', 'call CheckIfUserIsFlyingType', 'cp HELD_AIR_BALLOON', 'cp LEVITATE']
    offsets = [airborne.index(gate) for gate in gates]
    assert offsets == sorted(offsets)
    turn = files['home/battle.asm'].split('\nStackCallOpponentTurn::', 1)[1].split('\nSetPlayerTurn::', 1)[0]
    assert 'ld [hl], HIGH(SwitchTurn)' in turn and 'ld [hl], LOW(SwitchTurn)' in turn
    assert 'push af\n\tldh a, [hBattleTurn]\n\txor 1\n\tldh [hBattleTurn], a\n\tpop af\n\tret' in turn
    home = files['home/battle.asm'].split('\nGetOpponentAbility::', 1)[1].split('\nGetOpponentAbilityAfterMoldBreaker::', 1)[0]
    assert 'cp NEUTRALIZING_GAS' in home and 'farcall AbilityCanBeSuppressed' in home
    core = '\n'.join(source.default_lines(files['engine/battle/core.asm'].split('\nUserCanSwitch:', 1)[1].split('\nTryPlayerSwitch:', 1)[0]))
    assert core.index('cp RUN_AWAY') < core.index('farcall CheckIfTrappedByAbility') < core.index('ld a, [wPlayerWrapCount]')
    abilities = ['KEEN_EYE', 'ARENA_TRAP', 'MAGNET_PULL', 'SHADOW_TAG']
    species = [('RATTATA', set()), ('MAGNEMITE', {'STEEL'}), ('PIDGEY', {'FLYING'}), ('GASTLY', {'GHOST'}), ('SKARMORY', {'STEEL', 'FLYING'})]

    def applies(ability, types):
        if 'GHOST' in types:
            return False
        return ability == 'SHADOW_TAG' or ability == 'MAGNET_PULL' and 'STEEL' in types or ability == 'ARENA_TRAP' and 'FLYING' not in types

    vectors = []
    for a, b, (sa, ta), (sb, tb) in itertools.product(abilities, abilities, species, species):
        raw = [applies(b, ta), applies(a, tb)]
        source_mask = 0 if all(raw) else int(raw[0]) | int(raw[1]) << 1
        native = [raw[0] and not (a == b == 'SHADOW_TAG'), raw[1] and not (a == b == 'SHADOW_TAG')]
        native_mask = int(native[0]) | int(native[1]) << 1
        vectors.append(dict(species=[sa,sb], abilities=[a,b], source_trapped_mask=source_mask, native_trapped_mask=native_mask))
    assert len(vectors) == 400
    header = '// Generated from pinned trapping rules; bit 0 = player, bit 1 = opponent.\n'
    header += 'struct PolishedTrappingVector { u16 species[2]; u16 abilities[2]; u8 sourceMask; u8 nativeMask; };\n'
    header += 'static const struct PolishedTrappingVector sPolishedTrappingVectors[] = {\n'
    for v in vectors:
        header += '    {{SPECIES_%s, SPECIES_%s}, {ABILITY_%s, ABILITY_%s}, %d, %d},\n' % (*v['species'], *v['abilities'], v['source_trapped_mask'], v['native_trapped_mask'])
    header += '};\n'
    report = dict(source_repository='Rangi42/polishedcrystal', source_commit=source.PIN,
        source_variant='default (FAITHFUL undefined)', profile='B_POLISHED_MOVE_BALANCE',
        scope='Ordinary singles ability-trap eligibility and mutual cancellation; native grounding supplies optional field extensions.',
        rules=['Ghost targets cannot be trapped by these abilities.',
               'Shadow Tag traps other targets; Magnet Pull requires Steel; Arena Trap requires a grounded target.',
               'Check each direction independently. If both apply, neither battler is ability-trapped.',
               'Same-name abilities do not grant an independent exemption: a Ghost Shadow Tag holder can trap a non-Ghost Shadow Tag holder.',
               'Neutralizing Gas suppresses the ability before eligibility. Run Away remains an earlier switch exemption; move traps remain separate.'],
        matrix_vectors=vectors,
        adapter='Compute both eligible directions from live types, unsuppressed abilities and grounding, and return the existing native trapper identifier only for one-way trapping. Keep native profiles, doubles, active gimmicks, fields, IDs and save state.',
        gaps=['Complete wild fleeing/Teleport, source AI strategy and estimates, forced switches and full switch timing remain separate.',
              'Shed Shell/item suppression, all grounding modifiers and optional formats/gimmicks are not certified as source behavior.',
              'The native Air Balloon/Iron Ball/Levitate controls verify selected ordinary grounding cases; Unnerve and other item/ability modifiers remain separate.'])
    return {source.ROOT/'test/battle/ability/polished_trapping_vectors.h': header,
            source.ROOT/'tools/polished_port/manifests/trapping_abilities.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, expected in build().items():
        if args.check:
            assert path.read_text() == expected, f'Regenerate {path}'
        else:
            path.write_text(expected)
    print('PASS: pinned mutual trapping gates and 400 type/ability vectors')
