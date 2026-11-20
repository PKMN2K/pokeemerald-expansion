#!/usr/bin/env python3
"""Verify source roamer HP/status survive the production Flash save path.

This fixture exercises the shipping persistence and save APIs. It does not
certify roaming encounters, routes or the complete battle-end lifecycle.
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
#include "pokemon.h"
#include "roamer.h"
#include "save.h"

EWRAM_DATA volatile u32 gAuditResult[6] = {0};
EWRAM_DATA volatile u32 gAuditReady = 0;
EWRAM_DATA volatile u32 gAuditPhase = 0;

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
        struct Pokemon mon;
        u32 status = STATUS1_TOXIC_POISON | 0x500;
        u32 hp = 300;
        NewGameInitData();
        CreateMonWithIVs(&mon, SPECIES_RAIKOU, 100, 0, OTID_STRUCT_PLAYER_ID, 31);
        SetMonData(&mon, MON_DATA_HP, &hp);
        SetMonData(&mon, MON_DATA_STATUS, &status);
        gSaveBlock1Ptr->roamer[0].species = SPECIES_RAIKOU;
        gSaveBlock1Ptr->roamer[0].level = 100;
        gSaveBlock1Ptr->roamer[0].active = TRUE;
        gEncounteredRoamerIndex = 0;
        UpdateRoamerHPStatus(&mon);
    }
    const struct Roamer *roamer = &gSaveBlock1Ptr->roamer[0];
    gAuditResult[0] = roamer->species == SPECIES_RAIKOU;
    gAuditResult[1] = roamer->level == 100;
    gAuditResult[2] = roamer->active;
    gAuditResult[3] = roamer->hp == 300;
    gAuditResult[4] = roamer->statusA == STATUS1_POISON;
    gAuditResult[5] = roamer->statusB == 0;
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
            values = struct.unpack('<6I', game.read(game.sym['gAuditResult'], 24))
            assert values == (1,)*6, (phase, values)
            return
    raise AssertionError(f'Roamer storage fixture timed out in phase {phase}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='polished-escape-storage-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, CODE)
        game = Game(args.driver.resolve(), elf, elf, output, directory)
        try:
            check(game, 0)
            game.command('reset')
            check(game, 1)
        finally:
            game.close()
    report = dict(source_commit=PIN, fixture_only=True, checks=12, resets=1,
        preserved_hp=300, toxic_saved_as_regular_poison=True,
        production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        complete_behavior_parity=False)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: 12 roamer HP/status checks across a Flash save/reset/load cycle', flush=True)


if __name__ == '__main__':
    main()
