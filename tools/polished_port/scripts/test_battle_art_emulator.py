#!/usr/bin/env python3
"""Read the shipping item and trainer renderers' tiles and palettes in mGBA."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from audit_moves_machines import ROOT, compile_probe, sample
from build_battle_art import build
from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import Game


def run(driver, output):
    output.mkdir(parents=True, exist_ok=True)
    outputs, items, portrait = build()
    for path, value in outputs.items():
        assert path.read_bytes() == value, path
    code = r'''
#include "global.h"
#include "data.h"
#include "decompress.h"
#include "graphics.h"
#include "item_icon.h"
#include "item.h"
#include "main.h"
#include "palette.h"
#include "malloc.h"
#include "sprite.h"
#include "constants/opponents.h"
static const enum Item icons[] = { ITEM_ROWS };
EWRAM_DATA volatile u32 gAuditReady = 0;
EWRAM_DATA volatile u32 gAuditKind = 0;
EWRAM_DATA volatile u32 gAuditCursor = 0;
EWRAM_DATA u32 gAuditResult[520] = {0};
void CB2_MoveMachineAudit(void)
{
    if (gAuditReady)
    {
        if (JOY_NEW(A_BUTTON)) { gAuditCursor++; gAuditReady = 0; }
        return;
    }
    if (gAuditCursor < ARRAY_COUNT(icons))
    {
        enum Item item = icons[gAuditCursor];
        if (gAuditCursor == 0)
        {
            InitHeap(gHeap, HEAP_SIZE);
            ResetSpriteData();
            FreeAllSpritePalettes();
        }
        u32 sprite = AddItemIconSprite(0x1234, 0x1234, item);
        if (sprite >= MAX_SPRITES) return;
        const void *tiles = (const u8 *)OBJ_VRAM0 + gSprites[sprite].oam.tileNum * TILE_SIZE_4BPP;
        memcpy(gAuditResult, tiles, 512);
        memcpy(&gAuditResult[128], &gPlttBufferUnfaded[OBJ_PLTT_ID(gSprites[sprite].oam.paletteNum)], 32);
        DestroySprite(&gSprites[sprite]);
        FreeSpriteTilesByTag(0x1234);
        FreeSpritePaletteByTag(0x1234);
    }
    else if (gAuditCursor == ARRAY_COUNT(icons))
    {
        enum TrainerPicID pic = GetTrainerStructFromId(TRAINER_POLISHED_GIOVANNI1)->trainerPic;
        DecompressDataWithHeaderWram(GetTrainerFrontPicData(pic), gAuditResult);
        memcpy(&gAuditResult[512], GetTrainerFrontPicPalette(pic), 32);
    }
    else
    {
        gAuditResult[0] = GetItemIconPic(ITEM_BEAST_BALL) == gItemsInfo[ITEM_BEAST_BALL].iconPic
            && GetItemIconPalette(ITEM_BEAST_BALL) == gItemsInfo[ITEM_BEAST_BALL].iconPalette;
        gAuditResult[1] = GetItemIconPic(ITEM_LIST_END) == gItemIcon_ReturnToFieldArrow
            && GetItemIconPalette(ITEM_LIST_END) == gItemIconPalette_ReturnToFieldArrow;
    }
    gAuditReady = 1;
}
'''.replace('ITEM_ROWS', ', '.join(item for item, _, _ in items))
    with tempfile.TemporaryDirectory(prefix='polished-battle-art-') as tmp:
        tmp = Path(tmp)
        elf = compile_probe(ROOT, tmp, code)
        game = Game(driver, elf, elf, output, tmp)
        try:
            for i, (name, tiles, colors) in enumerate(items + [('GIOVANNI1', *portrait)]):
                if name != 'GIOVANNI1':
                    # The bag uses a 32x32 OBJ with the 24x24 source at its top left.
                    tiles = b''.join(tiles[y:y+96] + bytes(32) for y in range(0, 288, 96)) + bytes(128)
                expected = tiles + colors
                observed = sample(game, 0, i, len(expected)//4)
                assert struct.pack('<'+'I'*len(observed), *observed) == expected, name
            assert sample(game, 0, len(items)+1, 2) == [1, 1]
        finally:
            game.close()
    print(f'PASS: all {len(items)} source item tile sheets and palettes and Giovanni portrait matched byte for byte')
    report = dict(source_commit=PIN, production_rom_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
                  verified_items=[name for name, _, _ in items], item_vram_checked=True, registered_trainer_portraits=['GIOVANNI1'], expansion_item_fallback=True)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--driver', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    run(args.driver.resolve(), args.output.resolve())
