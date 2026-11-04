#!/usr/bin/env python3
"""Extend the disposable search fixture for live Condition-detail acceptance.

Run only in a capture checkout; no patched runtime, ROM or save is committed.
Boot normally for five valid party mons plus an egg, R for 104 search results,
L for one party mon, or SELECT for only the party Cancel entry.
"""
import runpy
from pathlib import Path

runpy.run_path(str(Path(__file__).with_name("prepare_hgss_condition_search_capture.py")))
nav = Path("src/pokenav.c")
source = nav.read_text()
hook = "    gPartiesCount[B_TRAINER_PLAYER] = 5;\n    InitEventData();"
assert source.count(hook) == 1
source = source.replace(hook, r'''
    gPartiesCount[B_TRAINER_PLAYER] = 5;
    if (!largeList)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][5];
        CreateMon(mon, SPECIES_NIDORAN_M, 100, 0, OTID_STRUCT_PRESET(0));
        SetMonData(mon, MON_DATA_NICKNAME, COMPOUND_STRING("WWWWWWWWWWWW"));
        gPartiesCount[B_TRAINER_PLAYER] = 6;
        for (j = 0; j < ARRAY_COUNT(dataIds); j++)
        {
            value = 255;
            SetMonData(&gParties[B_TRAINER_PLAYER][0], dataIds[j], &value);
            value = 0;
            SetMonData(&gParties[B_TRAINER_PLAYER][1], dataIds[j], &value);
            value = 5;
            SetMonData(mon, dataIds[j], &value);
        }
        value = 255;
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SHEEN, &value);
        value = 6;
        SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MARKINGS, &value);
        value = 9;
        SetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_MARKINGS, &value);
        StringCopy(GetBoxNamePtr(0), COMPOUND_STRING("WWWWWWWW"));
    }
    if (JOY_HELD(L_BUTTON | SELECT_BUTTON))
    {
        u32 count = JOY_HELD(SELECT_BUTTON) ? 0 : 1;
        for (i = count; i < PARTY_SIZE; i++)
            ZeroMonData(&gParties[B_TRAINER_PLAYER][i]);
        gPartiesCount[B_TRAINER_PLAYER] = count;
    }
    InitEventData();''', 1)
nav.write_text(source)
print("Prepared disposable Condition detail fixture (party, marks, zero/max stats, sheen, 104 ranks)")
