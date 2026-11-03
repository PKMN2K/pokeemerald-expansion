#!/usr/bin/env python3
"""Run the dormant production Headbutt C helpers against pinned source rows.

Builds extracted production functions on the host, not a second reimplementation.
Does not register a field move, touch game RNG, or create wild encounters.
"""
import ctypes
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BADGE_BASE = (4, 8, 12, 16, 20, 24, 28, 32, 36,
              40, 43, 46, 49, 52, 55, 58, 61)
FUNCTIONS = (
    ("s16", "JohtoHeadbuttSelectWeightedEntry"),
    ("bool8", "JohtoHeadbuttResolveSourceEncounter"),
    ("bool8", "JohtoHeadbuttGetBadgeBaseLevel"),
    ("bool8", "JohtoHeadbuttResolveLevel"),
    ("bool8", "JohtoHeadbuttResolveLevelFromBadgeCount"),
)


class SourceRow(ctypes.Structure):
    _fields_ = [("weight", ctypes.c_uint8),
                ("sourceSpecies", ctypes.c_char_p),
                ("sourceForm", ctypes.c_char_p),
                ("sourceLevelExpression", ctypes.c_char_p)]


def extract_native_code():
    cpp = (ROOT / "src/field_move.c").read_text(encoding="utf-8")
    hdr = (ROOT / "include/field_move.h").read_text(encoding="utf-8")
    structures = []
    for marker in ("struct JohtoHeadbuttSourceEncounter\n{",
                   "enum JohtoHeadbuttLevelMode\n{"):
        structures.append(hdr.split(marker, 1)[1].split("\n};", 1)[0])
    prefix = ("#include <stddef.h>\n#include <stdint.h>\n"
              "typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef int32_t s32;\n"
              "typedef uint8_t bool8;\n#define TRUE 1\n#define FALSE 0\n"
              "#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))\n"
              "struct JohtoHeadbuttSourceEncounter\n{"
              + structures[0] + "\n};\n"
              "enum JohtoHeadbuttLevelMode\n{" + structures[1] + "\n};\n")
    snippets = []
    for result_type, name in FUNCTIONS:
        marker = result_type + " " + name + "("
        assert cpp.count(marker) == 1, name
        body = cpp.split(marker, 1)[1].split("\n}\n", 1)[0]
        snippets.append(marker + body + "\n}\n")
    return prefix + "\n".join(snippets)


def run(encounter_report):
    with tempfile.TemporaryDirectory(prefix="johto_headbutt_native_") as temp:
        path = Path(temp)
        source = path / "helpers.c"
        library = path / "helpers.so"
        source.write_text(extract_native_code(), encoding="utf-8")
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-shared", "-fPIC", str(source), "-o", str(library)],
                       check=True, text=True, timeout=60)
        c = ctypes.CDLL(str(library))
        c.JohtoHeadbuttSelectWeightedEntry.argtypes = (
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint8, ctypes.c_uint8)
        c.JohtoHeadbuttSelectWeightedEntry.restype = ctypes.c_int16
        c.JohtoHeadbuttResolveSourceEncounter.argtypes = (
            ctypes.POINTER(SourceRow), ctypes.c_uint8, ctypes.c_uint8,
            ctypes.POINTER(SourceRow))
        c.JohtoHeadbuttResolveSourceEncounter.restype = ctypes.c_uint8
        c.JohtoHeadbuttResolveLevelFromBadgeCount.argtypes = (
            ctypes.c_int, ctypes.c_int16, ctypes.c_uint8,
            ctypes.POINTER(ctypes.c_uint8))
        c.JohtoHeadbuttResolveLevelFromBadgeCount.restype = ctypes.c_uint8

        groups_seen = set()
        selection_cases = 0
        level_cases = 0
        dynamic_cases = 0
        for group in encounter_report["source_tree_groups"]:
            if group["is_rock_smash_table"]:
                continue
            key = group["effective_table_label"]
            if key in groups_seen:
                continue
            groups_seen.add(key)
            for entries in (group["normal"], group["rare"]):
                assert entries and sum(e["weight_percent"] for e in entries) == 100
                weights = (ctypes.c_uint8 * len(entries))(
                    *(e["weight_percent"] for e in entries))
                rows = (SourceRow * len(entries))(*(
                    SourceRow(e["weight_percent"],
                              e["source_species"].encode("ascii"),
                              (e["source_form"] or "").encode("ascii"),
                              e["source_level_expression"].encode("ascii"))
                    for e in entries))
                expected = []
                for idx, entry in enumerate(entries):
                    expected.extend([idx] * entry["weight_percent"])
                assert len(expected) == 100
                for roll, index in enumerate(expected):
                    assert c.JohtoHeadbuttSelectWeightedEntry(
                        weights, len(entries), roll) == index
                    out = SourceRow()
                    assert c.JohtoHeadbuttResolveSourceEncounter(
                        rows, len(entries), roll, ctypes.byref(out))
                    entry = entries[index]
                    assert out.weight == entry["weight_percent"]
                    assert out.sourceSpecies.decode("ascii") == entry["source_species"]
                    assert out.sourceForm.decode("ascii") == (entry["source_form"] or "")
                    assert out.sourceLevelExpression.decode("ascii") == entry["source_level_expression"]
                    selection_cases += 1
                    expression = entry["source_level_expression"]
                    if entry["fixed_level"] is None:
                        match = re.fullmatch(r"LEVEL_FROM_BADGES\s*([+-])\s*(\d+)", expression)
                        assert match, expression
                        offset = int(match.group(2)) * (1 if match.group(1) == "+" else -1)
                        mode, value = 1, offset
                    else:
                        mode, value = 0, entry["fixed_level"]
                    for badge, base in enumerate(BADGE_BASE):
                        level = ctypes.c_uint8(0)
                        assert c.JohtoHeadbuttResolveLevelFromBadgeCount(
                            mode, value, badge, ctypes.byref(level))
                        expected_level = (max(2, min(99, base + value))
                                          if mode else value)
                        assert level.value == expected_level, (
                            key, expression, roll, badge, level.value, expected_level)
                        level_cases += 1
                        dynamic_cases += bool(mode)
        assert len(groups_seen) == 8
        assert selection_cases == 1600 and level_cases == 27200
        assert dynamic_cases > 0
        # Invalid tables/rolls and badge counts must fail closed.
        assert c.JohtoHeadbuttSelectWeightedEntry(weights, len(entries), 100) == -1
        assert not c.JohtoHeadbuttResolveSourceEncounter(
            rows, len(entries), 100, ctypes.byref(SourceRow()))
        invalid = ctypes.c_uint8(99)
        assert not c.JohtoHeadbuttResolveLevelFromBadgeCount(
            1, 0, 17, ctypes.byref(invalid))
        assert invalid.value == 99
        return {"native_c_weighted_selection_cases": selection_cases,
                "native_c_badge_level_cases": level_cases,
                "native_c_badge_relative_cases": dynamic_cases,
                "native_c_compiled_helpers_executed": True,
                "native_c_invalid_input_guards_checked": True,
                "native_c_rom_encounters_enabled": False}
