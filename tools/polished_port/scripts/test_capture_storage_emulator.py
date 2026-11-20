#!/usr/bin/env python3
"""Exercise production capture delivery, form ownership and Flash save loading.

The fixture boot creates known captures through shipping APIs. Two real emulator
resets reload the Flash save. This is a storage integration probe, not a replay
of the capture menu/animation or every acquisition path.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from audit_moves_machines import ROOT, compile_probe
from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import Game, A


CODE = r'''
#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "polished_capture.h"
#include "save.h"

static const u16 species[] = {SPECIES_UNOWN_B, SPECIES_RATTATA_ALOLA,
    SPECIES_UNOWN_C, SPECIES_CLEFFA, SPECIES_WOBBUFFET, SPECIES_CATERPIE,
    SPECIES_RAICHU_ALOLA};
EWRAM_DATA volatile u32 gAuditResult[18] = {0};
EWRAM_DATA volatile u32 gAuditPhase = 0;
EWRAM_DATA volatile u32 gAuditReady = 0;
EWRAM_DATA volatile u32 gAuditProgress = 0;
EWRAM_DATA volatile u32 gAuditSpecies[7] = {0};
static EWRAM_DATA bool8 initialized = FALSE;

static void CheckState(bool32 migrated)
{
    gAuditResult[0] = gSaveBlock2Ptr->pokedex.unknown3 == 0x50434F31;
    gAuditResult[1] = gPartiesCount[B_TRAINER_PLAYER] == PARTY_SIZE;
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        gAuditSpecies[i] = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES);
        gAuditResult[2+i] = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == species[i];
    }
    gAuditSpecies[6] = GetBoxMonDataAt(0, 0, MON_DATA_SPECIES);
    gAuditResult[8] = GetBoxMonDataAt(0, 0, MON_DATA_SPECIES) == SPECIES_RAICHU_ALOLA;
    gAuditResult[9] = PolishedWasCaught(SPECIES_UNOWN) && PolishedWasCaught(SPECIES_UNOWN_D);
    gAuditResult[10] = PolishedWasCaught(SPECIES_RATTATA_ALOLA) == !migrated;
    gAuditResult[11] = PolishedWasCaught(SPECIES_RATTATA) == migrated;
    gAuditResult[12] = PolishedWasCaught(SPECIES_RAICHU_ALOLA) == !migrated;
    gAuditResult[13] = PolishedWasCaught(SPECIES_RAICHU) == migrated;
    gAuditResult[14] = GetSetPokedexFlag(NATIONAL_DEX_RATTATA, FLAG_GET_CAUGHT) != 0;
    gAuditResult[15] = GetSetPokedexFlag(NATIONAL_DEX_RAICHU, FLAG_GET_CAUGHT) != 0;
    gAuditResult[16] = PolishedWasCaught(SPECIES_PONYTA_GALAR) == migrated
        && !PolishedWasCaught(SPECIES_PONYTA);
    // Field access must not inspect a freed/null battle struct.
    gAuditResult[17] = !gMain.inBattle && gBattleStruct == NULL && !IsGen6ExpShareEnabled();
}

void CB2_MoveMachineAudit(void)
{
    if (!initialized)
    {
        gAuditProgress = 1;
        SetSaveBlocksPointers(0);
        ResetMenuAndMonGlobals();
        Save_ResetSaveCounters();
        Sav2_ClearSetDefault();
        MoveSaveBlocks_ResetHeap();
        gAuditProgress = 2;
        if (LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
        {
            bool32 migrated = PolishedWasCaught(SPECIES_PONYTA_GALAR);
            gAuditPhase = migrated ? 3 : 1;
            CheckState(migrated);
        }
        else
        {
            gAuditProgress = 3;
            NewGameInitData();
            ZeroPlayerPartyMons(); // Remove the demo boot party before fixture delivery.
            ResetPokedex();
            for (u32 i = 0; i < ARRAY_COUNT(species); i++)
            {
                gAuditProgress = 4+i;
                struct Pokemon mon;
                CreateMonWithIVs(&mon, species[i], 10, 0, OTID_STRUCT_PLAYER_ID, 20);
                GiveCapturedMonToPlayer(&mon);
                HandleSetPokedexFlagFromMon(&mon, FLAG_SET_SEEN);
                HandleSetPokedexFlagFromMon(&mon, FLAG_SET_CAUGHT);
            }
            CheckState(FALSE);
            gAuditProgress = 11;
            if (TrySavingData(SAVE_NORMAL) != SAVE_STATUS_OK) gAuditResult[0] = 0;
        }
        initialized = TRUE;
        gAuditReady = 1;
    }
    else if (gAuditPhase == 1 && (gMain.newKeys & A_BUTTON))
    {
        // Model the old save format: National Dex flags exist, source form
        // ownership does not. No historical variant ownership is invented.
        gSaveBlock2Ptr->pokedex.unknown3 = 0;
        memset(gSaveBlock2Ptr->pokedex.filler, 0, sizeof(gSaveBlock2Ptr->pokedex.filler));
        PolishedRecordCaughtSpecies(SPECIES_PONYTA_GALAR);
        CheckState(TRUE);
        if (TrySavingData(SAVE_NORMAL) != SAVE_STATUS_OK) gAuditResult[0] = 0;
        gAuditPhase = 2;
    }
}
'''


def read_phase(game, phase):
    for _ in range(1800):
        game.frames(1)
        if game.u32(game.sym['gAuditReady']) and game.u32(game.sym['gAuditPhase']) == phase:
            values = struct.unpack('<18I', game.read(game.sym['gAuditResult'], 72))
            assert values == (1,) * 18, (phase, values, struct.unpack('<7I', game.read(game.sym['gAuditSpecies'], 28)))
            return
    raise AssertionError(dict(phase=phase,
        progress=game.u32(game.sym['gAuditProgress']),
        results=game.read(game.sym['gAuditResult'], 72).hex(),
        species=game.read(game.sym['gAuditSpecies'], 28).hex()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='polished-capture-storage-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(ROOT, directory, CODE)
        game = Game(args.driver.resolve(), elf, elf, output, directory)
        try:
            read_phase(game, 0)
            game.command('reset')
            read_phase(game, 1)
            game.frames(1, A)
            game.frames(1)
            read_phase(game, 2)
            game.command('reset')
            read_phase(game, 3)
        finally:
            game.close()
    report = dict(source_commit=PIN, fixture_only=True, checks=72, resets=2, captures=7,
        party_captures=6, boxed_captures=1, legacy_migration=True,
        production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        complete_behavior_parity=False)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: 72 capture storage checks, two Flash save/reset/load cycles and legacy ownership migration', flush=True)


if __name__ == '__main__':
    main()
