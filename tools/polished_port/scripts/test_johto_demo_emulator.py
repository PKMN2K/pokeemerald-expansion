#!/usr/bin/env python3
"""Exercise a freshly booted Johto demo ROM using real buttons in mGBA.

Requires a compiled mgba_johto_driver, the demo GBA/ELF pair and arm-none-eabi-nm.
No ROM/RAM patches, injected scripts, or borrowed save files are used.
"""
import argparse
import hashlib
import json
import os
import re
import select
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path

from stage_johto_time_palettes import stage, load_source_palettes
from stage_johto_roof_palettes import load_roof_tables, roof_colors, apply_roof_colors
from stage_johto_animation_frames import build_frames

A, B, START, RIGHT, LEFT, UP, DOWN = 1, 2, 8, 16, 32, 64, 128
ROOT = Path(__file__).resolve().parents[3]


def symbols(elf):
    lines = subprocess.check_output(["arm-none-eabi-nm", "-n", str(elf)], text=True).splitlines()
    return {p[2]: int(p[0], 16) for line in lines if len(p := line.split()) == 3}


def png_from_ppm(path):
    raw = path.read_bytes().split(b"\n", 3)[3]
    assert len(raw) == 240 * 160 * 3
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 240, 160, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(b"".join(b"\0" + raw[y * 720:(y + 1) * 720] for y in range(160))))
    png += chunk(b"IEND", b"")
    path.with_suffix(".png").write_bytes(png)
    path.unlink()


class Game:
    def __init__(self, driver, rom, elf, output, tmp):
        self.sym = symbols(elf)
        self.output, self.tmp = output, tmp
        self.process = subprocess.Popen([str(driver), str(rom)], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, text=True, env={**os.environ, "TZ": "UTC"})
        self.timestamp = 1793786400

    def command(self, line):
        self.process.stdin.write(line + "\n")
        self.process.stdin.flush()
        if not select.select([self.process.stdout], [], [], 20)[0]:
            raise TimeoutError(f"mGBA stalled: {line}")
        result = self.process.stdout.readline().strip()
        assert result and result.split()[0] == line.split()[0], (line, result)
        return result

    def read(self, address, count):
        return bytes.fromhex(self.command(f"read {address:x} {count}")[5:])

    def u32(self, address):
        return struct.unpack("<I", self.read(address, 4))[0]

    def frames(self, count=20, keys=0):
        self.command(f"frames {count} {keys}")

    def position(self):
        save = self.u32(self.sym["gSaveBlock1Ptr"])
        return struct.unpack("<hh", self.read(save, 4))

    def map_number(self):
        save = self.u32(self.sym["gSaveBlock1Ptr"])
        return tuple(self.read(save + 4, 2))  # group, map

    def snapshot(self, name):
        self.command(f"save {self.tmp / (name + '.state')}")

    def restore(self, name):
        self.command(f"load {self.tmp / (name + '.state')}")
        self.frames(2)

    def screenshot(self, name):
        file = self.output / (name + ".ppm")
        self.command(f"shot {file}")
        png_from_ppm(file)

    def step(self, key):
        before = self.position()
        for _ in range(40):
            self.frames(1, key)
            if self.position() != before:
                self.frames(20)
                return
        raise AssertionError(f"Blocked at {before}, direction {key}")

    def go(self, x, y):
        for _ in range(60):
            px, py = self.position()
            if (px, py) == (x, y):
                return
            self.step((RIGHT if px < x else LEFT) if px != x else (DOWN if py < y else UP))
        raise AssertionError(f"Could not reach {(x, y)}: {self.position()}")

    def face(self, key):
        self.frames(1, key)
        self.frames(8)

    def interact(self, name):
        self.frames(1, A)
        self.frames(100)
        assert self.read(self.sym["sGlobalScriptContext"] + 1, 1) != b"\0", f"No interaction: {name}"
        self.screenshot(name)
        for _ in range(8):
            self.frames(1, A)
            self.frames(40)
            if self.read(self.sym["sGlobalScriptContext"] + 1, 1) == b"\0":
                return
        raise AssertionError(f"Dialogue did not release controls: {name}")

    def set_hour(self, hour):
        self.command(f"rtc {self.timestamp}")
        self.refresh_clock()
        current = self.read(self.sym["gLocalTime"] + 2, 1)[0]
        self.timestamp += ((hour - current) % 24) * 3600
        self.command(f"rtc {self.timestamp}")
        self.refresh_clock()
        actual = self.read(self.sym["gLocalTime"] + 2, 1)[0]
        assert actual == hour, f"RTC did not reach {hour}: got {actual}"

    def refresh_clock(self):
        # Wait for the engine's normal RTC poll; do not overwrite its counter.
        counter = struct.unpack("<h", self.read(self.sym["gTimeUpdateCounter"], 2))[0]
        assert 0 <= counter <= 3600
        self.frames(counter + 5)

    def close(self):
        self.process.stdin.write("quit\n")
        self.process.stdin.flush()
        self.process.wait(timeout=10)
        assert self.process.returncode == 0


def run(args):
    args.output.mkdir(parents=True, exist_ok=True)
    checks = []
    with tempfile.TemporaryDirectory(prefix="johto-emulator-") as tmp:
        game = Game(args.driver.resolve(), args.rom.resolve(), args.elf.resolve(), args.output.resolve(), Path(tmp))
        try:
            game.frames(1400)
            for key in [START] + [A] * 12:
                game.frames(1, key)
                game.frames(240)
                if game.u32(game.sym["gMain"] + 4) == game.sym["CB2_Overworld"] | 1:
                    break
            else:
                raise AssertionError("Demo did not boot into the overworld")
            assert game.position() == (6, 8)
            group, number = game.map_number()
            assert number == 0
            checks.append("new game boots into the Johto courtyard")
            palettes = stage(load_source_palettes())[0]
            roofs = load_roof_tables()
            for hour, period in [(10, "day"), (7, "morn"), (19, "eve"), (23, "nite"), (10, "day")]:
                game.set_hour(hour)
                expected = apply_roof_colors(palettes[f"standard.{period}"], roof_colors(roofs, period, map_group=24)[0])
                assert game.read(game.sym["gPlttBufferUnfaded"], 224) == expected, f"Wrong {period} palette"
                game.screenshot("courtyard-" + period)
            checks.append("all four live RGB555 palettes match the pinned source and roof group")
            game.snapshot("courtyard")
            # Read the actual animated tile in VRAM; it must cycle after clock changes.
            water = {game.read(0x06000000 + 149 * 32, 32)}
            for _ in range(8):
                game.frames(22)
                water.add(game.read(0x06000000 + 149 * 32, 32))
            # The source has four frames, two of which have identical pixels.
            assert water == set(build_frames()["water"])
            checks.append("authentic water animation reaches VRAM")
            game.frames(180, LEFT)
            game.frames(20)
            assert game.position() == (2, 8), game.position()
            game.frames(60, LEFT)
            assert game.position() == (2, 8)
            game.go(3, 11)
            obj = game.read(game.sym["gPlayerAvatar"] + 5, 1)[0]
            assert game.read(game.sym["gObjectEvents"] + obj * 36 + 30, 1) == b"\x02"
            game.screenshot("grass")
            checks.append("tree collision blocks movement; grass remains walkable and has its native effect")
            game.go(3, 6)
            game.face(UP)
            game.interact("sign")
            game.go(7, 9)
            game.go(8, 9)
            game.face(UP)
            game.interact("npc")
            checks.append("sign and authentic NPC dialogue return control")
            game.restore("courtyard")
            game.frames(60, UP)
            game.frames(180)
            assert game.map_number() == (group, 1)
            assert game.position() == (4, 11)
            game.go(4, 8)
            game.screenshot("laboratory")
            game.go(4, 10)
            game.frames(60, DOWN)
            game.frames(180)
            assert game.map_number() == (group, 0)
            assert game.position() == (6, 6)
            game.screenshot("door-return")
            checks.append("door enters the original lab layout and its exit returns outside")
            game.restore("courtyard")
            game.go(6, 16)
            game.go(18, 16)
            game.go(18, 15)
            game.face(UP)
            game.frames(1, A)
            for _ in range(10):
                game.frames(60)
                game.frames(1, A)
                if game.read(game.sym["gPlayerAvatar"], 1)[0] & 8:
                    break
            game.frames(180)
            assert game.read(game.sym["gPlayerAvatar"], 1)[0] & 8, "Surf did not start"
            game.screenshot("surf")
            game.frames(100, DOWN)
            game.frames(40)
            assert not game.read(game.sym["gPlayerAvatar"], 1)[0] & 8, "Surf did not end on shore"
            checks.append("Surf starts on source water and returns to walking at the shoreline")
        finally:
            game.close()
    report = {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(), "checks": checks}
    (args.output / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    for check in checks:
        print("PASS:", check)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args())
