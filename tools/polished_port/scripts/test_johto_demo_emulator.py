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
from build_johto_demo import palette_rows, source_files

A, B, START, RIGHT, LEFT, UP, DOWN = 1, 2, 8, 16, 32, 64, 128
ROOT = Path(__file__).resolve().parents[3]


def symbols(elf):
    lines = subprocess.check_output(["arm-none-eabi-nm", "-n", str(elf)], text=True).splitlines()
    result = {}
    for line in lines:
        fields = line.split()
        if len(fields) != 3:
            continue
        address, kind, name = fields
        value = int(address, 16)
        result[name] = value
        # Several controller translation units use the same static function
        # name. Preserve all addresses for state-aware emulator input.
        result.setdefault(name + "@all", []).append(value)
    return result


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
        # Set the cartridge's RTC before boot/New Game seeds the native RNG.
        # A host wall-clock seed made otherwise identical input replays differ.
        self.command(f"rtc {self.timestamp}")

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

    def check_sprite_palettes(self, expected):
        tags = struct.unpack("<16H", self.read(self.sym["sSpritePaletteTags"], 32))
        constants = (ROOT / "include/constants/event_objects.h").read_text()
        for name in ("POLISHED_CHRIS", "POLISHED_POKEFAN"):
            tag = int(re.search(r"#define OBJ_EVENT_PAL_TAG_" + name + r"\s+(0x[0-9A-Fa-f]+)", constants)[1], 16)
            assert tag in tags, f"Imported sprite palette missing: {name}"
            offset = (16 + tags.index(tag)) * 32
            for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
                actual = self.read(self.sym[buffer] + offset, 32)
                if actual != expected:
                    self.screenshot("palette-mismatch")
                    raise AssertionError(f"Wrong {name} {buffer}: {actual.hex()} != {expected.hex()}")

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

    def player_images(self):
        sprite = self.read(self.sym["gPlayerAvatar"] + 4, 1)[0]
        return self.u32(self.sym["gSprites"] + sprite * 68 + 12)

    def press(self, key, wait=40):
        self.frames(1, key)
        self.frames(wait)

    def use_key_item(self, index):
        self.press(START)
        # Follow the real menu's current cursor; visits preserve its position.
        actions = self.read(self.sym["sCurrentStartMenuActions"], 9)
        bag = actions.index(2)  # MENU_ACTION_BAG
        for _ in range(9):
            if self.read(self.sym["sStartMenuCursorPos"], 1)[0] == bag:
                break
            self.press(DOWN)
        else:
            raise AssertionError("Could not select the Bag")
        self.press(A, 120)
        for _ in range(5):
            if self.read(self.sym["gBagPosition"] + 5, 1)[0] == 4:  # POCKET_KEY_ITEMS
                break
            self.press(RIGHT)
        else:
            raise AssertionError("Could not select Key Items")
        for _ in range(5):
            cursor = struct.unpack("<H", self.read(self.sym["gBagPosition"] + 16, 2))[0]
            if cursor == index:
                break
            self.press(DOWN if cursor < index else UP)
        else:
            raise AssertionError("Could not select the supplied key item")
        self.press(A)
        self.press(A, 120)  # USE

    def wait_player_images(self, name):
        for _ in range(100):
            if (self.u32(self.sym["gMain"] + 4) == self.sym["CB2_Overworld"] | 1
                    and self.player_images() == self.sym[name]):
                return
            self.frames(5)
        raise AssertionError(f"Player did not use {name}")

    def finish_fishing(self, restored_images):
        for _ in range(24):
            self.press(B, 60)
            if (self.player_images() == self.sym[restored_images]
                    and self.read(self.sym["gPlayerAvatar"] + 6, 1) == b"\0"):
                self.frames(60)
                return
        raise AssertionError("Fishing did not restore the player graphics")

    def check_fishing_alignment(self, surfing=False):
        # The converted body starts at (8,8) inside a 32x32 sheet, eight pixels
        # above the normal walking/surfing anchor. Native Brendan frame shifts
        # must not move these identical Crystal poses around during casting.
        for _ in range(28):
            sprite = self.read(self.sym["gPlayerAvatar"] + 4, 1)[0]
            x2, y2 = struct.unpack("<hh", self.read(self.sym["gSprites"] + sprite * 68 + 36, 4))
            if surfing:
                obj = self.read(self.sym["gPlayerAvatar"] + 5, 1)[0]
                blob = self.read(self.sym["gObjectEvents"] + obj * 36 + 26, 1)[0]
                bob = struct.unpack("<h", self.read(self.sym["gSprites"] + blob * 68 + 38, 2))[0]
                y2 -= bob
            assert (x2, y2) == (0, 8), f"Crystal fishing body shifted: {(x2, y2)}"
            self.frames(1)

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
            sprite_rows = palette_rows(source_files()["gfx/overworld/npc_sprites.pal"].decode())
            sprite_palettes = {period: struct.pack("<16H", *sprite_rows[index * 20], *([0] * 12))
                               for index, period in enumerate(("morn", "day", "nite", "eve"))}
            for hour, period in [(10, "day"), (7, "morn"), (19, "eve"), (23, "nite"), (10, "day")]:
                game.set_hour(hour)
                expected = apply_roof_colors(palettes[f"standard.{period}"], roof_colors(roofs, period, map_group=24)[0])
                for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
                    assert game.read(game.sym[buffer], 224) == expected, f"Wrong {period} {buffer}"
                game.check_sprite_palettes(sprite_palettes[period])
                game.screenshot("courtyard-" + period)
            checks.append("all four live RGB555 palettes match the pinned source and roof group")
            checks.append("Chris and NPC use exact source OBJ colors in every period without double tinting")
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
            game.set_hour(23)
            game.check_sprite_palettes(sprite_palettes["day"])
            game.screenshot("laboratory")
            game.go(4, 10)
            game.frames(60, DOWN)
            game.frames(180)
            assert game.map_number() == (group, 0)
            assert game.position() == (6, 6)
            game.check_sprite_palettes(sprite_palettes["nite"])
            expected = apply_roof_colors(palettes["standard.nite"], roof_colors(roofs, "nite", map_group=24)[0])
            for buffer in ("gPlttBufferUnfaded", "gPlttBufferFaded"):
                assert game.read(game.sym[buffer], 224) == expected, f"Nighttime door tint: {buffer}"
            game.screenshot("door-return")
            checks.append("door enters the original lab layout and its exit returns outside")
            checks.append("nighttime door transitions restore indoor day and outdoor night OBJ palettes")
            game.restore("courtyard")
            game.use_key_item(1)  # Mach Bike, after the Old Rod in this fresh bag
            game.wait_player_images("sPicTable_PolishedChrisBike")
            assert game.read(game.sym["gPlayerAvatar"], 1)[0] & 2
            game.step(DOWN)
            game.screenshot("bike")
            game.use_key_item(1)
            game.wait_player_images("sPicTable_PolishedChrisNormal")
            assert not game.read(game.sym["gPlayerAvatar"], 1)[0] & 6
            checks.append("native bike use selects Chris's cycling graphics and restores walking")
            game.restore("courtyard")
            game.go(6, 16)
            game.go(18, 16)
            game.go(18, 15)
            game.face(UP)
            game.use_key_item(0)
            game.wait_player_images("sPicTable_PolishedChrisFishing")
            game.check_fishing_alignment()
            game.screenshot("land-fishing")
            game.finish_fishing("sPicTable_PolishedChrisNormal")
            assert game.position() == (18, 15)
            game.frames(1, A)
            for _ in range(10):
                game.frames(60)
                game.frames(1, A)
                if game.read(game.sym["gPlayerAvatar"], 1)[0] & 8:
                    break
            game.frames(180)
            assert game.read(game.sym["gPlayerAvatar"], 1)[0] & 8, "Surf did not start"
            game.screenshot("surf")
            game.go(18, 12)
            for facing, direction in (("east", RIGHT), ("south", DOWN), ("west", LEFT), ("north", UP)):
                game.face(direction)
                game.frames(20)
                game.use_key_item(0)
                game.wait_player_images("sPicTable_PolishedChrisSurfFishing")
                game.check_fishing_alignment(surfing=True)
                game.screenshot("surf-fishing-" + facing)
                game.finish_fishing("sPicTable_PolishedChrisSurfing")
                assert game.read(game.sym["gPlayerAvatar"], 1)[0] & 8, "Fishing lost Surf state"
            checks.append("land and surf fishing use their separate authentic sheets and restore the prior player state")
            checks.append("all four fishing directions preserve the Crystal body anchor and Surf bobbing")
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
