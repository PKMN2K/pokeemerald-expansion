#!/usr/bin/env python3
"""Check source trainer construction against actual production-linked parties.

This separate fixture calls the shipping trainer factory with the shipping
trainer table. It does not replace that table with battle-test dummy data.
"""
import argparse
import hashlib
import json
from pathlib import Path
import tempfile

import build_trainer_attributes as trainers
import build_pokemon_base_data as species
from audit_moves_machines import compile_probe, sample
from test_johto_demo_emulator import Game

ROOT = trainers.ROOT


def probe_code(parties):
    code = r'''
#include "global.h"
#include "battle.h"
#include "data.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokemon.h"
#include "save.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/opponents.h"
void CreateTrainerPartyForPlayer(void);
struct ExpectedMon { u16 species, nature, gender; };
struct ExpectedTrainer { u16 trainer; struct ExpectedMon mons[PARTY_SIZE]; };
static const struct ExpectedTrainer expected[] = {
'''
    for party in parties:
        mons = ', '.join('{'+', '.join((m['species'], m['nature'], 'MON_'+m['gender']))+'}' for m in party['mons'])
        code += '{'+party['target_trainer']+', {'+mons+'}},\n'
    return code + r'''
};
EWRAM_DATA volatile u32 gAuditResult[157] = {0};
EWRAM_DATA volatile u32 gAuditCursor = 0;
EWRAM_DATA volatile u32 gAuditKind = 0;
EWRAM_DATA volatile u32 gAuditReady = 0;
static EWRAM_DATA bool8 initialized = FALSE;
void CB2_MoveMachineAudit(void)
{
    if (!initialized)
    {
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        NewGameInitData();
        VarSet(VAR_POLISHED_EARNED_BADGES, 0);
        gBattleTypeFlags = BATTLE_TYPE_TRAINER;
        initialized = TRUE;
    }
    if (gAuditReady)
    {
        if (gMain.newKeys & A_BUTTON)
        {
            gAuditCursor++;
            gAuditReady = 0;
        }
        return;
    }
    if (gAuditCursor >= ARRAY_COUNT(expected)) return;
    const struct ExpectedTrainer *team = &expected[gAuditCursor];
    gSpecialVar_0x8004 = team->trainer;
    CreateTrainerPartyForPlayer();
    u32 count = GetTrainerPartySizeFromId(team->trainer);
    gAuditResult[0] = count;
    for (u32 slot = 0; slot < count && slot < PARTY_SIZE; slot++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];
        u32 offset = 1 + slot * 25;
        gAuditResult[offset] = GetMonData(mon, MON_DATA_SPECIES) == team->mons[slot].species;
        gAuditResult[offset+1] = GetMonData(mon, MON_DATA_LEVEL);
        gAuditResult[offset+2] = GetNature(mon) == team->mons[slot].nature;
        gAuditResult[offset+3] = GetMonGender(mon) == team->mons[slot].gender;
        gAuditResult[offset+4] = GetMonData(mon, MON_DATA_ABILITY_NUM);
        gAuditResult[offset+5] = GetMonData(mon, MON_DATA_FRIENDSHIP);
        gAuditResult[offset+6] = GetMonData(mon, MON_DATA_POKEBALL) == BALL_POKE;
        for (u32 stat = 0; stat < NUM_STATS; stat++)
        {
            gAuditResult[offset+7+stat] = GetMonData(mon, MON_DATA_HP_IV+stat);
            gAuditResult[offset+13+stat] = GetMonData(mon, MON_DATA_HP_EV+stat);
            gAuditResult[offset+19+stat] = GetMonData(mon, MON_DATA_MAX_HP+stat);
        }
    }
    if (team->trainer == TRAINER_POLISHED_GIOVANNI1)
    {
        struct Pokemon *boss = &gParties[B_TRAINER_PLAYER][0];
        static const u8 expectedName[] = {CHAR_QUESTION_MARK, CHAR_QUESTION_MARK, CHAR_QUESTION_MARK, CHAR_QUESTION_MARK, CHAR_QUESTION_MARK, EOS};
        u8 name[POKEMON_NAME_LENGTH + 1];
        GetMonData(boss, MON_DATA_NICKNAME, name);
        gAuditResult[151] = GetMonData(boss, MON_DATA_HELD_ITEM) == ITEM_POLISHED_ARMOR_SUIT;
        gAuditResult[152] = StringCompare(name, expectedName) == 0;
        gAuditResult[153] = GetMonData(boss, MON_DATA_MOVE1) == MOVE_SHADOW_BALL;
        gAuditResult[154] = GetMonData(boss, MON_DATA_MOVE2) == MOVE_PSYSTRIKE;
        gAuditResult[155] = GetMonData(boss, MON_DATA_MOVE3) == MOVE_NASTY_PLOT;
        gAuditResult[156] = GetMonData(boss, MON_DATA_MOVE4) == MOVE_AURA_SPHERE;
    }
    gAuditReady = 1;
}
'''


def expected_stats(mon, base, level):
    # Source modern-EV formula, in HP/Atk/Def/Spe/SAt/SDf order. All current
    # registered natures are covered explicitly; new natures require review.
    boosts = {'NATURE_HARDY': (0, 0), 'NATURE_ADAMANT': (1, 4),
              'NATURE_RELAXED': (2, 3), 'NATURE_RASH': (4, 5), 'NATURE_MODEST': (4, 1)}
    raised, lowered = boosts[mon['nature']]
    result = []
    for i, (stat, iv, ev) in enumerate(zip(base['stats'], mon['ivs'], mon['evs'], strict=True)):
        value = (2 * stat + iv + ev // 4) * level // 100 + (level + 10 if i == 0 else 5)
        if i and raised != lowered:
            if i == raised: value = value * 110 // 100
            if i == lowered: value = value * 90 // 100
        result.append(value)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    parties = json.loads(trainers.build()[trainers.MANIFEST])['bindings']
    bases = {r['target_species']: r for r in json.loads(species.build()[species.MANIFEST])['records']}
    checked = []
    with tempfile.TemporaryDirectory(prefix='polished-trainer-production-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, probe_code(parties))
        game = Game(args.driver.resolve(), elf, elf, output, directory)
        try:
            for cursor, party in enumerate(parties):
                row = sample(game, 0, cursor, 157)
                assert row[0] == len(party['mons']), (party['target_trainer'], row[0])
                for slot, mon in enumerate(party['mons']):
                    level = int(mon['source_level'].split('+')[-1]) + (4 if 'LEVEL_FROM_BADGES' in mon['source_level'] else 0)
                    expected = [1, level, 1, 1, mon['ability_slot'], 160, 1, *mon['ivs'], *mon['evs'],
                                *expected_stats(mon, bases[mon['species']], level)]
                    actual = row[1+slot*25:1+(slot+1)*25]
                    assert actual == expected, (party['target_trainer'], slot, actual, expected)
                if party['target_trainer'] == 'TRAINER_POLISHED_GIOVANNI1':
                    assert row[151:157] == [1] * 6, ('Giovanni item/nickname/moves', row[151:157])
                checked.append(dict(trainer=party['target_trainer'], pokemon=row[0]))
                print('PASS: production trainer', party['target_trainer'], row[0], flush=True)
        finally:
            game.close()
    report = dict(source_commit=trainers.archive.source.PIN, fixture_only=True,
        production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        trainers=checked, total_pokemon=sum(r['pokemon'] for r in checked),
        result='production construction matches source species, level, nature, gender, ability slot, friendship, ball, six IVs, six EVs and six stats',
        complete_behavior_parity=False)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f"PASS: {len(checked)} production trainers / {report['total_pokemon']} Pokemon", flush=True)


if __name__ == '__main__':
    main()
