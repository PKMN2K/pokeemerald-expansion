#!/usr/bin/env python3
"""Verify level-one growth and friendship survive the production Flash save path.

This fixture exercises the shipping persistence and save APIs. It does not
certify the full battle lifecycle or every field happiness event.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from audit_moves_machines import ROOT, compile_probe
from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import Game

CODE = r'''
#include "global.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "overworld.h"
#include "pokemon.h"
#include "polished_friendship.h"
#include "save.h"
#include "constants/opponents.h"
#include "constants/pokeball.h"

EWRAM_DATA volatile u32 gAuditResult[15] = {0};
EWRAM_DATA volatile u32 gAuditReady = 0;
EWRAM_DATA volatile u32 gAuditPhase = 0;
static const u16 species[] = {SPECIES_CHANSEY, SPECIES_PIKACHU, SPECIES_MAGIKARP};
static const u8 friendship[] = {162, 107, 150};

void CB2_MoveMachineAudit(void)
{
    if (gAuditReady) return;
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
        gAuditPhase = 1;
    else
    {
        NewGameInitData();
        ZeroPlayerPartyMons();
        for (u32 i = 0; i < ARRAY_COUNT(species); i++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
            u32 value = i == 1 ? 99 : 160;
            u32 item = ITEM_SOOTHE_BELL, ball = BALL_LUXURY;
            u32 location = GetCurrentRegionMapSectionId();
            CreateMonWithIVs(mon, species[i], 1, 0, OTID_STRUCT_PLAYER_ID, 31);
            SetMonData(mon, MON_DATA_FRIENDSHIP, &value);
            SetMonData(mon, MON_DATA_HELD_ITEM, &item);
            SetMonData(mon, MON_DATA_POKEBALL, &ball);
            SetMonData(mon, MON_DATA_MET_LOCATION, &location);
            AdjustFriendship(mon, i == 2 ? FRIENDSHIP_EVENT_FAINT_LARGE : FRIENDSHIP_EVENT_GROW_LEVEL);
        }
        CalculatePlayerPartyCount();
    }
    gAuditResult[0] = gPartiesCount[B_TRAINER_PLAYER] == 3;
    for (u32 i = 0; i < ARRAY_COUNT(species); i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        gAuditResult[1+i*4] = GetMonData(mon, MON_DATA_SPECIES) == species[i];
        gAuditResult[2+i*4] = GetMonData(mon, MON_DATA_EXP) == 0;
        gAuditResult[3+i*4] = GetMonData(mon, MON_DATA_LEVEL) == 1 && GetLevelFromBoxMonExp(&mon->box) == 1;
        gAuditResult[4+i*4] = GetMonData(mon, MON_DATA_FRIENDSHIP) == friendship[i];
    }
    gAuditResult[13] = PolishedIsBossTrainer(TRAINER_POLISHED_FALKNER);
    gAuditResult[14] = !PolishedIsBossTrainer(TRAINER_POLISHED_SPROUT_LI);
    if (!gAuditPhase && TrySavingData(SAVE_NORMAL) != SAVE_STATUS_OK)
        gAuditResult[0] = 0;
    gAuditReady = 1;
}
'''


def check(game, phase):
    for _ in range(1800):
        game.frames(1)
        if game.u32(game.sym['gAuditReady']):
            assert game.u32(game.sym['gAuditPhase']) == phase
            values = struct.unpack('<15I', game.read(game.sym['gAuditResult'], 60))
            assert values == (1,)*15, (phase, values)
            return
    raise AssertionError(f'Friendship storage fixture timed out in phase {phase}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='polished-friendship-storage-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, CODE)
        game = Game(args.driver.resolve(), elf, elf, output, directory)
        try:
            check(game, 0)
            game.command('reset')
            check(game, 1)
        finally:
            game.close()
    report = dict(source_commit=PIN, fixture_only=True, checks=30, resets=1,
        level_one_exp=0, friendship=[162,107,150],
        production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        complete_behavior_parity=False)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: 30 friendship/growth checks across a Flash save/reset/load cycle', flush=True)


if __name__ == '__main__':
    main()
