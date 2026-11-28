"""Basic trainer decisions from AI_Redundant in the default v3.2.3 ROM."""
from itertools import product
import re

RECOVERY = {'RECOVER', 'REST', 'ROOST', 'FRESH_SNACK', 'HEALINGLIGHT'}
SETTERS = {'RAIN_DANCE': 'rain', 'SUNNY_DAY': 'sun', 'SANDSTORM': 'sand', 'HAIL': 'hail'}
SCREENS = {'REFLECT': 'reflect', 'LIGHT_SCREEN': 'lightScreen', 'SAFEGUARD': 'ownSafeguard'}
CONFUSION = {'SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS'}
STATUS_ONLY = {'SPORE', 'HYPNOSIS', 'SING', 'SLEEP_POWDER', 'WILL_O_WISP', 'TOXIC',
               'POISONPOWDER', 'GLARE', 'STUN_SPORE', 'THUNDER_WAVE'}
ACTIVE_FLAGS = {
    'FOCUS_ENERGY': 'focusEnergy', 'TRANSFORM': 'transformed', 'SUBSTITUTE': 'substitute',
    'LEECH_SEED': 'targetSeeded', 'DISABLE': 'disabled', 'ENCORE': 'encored', 'MEAN_LOOK': 'meanLook',
    'FORESIGHT': 'targetIdentified', 'PERISH_SONG': 'perish', 'FUTURE_SIGHT': 'futureSight',
    'DESTINY_BOND': 'destinyBond',
}
SCENARIOS = ('basic_screens', 'basic_confusion', 'basic_weather', 'basic_seed_focus', 'types_foresight')
BATTLES = {name + suffix for name in SCENARIOS for suffix in ('', '_next')} | {
    'basic_egg_hp', 'basic_no_reserves', 'types_dragon', 'types_psychic',
    'types_umbreon', 'types_umbreon_ring', 'types_wyrdeer', 'types_wyrdeer_ring', 'types_powder', 'types_goggles',
    'types_bug_dark', 'types_cut_fairy', 'types_hidden_power', 'types_rock'}


def check_source(assembly, status_effects, move_records):
    table = dict(re.findall(r'^\s*dbw EFFECT_(\w+),\s+\.(\w+)$', assembly, re.M))
    expected = {
        'Heal': {'RECOVER', 'REST', 'FRESH_SNACK'}, 'HealingLight': {'HEALINGLIGHT'}, 'Roost': {'ROOST'},
        'RainDance': {'RAIN_DANCE'}, 'SunnyDay': {'SUNNY_DAY'}, 'Sandstorm': {'SANDSTORM'}, 'Hail': {'HAIL'},
        'Reflect': {'REFLECT'}, 'LightScreen': {'LIGHT_SCREEN'}, 'Safeguard': {'SAFEGUARD'},
        'Confuse': CONFUSION, 'Swagger': {'SWAGGER'},
    }
    expected.update({name: {move} for name, move in (
        ('DreamEater', 'DREAM_EATER'), ('FocusEnergy', 'FOCUS_ENERGY'), ('Roar', 'ROAR'), ('Transform', 'TRANSFORM'),
        ('Substitute', 'SUBSTITUTE'), ('LeechSeed', 'LEECH_SEED'), ('Disable', 'DISABLE'), ('Encore', 'ENCORE'),
        ('SleepTalk', 'SLEEP_TALK'), ('MeanLook', 'MEAN_LOOK'), ('Curse', 'CURSE'), ('Spikes', 'SPIKES'),
        ('ToxicSpikes', 'TOXIC_SPIKES'), ('Foresight', 'FORESIGHT'), ('PerishSong', 'PERISH_SONG'), ('Attract', 'ATTRACT'),
        ('Teleport', 'TELEPORT'), ('FutureSight', 'FUTURE_SIGHT'), ('BatonPass', 'BATON_PASS'),
        ('TrickRoom', 'TRICK_ROOM'), ('DestinyBond', 'DESTINY_BOND'))})
    assert set(table.values()) == expected.keys()
    status_effects = set(re.findall(r'^\s*db EFFECT_(\w+)', status_effects, re.M))
    status_moves = set()
    found = {handler: set() for handler in expected}
    for line in move_records:
        match = re.match(r'move (\w+),\s*EFFECT_(\w+),', line)
        if match and match[2] in status_effects:
            status_moves.add(match[1])
        if match and (handler := table.get(match[2])) in found:
            found[handler].add(match[1])
    assert found == expected and status_moves == STATUS_ONLY
    assert len(set.union(status_moves, *found.values())) == 47


def penalty(move, hp, maximum, weather, flags, status, speeds, room, stream):
    if move in STATUS_ONLY:
        redundant = flags.get('targetStatus') or flags.get('safeguard')
    elif move == 'DREAM_EATER':
        target_status = flags.get('targetStatus', 0)
        redundant = not (isinstance(target_status, int) and target_status & 7)
    elif move in RECOVERY:
        redundant = hp == maximum
    elif move in SETTERS:
        redundant = weather == SETTERS[move]
    elif move in SCREENS:
        redundant = flags.get(SCREENS[move])
    elif move in CONFUSION or move == 'SWAGGER':
        redundant = flags.get('targetConfused') or (move in CONFUSION and flags.get('safeguard'))
    elif move in ACTIVE_FLAGS:
        redundant = flags.get(ACTIVE_FLAGS[move])
    elif move in ('TELEPORT', 'BATON_PASS'):
        redundant = not flags.get('userReserve')
    elif move == 'ROAR':
        redundant = not flags.get('targetReserve')
    elif move == 'SLEEP_TALK':
        redundant = not (isinstance(status, int) and status & 7)
    elif move == 'CURSE':
        redundant = flags.get('userGhost') and flags.get('cursed')
    elif move == 'SPIKES':
        redundant = flags.get('targetSpikes') == 3
    elif move == 'TOXIC_SPIKES':
        redundant = flags.get('toxicSpikes') == 2
    elif move == 'ATTRACT':
        redundant = not flags.get('oppositeGender') or flags.get('targetInLove')
    elif move == 'TRICK_ROOM':
        if speeds[0] == speeds[1]:
            next(stream)
        redundant = speeds[0] < speeds[1] if room else speeds[0] > speeds[1]
    else:
        redundant = False
    return -10 if redundant else 0


def cases(row, state_type):
    def emit(slots, flags, mask, hp=301, weather='clear', state=None):
        return row(slots, hp, 301, 256, weather, 'clear', mask, 7, target_max=301,
                   state=state or state_type(), tactics=flags)

    screens = ('REFLECT', 'LIGHT_SCREEN', 'SAFEGUARD', 'CONFUSE_RAY')
    for bits, mask in product(product((0, 1), repeat=4), (0, 2, 9, 15)):
        flags = dict(zip(('reflect', 'lightScreen', 'ownSafeguard', 'safeguard'), bits))
        yield emit(screens, flags, mask)

    for confused, guard, own_guard, mask in product((0, 1, 3), (0, 1), (0, 1), (0, 1, 5, 15)):
        yield emit(('SUPERSONIC', 'CONFUSE_RAY', 'SWEET_KISS', 'SWAGGER'),
                   dict(targetConfused=confused, safeguard=guard, ownSafeguard=own_guard), mask)

    for hp, status, mask in product((256, 300, 301), (0, 'STATUS1_POISON'), (0, 2, 15)):
        for slots in (('REST', 'RECOVER', 'ROOST', 'FRESH_SNACK'), ('HEALINGLIGHT', 'REST', 'RECOVER', 'ROOST')):
            yield emit(slots, {}, mask, hp, state=state_type(status=status))

    for weather, mask in product(('clear', 'sun', 'rain', 'sand', 'hail'), (0, 1, 6, 15)):
        yield emit(tuple(SETTERS), {}, mask, weather=weather)


    for move, field in ACTIVE_FLAGS.items():
        for active, mask in product((0, 1), (0, 1, 5)):
            yield emit((move, 'TACKLE', move, 'TACKLE'), {field: active}, mask)
    for a, b, mask in product((0, 1), (0, 1), (0, 1, 6)):
        for slots, flags in (
                (('BATON_PASS', 'ROAR', 'TELEPORT', 'ROAR'), dict(userReserve=a, targetReserve=b)),
                (('CURSE', 'ATTRACT', 'CURSE', 'ATTRACT'), dict(userGhost=a, cursed=b, oppositeGender=b, targetInLove=a))):
            yield emit(slots, flags, mask)
    for spikes, toxic, mask in product(range(4), range(3), (0, 2)):
        yield emit(('SPIKES', 'TOXIC_SPIKES', 'SPIKES', 'TOXIC_SPIKES'), dict(targetSpikes=spikes, toxicSpikes=toxic), mask)
    for status, guard, mask in product((0, 2, 'STATUS1_POISON'), (0, 1), (0, 5)):
        for slots in (('SLEEP_TALK', 'GLARE', 'WILL_O_WISP', 'POISONPOWDER'),
                      ('STUN_SPORE', 'TOXIC', 'THUNDER_WAVE', 'SLEEP_TALK')):
            yield emit(slots, dict(targetStatus=status, safeguard=guard), mask, state=state_type(status=status))
    for speeds, room, mask in product(((99, 100), (100, 100), (101, 100)), (False, True), (0, 1, 4, 5)):
        yield row(('TRICK_ROOM', 'ATTRACT', 'TRICK_ROOM', 'ICY_WIND'), 100, 101, 99, 'clear', 'clear',
                  mask, 7, speeds=speeds, room=room)


def check_battles(results):
    def check(name, scores, moves):
        battle = results[name]
        if tuple(battle['scores']) != scores or battle['move'] not in moves:
            raise ValueError(f'{name}: {battle}')

    check('basic_screens', (100, 100), ('Reflect', 'Light Screen'))
    if results['basic_screens']['move'] == 'Reflect':
        check('basic_screens_next', (90, 100), ('Light Screen',))
    else:
        check('basic_screens_next', (100, 90), ('Reflect',))
    check('basic_confusion', (100, 100), ('Confuse Ray', 'Swagger'))
    check('basic_confusion_next', (90, 100), ('Swagger',))
    check('basic_weather', (90, 100), ('Rain Dance',))
    check('basic_weather_next', (100, 90), ('Sunny Day',))

    check('basic_seed_focus', (100, 100), ('Leech Seed', 'Focus Energy'))
    if results['basic_seed_focus']['move'] == 'Leech Seed':
        check('basic_seed_focus_next', (100, 90), ('Focus Energy',))
    else:
        check('basic_seed_focus_next', (90, 100), ('Leech Seed',))
    check('basic_egg_hp', (100, 90), ('Roar',))
    check('basic_no_reserves', (90, 90), ('Roar', 'Baton Pass'))
    check('types_dragon', (101, 100), ('Ice Beam',))
    check('types_psychic', (99, 101), ('Crunch',))
    check('types_foresight', (100, 90), ('Foresight',))
    check('types_foresight_next', (90, 100), ('Brick Break',))
    check('types_umbreon', (90, 99), ('Shadow Ball',))
    check('types_umbreon_ring', (100, 99), ('Psychic',))
    check('types_wyrdeer', (90, 100), ('Brick Break',))
    check('types_wyrdeer_ring', (101, 100), ('Shadow Ball',))
    check('types_powder', (100, 99, 90), ('PoisonPowder',))
    check('types_goggles', (90, 99, 90), ('Dark Pulse',))
    check('types_bug_dark', (101, 100), ('Bug Buzz',))
    check('types_cut_fairy', (99, 101), ('Cut',))
    check('types_hidden_power', (100, 101), ('Aerial Ace',))
    check('types_rock', (101, 100), ('Rock Slide',))
