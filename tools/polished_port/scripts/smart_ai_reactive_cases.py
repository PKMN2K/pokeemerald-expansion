"""Final sixteen reachable Smart moves in the pinned default move/effect tables."""
from itertools import product

HANDLERS = {
    'PriorityHit': {'QUICK_ATTACK', 'MACH_PUNCH', 'BULLET_PUNCH', 'AQUA_JET', 'ICE_SHARD', 'EXTREMESPEED'},
    'Counter': {'COUNTER', 'MIRROR_COAT'}, 'Disable': {'DISABLE'}, 'Encore': {'ENCORE'},
    'Roar': {'ROAR'}, 'MeanLook': {'MEAN_LOOK'}, 'Curse': {'CURSE'}, 'Protect': {'PROTECT'},
    'PerishSong': {'PERISH_SONG'}, 'BatonPass': {'BATON_PASS'},
}
MOVES = set.union(*HANDLERS.values())


def score(move, f, hp, maximum, target, speeds, room, rng, source_moves, cache):
    def byte():
        return next(rng) & 255

    def faster():
        if speeds[0] == speeds[1]:
            byte()
            return False
        return speeds[0] < speeds[1] if room else speeds[0] > speeds[1]

    selected = f.get('selected', 'MOVE_NONE')
    last = f.get('last', 'MOVE_NONE')
    data, useful, encore = source_moves
    if move in HANDLERS['PriorityHit']:
        if faster():
            return 0
        if f.get('hidden') in ('STATE_ON_AIR', 'STATE_UNDERGROUND'):
            return -10
        return 3 if f.get('damage', 0) > target else 0
    if move in HANDLERS['Counter']:
        category = data['MOVE_' + move]
        known = [f.get('seen' + str(i)) for i in range(4)]
        matches = [m for m in known if data.get(m) == category]
        if not matches:
            return -1
        return int((len(matches) >= 3 or data.get(selected) == category) and byte() >= 100)
    if move == 'DISABLE':
        if faster() and selected in useful:
            return int(byte() >= 100)
        return -int(byte() >= 20)
    if move == 'ENCORE':
        if not faster():
            return -3
        if last == 'MOVE_NONE':
            return -10
        if data.get(last) and f.get('matchup', 0) < 4096:
            if f.get('matchup', 0):
                return 0
        elif selected not in encore:
            return -3
        return 2 if byte() >= 70 else 0
    if move == 'ROAR':
        return -int(cache[0] >= 10)
    if move == 'MEAN_LOOK':
        if hp <= maximum // 2:
            return -1
        if not f.get('targetReserve') or f.get('ghost'):
            return -10
        if f.get('targetStatus') == 'STATUS1_TOXIC_POISON' or any(f.get(x) for x in ('targetInLove','targetIdentified','targetRolling')):
            return 3 if byte() >= 50 else 0
        return -int(cache[0] < 11)
    if move == 'BATON_PASS':
        return 0
    if move == 'CURSE':
        if not f.get('userGhost'):
            return 0
        if f.get('cursed'):
            return -10
        escapes = f.get('ghost') or f.get('shell') or f.get('targetAbility') == 'ABILITY_RUN_AWAY' or not f.get('trapped')
        return int(f.get('userReserve', 0)) + 2 * (hp <= maximum // 4) - int(bool(f.get('targetReserve') and escapes))
    if move == 'PERISH_SONG':
        if (not f.get('userReserve') or f.get('perish') or
                (f.get('targetAbility') == 'ABILITY_SOUNDPROOF' and f.get('userAbility') != 'ABILITY_MOLD_BREAKER')):
            return -5
        cache[0] = f.get('best', 0)
        if not cache[0]:
            return -5
        return 3 if not f.get('targetReserve') or (not f.get('ghost') and not f.get('shell') and f.get('trapped')) else 0
    assert move == 'PROTECT'
    roll = byte()
    if f.get('userProtected'):
        return -1 - 2 * (roll >= 20)
    if f.get('targetLockOn'):
        return -2 * (roll >= 20)
    rolling = f.get('targetRolling', 0)
    residual = f.get('cursed') or f.get('targetStatus') == 'STATUS1_TOXIC_POISON' or f.get('targetSeeded')
    if f.get('charged') or (rolling and (rolling >= 3 or residual)):
        return int(roll >= 50)
    return -2 * (roll >= 20)


def cases(row, draws, source_moves):
    def emit(slots, hp=80, target=99, speeds=(80, 120), room=False, mask=0, seed=7, **flags):
        return row(slots, hp, 101, target, 'clear', 'clear', mask, seed, speeds, room, target_max=301, tactics=flags)

    for slots in (('QUICK_ATTACK','ICE_SHARD','MACH_PUNCH','BULLET_PUNCH'), ('AQUA_JET','EXTREMESPEED','ENCORE','COUNTER')):
        for damage, target, speeds, hidden, room, mask in product((98, 99, 100, 256), (99, 255),
                ((80,120),(100,100),(120,80)), ('STATE_NONE','STATE_ON_AIR','STATE_UNDERGROUND','STATE_UNDERWATER'),
                (False,True), (0,5)):
            yield emit(slots, target=target, damage=damage, speeds=speeds, hidden=hidden, room=room, mask=mask)
    for known, selected in product(((),('SURF',),('SPLASH','TACKLE'),('SURF','THUNDER','ICE_BEAM'),
            ('COUNTER','QUICK_ATTACK','GUST','SPLASH'),('QUICK_ATTACK','TACKLE','COUNTER')), ('NONE','TACKLE','THUNDER','SPLASH')):
        for speeds in ((120,80),(100,100)):
            yield emit(('COUNTER','MIRROR_COAT','DISABLE','ENCORE'), selected='MOVE_'+selected,
                       speeds=speeds, **{'seen'+str(i):'MOVE_'+m for i,m in enumerate(known)})
    for last, selected, matchup in product(('MOVE_NONE','MOVE_TACKLE','MOVE_SPLASH'), ('MOVE_SPLASH','MOVE_SURF'), (0,2048,4096,8192)):
        yield emit(('ENCORE','DISABLE','MIRROR_COAT','COUNTER'), last=last, selected=selected, matchup=matchup, speeds=(120,80))
    for selected in sorted(source_moves[1] | source_moves[2]):
        yield emit(('ENCORE','DISABLE','COUNTER','MIRROR_COAT'), selected=selected, last='MOVE_SPLASH', speeds=(120,80))
    for cached, best, mask in product((0,7,9,10,11,13),(0,7,10,11,13),(0,2)):
        yield emit(('ROAR','PERISH_SONG','ROAR','MEAN_LOOK'), cached=cached, best=best, userReserve=1, targetReserve=1, mask=mask)
    for hp, user, target, flags in product((25,26,50,51,101),(0,1),(0,1),
            ({},{'ghost':1},{'trapped':1},{'userGhost':1},{'userGhost':1,'cursed':1},
             {'userGhost':1,'trapped':1},{'userGhost':1,'trapped':1,'shell':1},
             {'userGhost':1,'trapped':1,'targetAbility':'ABILITY_RUN_AWAY'},
             {'targetAbility':'ABILITY_SOUNDPROOF'}, {'targetAbility':'ABILITY_SOUNDPROOF','userAbility':'ABILITY_MOLD_BREAKER'},
             {'perish':1},{'targetStatus':'STATUS1_TOXIC_POISON'},{'targetIdentified':1},{'targetInLove':1},{'targetRolling':1})):
        yield emit(('CURSE','MEAN_LOOK','PERISH_SONG','ROAR'), hp=hp, userReserve=user, targetReserve=target, cached=10, best=11, **flags)
    for repeat, lock, charged, rolling, flags in product((0,1),(0,1),(0,1),(0,1,2,3,4),
            ({},{'cursed':1},{'targetSeeded':1},{'targetStatus':'STATUS1_TOXIC_POISON'})):
        yield emit(('PROTECT','BATON_PASS','PROTECT','ROAR'), userProtected=repeat, targetLockOn=lock, charged=charged,
                   targetRolling=rolling, stat='STAT_ATK', userOffset=rolling-1, **flags)
    for move, cutoff, flags in [('ENCORE',70,{'last':'MOVE_SPLASH','selected':'MOVE_SPLASH'}),
                                ('DISABLE',20,{}), ('DISABLE',100,{'selected':'MOVE_SURF'}),
                                ('COUNTER',100,{'seen0':'MOVE_TACKLE','selected':'MOVE_TACKLE'}),
                                ('MEAN_LOOK',50,{'targetReserve':1,'targetRolling':1}),
                                ('PROTECT',20,{'userProtected':1}), ('PROTECT',50,{'charged':1})]:
        for value in (cutoff-1, cutoff):
            seed = next(n for n in range(65536) if next(draws(n)) & 255 == value)
            yield emit((move,'TACKLE','TACKLE','TACKLE'), seed=seed, speeds=(120,80), **flags)


HISTORY_BATTLES = ('encore_history', 'physical_history', 'special_history', 'fire_reserve', 'grass_reserve')
BATTLES = {'priority_below', 'priority_equal', 'priority_faster', 'perish_first', 'roar_first', 'perish_alone', 'bond_damage', 'single_damage'} | {
    n + suffix for n in HISTORY_BATTLES for suffix in ('', '_next')
}


def check_battles(battles):
    fixed = {
        'priority_below': ('Quick Attack', (103,102)), 'priority_equal': ('Recover', (100,102)),
        'priority_faster': ('Recover', (100,102)), 'perish_first': ('Perish Song',(103,99)),
        'roar_first': ('Perish Song',(100,103)), 'perish_alone': ('Roar',(95,100)),
        'encore_history': ('Destiny Bond',(90,99)),
        'fire_reserve': ('Recover',(100,102)), 'grass_reserve': ('Recover',(100,102)),
        'fire_reserve_next': ('Roar',(100,98)), 'grass_reserve_next': ('Roar',(99,98)),
        'bond_damage': ('Quick Attack',(103,102)), 'single_damage': ('Recover',(100,102)),
    }
    for name, (move, scores) in fixed.items():
        b = battles[name]
        assert b['move'] == move and tuple(b['scores']) == scores, (name, b)
    b = battles['encore_history_next']
    assert b['move'] == 'Encore' and b['scores'][0] in (100,102) and b['scores'][1] == 99, b
    for name, slot, move in (('physical_history', 0, 'Counter'), ('special_history', 1, 'Mirror Coat')):
        assert tuple(battles[name]['scores']) == (99,99), (name, battles[name])
        b = battles[name+'_next']
        assert b['move'] == move and b['scores'][slot] in (100,101) and b['scores'][1-slot] == 99, (name, b)
