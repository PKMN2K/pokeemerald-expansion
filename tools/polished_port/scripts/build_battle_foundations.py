#!/usr/bin/env python3
"""Pin shared action/status and defensive weather rules, not whole-formula parity."""
import argparse
import json
import build_pokemon_learnsets as source


def build():
    files = source.sources()
    commands = files['engine/battle/effect_commands.asm']
    assert 'DEF percent EQUS "* $ff / 100"' in files['macros/data.asm']
    action = commands.split('BattleCommand_checkturn:', 1)[1].split('HitConfusion:', 1)[0]
    assert 'cp 1 + (33 percent)\n\tjr nc, .not_confused' in action
    assert 'cp 1 + (25 percent)\n\tret nc' in action
    sleep = commands.split('BattleCommand_sleep:', 1)[1].split('CanPoisonTarget:', 1)[0]
    assert 'ld a, 3\n\tcall BattleRandomRange\n\tadd 2' in sleep
    weather = commands.split('SandstormSpDefBoost:', 1)[1].split('BattleCommand_clearmissdamage:', 1)[0]
    assert 'lb bc, WEATHER_SANDSTORM, ROCK' in weather
    assert 'lb bc, WEATHER_HAIL, ICE' in weather
    assert 'call GetWeatherAfterOpponentUmbrella' in weather
    assert 'srl b\n\trr c\n\tadd hl, bc' in weather
    values = dict(POLISHED_ACTION_ROLL_RANGE=256,
                  POLISHED_CONFUSION_SELF_HIT=1 + 33 * 255 // 100,
                  POLISHED_PARALYSIS_STOP=1 + 25 * 255 // 100,
                  POLISHED_MOVE_SLEEP_MIN=2, POLISHED_MOVE_SLEEP_MAX=4)
    header = f'/* Generated from pinned Polished Crystal {source.PIN}. */\n'
    header += ''.join(f'#define {name} {value}\n' for name, value in values.items())
    report = dict(source_commit=source.PIN, values=values,
        scope='Source-profile singles without either active gimmick. Shared action rules also apply to optional attacks; move-inflicted sleep is separate from Effect Spore and Rest.',
        evidence=['checkturn: confusion 85/256; paralysis 64/256, with no Magic Guard exemption.',
                  'sleep: uniform stored counters 2, 3, 4; checkturn decrements before waking.',
                  'HailDefenseBoost and SandstormSpDefBoost: Ice physical Defense and Rock special Defense gain floor(stat/2).',
                  'The broader source archive pins GetSpeed (half paralysis speed except Quick Feet) and GetWeatherAfterOpponentUmbrella (hail/sand unaffected by Umbrella).'],
        open=['Exact RNG stream and call count; full action ordering and self-confusion damage.',
              'Complete damage/stat modifier ordering, rounding, saturation, criticals, AI estimates and weather lifecycle.',
              'Optional abilities, formats and gimmicks remain native; passing controls does not certify source behavior.'])
    return {source.ROOT/'include/constants/polished_battle_foundations.h': header,
            source.ROOT/'tools/polished_port/manifests/battle_foundations.json': json.dumps(report, indent=2)+'\n'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for path, content in build().items():
        if args.check:
            assert path.read_text() == content, f'Regenerate {path}'
        else:
            path.write_text(content)
    print('PASS: source action probabilities, sleep counters and weather defense rules')
