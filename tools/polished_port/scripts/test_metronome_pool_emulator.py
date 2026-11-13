#!/usr/bin/env python3
"""Compare the production-linked Metronome adapter with every pinned source draw."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import tempfile

import build_pokemon_learnsets as source
import build_metronome_struggle as rules
from audit_moves_machines import compile_probe
from test_johto_demo_emulator import Game


def run(driver, output):
    # Read identities/exceptions from the pinned assembly, independently of the
    # generated C lookup table. Compile only this harness into production objects.
    files = source.sources()
    rows = source.move_identities(files)
    excluded = set(re.findall(r'^\s*db (\w+)', files['data/moves/metronome_exception_moves.asm'], re.M))
    for path, expected in rules.build().items():
        assert path.read_text() == expected, f'Stale source rules: {path}'
    code = '\n'.join([
        '#include "global.h"', '#include "polished_metronome.h"',
        'static const enum Move expected[] = {',
        *[(('MOVE_NONE' if r['source_constant'] in excluded else r['target_move']) + ',') for r in rows],
        '};', r'''
EWRAM_DATA volatile u32 gMetronomeProbeResult[6] = {0};
void CB2_MoveMachineAudit(void)
{
    if (gMetronomeProbeResult[0])
        return;
    enum Move known[MAX_MON_MOVES] = {MOVE_NONE};
    for (u32 roll = 0; roll < ARRAY_COUNT(expected); roll++)
    {
        enum Move move = PolishedMetronomeMoveFromRoll(roll, known);
        gMetronomeProbeResult[1]++;
        gMetronomeProbeResult[2] += move != expected[roll];
        if (expected[roll] == MOVE_NONE)
            continue;
        gMetronomeProbeResult[3]++;
        for (u32 slot = 0; slot < MAX_MON_MOVES; slot++)
        {
            known[slot] = expected[roll];
            gMetronomeProbeResult[4]++;
            gMetronomeProbeResult[5] += PolishedMetronomeMoveFromRoll(roll, known) != MOVE_NONE;
            known[slot] = MOVE_NONE;
        }
    }
    gMetronomeProbeResult[0] = 1;
}
'''])
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='metronome-pool-') as tmp:
        directory = Path(tmp)
        elf = compile_probe(source.ROOT, directory, code)
        game = Game(driver, elf, elf, output, directory)
        try:
            game.frames(300)
            result = list(struct.unpack('<6I', game.read(game.sym['gMetronomeProbeResult'], 24)))
            assert result == [1, 256, 0, 244, 976, 0], result
        finally:
            game.close()
    report = dict(source_commit=source.PIN, fixture_only=True,
        production_rom_sha256=hashlib.sha256((source.ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        source_draws=256, eligible_candidates=244, known_slot_checks=976, mismatches=0,
        scope='Production-linked identity and rejection adapter; battle dispatch/recoil have separate tests; source PRNG stream is not imported.')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: production-linked Metronome: 256 source draws, 244 candidates, 976 known-slot checks, zero mismatches', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--driver', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    run(args.driver.resolve(), args.output.resolve())
