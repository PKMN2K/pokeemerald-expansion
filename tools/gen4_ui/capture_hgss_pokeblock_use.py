#!/usr/bin/env python3
"""Exercise real Pokéblock case, recipient, feeding and results in mGBA."""
import json
import subprocess
import sys
from pathlib import Path


def capture(walker, rom, out):
    out.mkdir(parents=True, exist_ok=True)
    actions = []
    audits = {}

    def boot(key=None):
        actions.clear()
        if key:
            actions.append(f'press:{key}:90')
        actions.append('run:700')

    def press(key, frames=180):
        actions.extend((f'press:{key}:1', f'run:{frames}'))

    def snap(name):
        actions.append(f'snap:{name}')

    def audit(name):
        actions.append(f'savestate:{name}')

    def run():
        subprocess.run([walker, rom, str(out), *actions], check=True, timeout=180)
        for path in out.glob('audit_*.ss'):
            data = path.read_bytes()
            # mGBA's uncompressed state contains WRAM at 0x21000..0x61000.
            marker = data.find(b'HGSSFEED', 0x21000, 0x61000)
            assert marker >= 0 and data.find(b'HGSSFEED', marker + 1, 0x61000) < 0
            values = list(data[marker + 8:marker + 50])
            audits[path.stem] = {'returns': values[0], 'blocks': values[1],
                                 'party': [values[2+i*6:8+i*6] for i in range(6)],
                                 'colors': values[38:42]}
            path.unlink()  # No fixture save states are published.
        (out / 'audit.json').write_text(json.dumps(audits, indent=2) + '\n')

    def enter():
        press('A', 160); press('A', 360)

    def feed(name, count):
        press('A'); snap(name + '_confirm')
        press('A', 1600); snap(name + '_feeding')
        press('A', 420); snap(name + '_before')  # Acknowledge actual feeding message.
        press('A', 25); snap(name + '_arrows')
        actions.append('run:90'); snap(name + '_result')
        if name == 'capped':
            for i in range(32):
                actions.append('run:8'); snap(f'sheen_{i:02d}')
        for i in range(count - 1):
            press('A', 30); snap(f'{name}_message_{i+2}')
        press('A', 360); snap(name + '_return'); audit('audit_' + name)

    boot(); snap('case_initial'); enter(); snap('first')
    press('UP'); snap('wrap_cancel'); press('DOWN'); snap('wrap_first')
    for name in ('second', 'genderless', 'full_sheen', 'last'):
        press('DOWN'); snap(name)
    press('A'); snap('last_confirm_yes'); press('DOWN'); snap('last_confirm_no')
    press('A'); snap('last_declined'); press('A'); press('B'); snap('last_b_dismissed')
    press('UP'); press('A'); press('A'); snap('refused')
    press('B'); snap('refused_dismissed'); press('B', 360); snap('browse_return'); audit('audit_cancel')
    enter(); feed('neutral', 5)
    enter(); press('DOWN'); feed('liked', 1)
    enter(); feed('unchanged', 1)
    enter(); press('DOWN'); press('DOWN'); feed('capped', 5)
    run()

    boot(); enter(); press('DOWN'); feed('disliked', 5); run()

    boot(); enter(); press('UP'); press('UP'); feed('last_fed', 5); run()

    boot('L'); enter(); snap('single_first'); press('DOWN'); snap('single_cancel')
    press('DOWN'); snap('single_wrap'); press('UP'); press('A', 360); snap('single_return'); audit('audit_single'); run()

    boot('SELECT'); enter(); snap('empty_cancel'); press('UP'); snap('empty_up')
    press('DOWN'); snap('empty_down'); press('A', 360); snap('empty_return'); audit('audit_empty'); run()

    boot('R'); enter(); snap('six_first'); press('UP'); snap('six_cancel')
    press('UP'); snap('six_last'); press('DOWN'); press('A', 360); snap('six_return'); audit('audit_six'); run()


if __name__ == '__main__':
    if len(sys.argv) != 4:
        raise SystemExit('usage: capture_hgss_pokeblock_use.py GBA_WALK ROM OUTPUT_DIR')
    capture(sys.argv[1], sys.argv[2], Path(sys.argv[3]))
