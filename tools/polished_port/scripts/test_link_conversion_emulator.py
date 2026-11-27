#!/usr/bin/env python3
"""Play Conversion on two production-linked GBA ROMs over mGBA's serial cable."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from audit_moves_machines import ROOT, compile_probe
from build_pokemon_learnsets import PIN
from test_johto_demo_emulator import A, DOWN, LEFT, RIGHT, UP, symbols

TEST_DIR = ROOT / "tools/polished_port/tests"


def build_driver(directory):
    driver = directory / "link-driver"
    # Ubuntu and Debian can build libmgba with different assertion settings.
    for flags in ([], ["-DNDEBUG"]):
        subprocess.run(["gcc", "-std=gnu11", "-O2", "-Wall", "-Wextra", "-Werror", *flags,
                        str(TEST_DIR / "mgba_link_driver.c"), "-lmgba", "-o", str(driver)], check=True)
        if subprocess.run([str(driver), "--abi"]).returncode == 0:
            return driver
    raise ValueError("mGBA serial header does not match the installed library")


class CableBattle:
    def __init__(self, directory):
        driver = build_driver(directory)
        fixture = (TEST_DIR / "link_conversion_fixture.c").read_text()
        carts = []
        for player in range(2):
            target = directory / str(player)
            target.mkdir()
            carts.append(compile_probe(ROOT, target, f"#define PROBE_TRAINER_ID {100 + player}\n" + fixture))
        self.sym = [symbols(elf) for elf in carts]
        self.proc = subprocess.Popen([str(driver), *map(str, carts)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     text=True, bufsize=1)
        (self.mon_size, self.types, self.pp, self.buffer_a, self.link_size, self.protocol,
         self.fire, self.water, self.choose_move, self.marker) = struct.unpack("<10I", self.read(0, "gLinkProbeOffsets", 40))

    def command(self, line):
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()
        reply = self.proc.stdout.readline().split()
        if not reply:
            raise RuntimeError(f"Serial driver stopped during {line}")
        return reply

    def read(self, player, address, size):
        if isinstance(address, str):
            address = self.sym[player][address]
        return bytes.fromhex(" ".join(self.command(f"read {player} {address:x} {size}")[1:]))

    def frames(self, count, keys):
        self.command(f"frames {count} {keys[0]} {keys[1]}")

    def close(self):
        self.proc.stdin.close()
        if self.proc.wait(timeout=10):
            raise RuntimeError("Serial driver failed")

    def menu(self, player):
        names = self.sym[player]
        controllers = struct.unpack("<4I", self.read(player, "gBattlerControllerFuncs", 16))
        for battler, fn in enumerate(controllers):
            if any(fn == addr | 1 for addr in names["HandleInputChooseAction@all"]):
                return "action", battler
            if any(fn == addr | 1 for addr in names["HandleInputChooseMove@all"]):
                resources = int.from_bytes(self.read(player, "gBattleResources", 4), "little")
                packet = self.read(player, resources + self.buffer_a + battler * 0x200, 4)
                return ("type" if packet[0] == self.choose_move and packet[3] else "move"), battler
        return None, None

    def select(self, player, battler, menu, target):
        cursor = "gActionSelectionCursor" if menu == "action" else "gMoveSelectionCursor"
        current = self.read(player, self.sym[player][cursor] + battler, 1)[0]
        if current == target:
            return A
        if (current ^ target) & 1:
            return RIGHT if target & 1 else LEFT
        return DOWN if target & 2 else UP

    def move_pp(self):
        return [list(self.read(0, self.sym[0]["gBattleMons"] + i * self.mon_size + self.pp, 4)) for i in range(2)]

    def play(self):
        for tick in range(1000):
            menus = [self.menu(i) for i in range(2)]
            if any(menu == "action" for menu, _ in menus):
                break
            self.frames(4, [A if tick % 4 == 0 and menu is None else 0 for menu, _ in menus])
        else:
            raise AssertionError("Both trainers failed to reach the cable battle")
        for player in range(2):
            peers = self.read(player, "gLinkPlayers", 2 * self.link_size)
            assert list(peers[self.protocol::self.link_size]) == [self.marker, self.marker]
            assert self.read(player, "sPolishedLinkChoices", 1) == b"\x01"

        tries = [0, 0]
        chosen = [False, False]
        pressed = [False, False]
        next_turn = False
        tackle_sent = [False, False]
        for tick in range(5000):
            menus = [self.menu(i) for i in range(2)]
            if all(chosen) and not next_turn and any(menu == "action" for menu, _ in menus):
                # The master owns the battle engine. Both ROMs run their own input controllers.
                battle_types = [self.read(0, self.sym[0]["gBattleMons"] + i * self.mon_size + self.types, 1)[0]
                                for i in range(2)]
                assert battle_types == [self.fire, self.water], battle_types
                assert tries == [1, 2], tries
                before = self.move_pp()
                next_turn = True
            if next_turn and all(tackle_sent):
                after = self.move_pp()
                if all(after[i][3] + 1 == before[i][3] for i in range(2)):
                    return dict(protocol=self.marker, choices=tries, types=battle_types,
                                tackle_pp_before=[row[3] for row in before], tackle_pp_after=[row[3] for row in after])
            keys = [0, 0]
            for player, (menu, battler) in enumerate(menus):
                if pressed[player]:
                    pressed[player] = False
                    continue
                if menu == "type":
                    target = 0 if player == 1 and tries[player] == 0 else player + 1
                elif menu == "move":
                    target = 3 if next_turn else 0
                elif menu == "action":
                    target = 0
                else:
                    keys[player] = A if tick % 8 == 0 else 0
                    pressed[player] = bool(keys[player])
                    continue
                keys[player] = self.select(player, battler, menu, target)
                if keys[player] == A and menu == "type":
                    tries[player] += 1
                    chosen[player] = target != 0
                elif keys[player] == A and menu == "move" and next_turn:
                    tackle_sent[player] = True
                pressed[player] = True
            self.frames(2, keys)
        raise AssertionError(("Cable battle did not reach the next two Tackle actions", tries, tackle_sent))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="polished-link-") as directory:
        battle = CableBattle(Path(directory))
        try:
            report = battle.play()
        finally:
            battle.close()
    report.update(source_commit=PIN, production_rom_sha256=hashlib.sha256((ROOT / "pokeemerald-johto-demo.gba").read_bytes()).hexdigest())
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: two linked GBAs negotiated Conversion, retried the guest selection and spent both Tackle PP")


if __name__ == "__main__":
    main()
