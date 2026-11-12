#!/usr/bin/env python3
"""Exercise source TM31/TM51 with controller input and production-linked fixtures."""
import argparse
import hashlib
import json
from pathlib import Path
from test_tm70_emulator import ROOT, constants, run


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--driver', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--tm', type=int, choices=(31,51))
    p.add_argument('--mode', type=int, choices=range(7))
    a=p.parse_args(); c=constants(); results=[]
    for number in ([a.tm] if a.tm else (31,51)):
        for mode in ([a.mode] if a.mode is not None else range(7)):
            result=run(a.driver.resolve(),a.output.resolve()/str(number)/str(mode),mode,c,number)
            results.append(dict(tm=number, **result))
            print(f'PASS: TM{number} fixture {mode}: {result}', flush=True)
    (a.output/'report.json').write_text(json.dumps(dict(
        production_sha256=hashlib.sha256((ROOT/'pokeemerald-johto-demo.gba').read_bytes()).hexdigest(),
        fixture_only=True, results=results),indent=2)+'\n')
