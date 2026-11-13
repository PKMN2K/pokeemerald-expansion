#!/usr/bin/env python3
"""Check emulator ARM layout probes after `make -f make_tools.mk; make generated`."""
import ast
import importlib
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]


def run():
    source = ('#include "global.h"\n#include "move.h"\n'
              'const u32 profiles[] = {POLISHED_JOHTO_DEMO, B_POLISHED_MOVE_PP, B_POLISHED_RETURN, B_POLISHED_MOVE_BALANCE, B_POLISHED_HIDDEN_POWER};\n')
    command = ["arm-none-eabi-gcc", "-mcpu=arm7tdmi", "-mthumb", "-mabi=apcs-gnu",
               "-DMODERN=1", "-DEMERALD", "-std=gnu17", "-iquote", "include", "-iquote", ".",
               "-S", "-x", "c", "-o", "-", "-"]
    for profile, balance_test in ((None, 0), (0, 0), (1, 0), (0, 1)):
        defines = [] if profile is None else [f"-DPOLISHED_JOHTO_DEMO={profile}"]
        if balance_test:
            defines += ["-DPOLISHED_MOVE_BALANCE_TEST=1", "-DTESTING=1"]
        result = subprocess.run(command + defines, cwd=ROOT, input=source,
                                text=True, capture_output=True, check=True)
        values = list(map(int, re.findall(r"\.word\s+(\d+)", result.stdout)))
        # GCC emits an all-zero array as .space rather than five .word lines.
        if not values:
            assert re.search(r"profiles:\s*\n\s*\.space\s+20\b", result.stdout), result.stdout
            values = [0, 0, 0, 0, 0]
        assert values == [int(profile or 0)] * 3 + [int(bool(profile or balance_test))] * 2, (profile, balance_test, values)
    print("PASS: omitted/native/Johto and isolated source-balance profile selections compile correctly", flush=True)

    count = 0
    for path in sorted(Path(__file__).parent.glob("test_*_emulator.py")):
        for node in ast.parse(path.read_text()).body:
            if not isinstance(node, ast.FunctionDef) or not any(
                    isinstance(value, ast.Constant) and value.value == "arm-none-eabi-gcc"
                    for value in ast.walk(node)):
                continue
            assert not node.args.args and not node.args.kwonlyargs, (path.name, node.name)
            probe = getattr(importlib.import_module(path.stem), node.name)
            try:
                values = probe()
            except subprocess.CalledProcessError:
                print(f"FAIL: {path.name}:{node.name}", file=sys.stderr, flush=True)
                raise
            assert isinstance(values, dict) and values, (path.name, node.name)
            count += 1
            print(f"PASS: {path.name}:{node.name} ({len(values)} values)", flush=True)
    assert count >= 9, "Emulator compiler probes were not discovered"
    print(f"PASS: all {count} emulator compiler probes", flush=True)


if __name__ == "__main__":
    try:
        run()
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise
