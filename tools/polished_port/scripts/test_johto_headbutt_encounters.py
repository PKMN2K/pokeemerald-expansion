#!/usr/bin/env python3
"""Exercise the complete dormant production C selector against pinned source data."""
import copy
import ctypes
import hashlib
import re
import subprocess
import tempfile
from pathlib import Path

from export_johto_headbutt_encounter_tables import (
    ROOT, FIXTURES, RUNTIME_TABLES, load_sources, parse_encounters, parse_maps,
    render_runtime_tables, source_map_assignments, species_candidates,
)
from route_johto_palette import parse_source_constants


class Encounter(ctypes.Structure):
    _fields_ = [("species", ctypes.c_uint16), ("level", ctypes.c_uint8)]


def assert_rejected(pointers, maps, candidates):
    try:
        render_runtime_tables(pointers, maps, candidates)
    except ValueError as error:
        assert "Unresolved Headbutt" in str(error)
        return
    raise AssertionError("unresolved species/form mapping emitted C data")


def validate_audit(pointers, maps):
    # Git blob hashes from the unchanged v3.2.3 source pin.
    for name, sha in {
        "badge_levels": "97c7b04e139082c7eb0a44a7a911247fcaf7d9e9",
        "voltorb_hisuian": "d1e3cac52fe4c641115f8d3b0265cd18bff9c6c1",
        "electrode_hisuian": "dd245cc36ea5242cab2ce5564f252a7f382cacbb",
    }.items():
        raw = (FIXTURES / f"johto_headbutt_{name}.asm").read_bytes()
        assert hashlib.sha1(b"blob " + str(len(raw)).encode() + b"\0" + raw).hexdigest() == sha
    constants = (ROOT / "include/constants/species.h").read_text()
    candidates = species_candidates(pointers)
    assert len(candidates) == 25
    assert all(c["base_constant"] and not c["unresolved_forms"] for c in candidates)
    # Regression for the overescaped regex: empty matches must be visible in
    # the report, not an assertion that destroys the diagnostic artifact.
    missing = species_candidates(pointers, "")
    assert len(missing) == 25 and all(c["base_constant"] is None for c in missing)
    assert_rejected(pointers, maps, missing)
    missing = species_candidates(pointers, constants.replace("SPECIES_VOLTORB_HISUI =", "REMOVED_VOLTORB_HISUI ="))
    assert next(c for c in missing if c["source_species"] == "VOLTORB")["unresolved_forms"] == ["HISUIAN_FORM"]
    assert_rejected(pointers, maps, missing)
    unknown = copy.deepcopy(pointers)
    unknown[1]["normal"][0]["source_form"] = "UNKNOWN_FORM"
    assert_rejected(unknown, maps, species_candidates(unknown))
    assert RUNTIME_TABLES.read_text() == render_runtime_tables(pointers, maps)


def build_native(temp):
    # Keep the actual public headers, species enum, generated tables and entire
    # new C translation unit. Only platform types and species availability are
    # supplied by the host shim. ROM tests below also check real gSpeciesInfo.
    (temp / "global.h").write_text('''#ifndef GUARD_GLOBAL_H
#define GUARD_GLOBAL_H
#include <stddef.h>
#include <stdint.h>
#include "constants/moves.h"
#include "constants/species.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t bool8;
typedef uint32_t bool32;
#define TRUE 1
#define FALSE 0
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#endif
''')
    (temp / "pokemon.h").write_text('#include "global.h"\nbool32 IsSpeciesEnabled(enum Species species);\n')
    source = (ROOT / "src/field_move.c").read_text()
    helpers = source[source.index("enum JohtoHeadbuttTreeScore GetJohtoHeadbuttTreeScore("):]
    (temp / "helpers.c").write_text('#include "global.h"\n#include "field_move.h"\n' + helpers + '''
static u16 sDisabledSpecies;
void SetDisabledSpecies(u16 species) { sDisabledSpecies = species; }
bool32 IsSpeciesEnabled(enum Species species) { return species != sDisabledSpecies; }
''')
    library = temp / "headbutt.so"
    subprocess.run(["cc", "-std=gnu17", "-Wall", "-Wextra", "-Werror", "-O2",
                    "-shared", "-fPIC", "-I", str(temp), "-I", str(ROOT / "include"),
                    str(ROOT / "src/polished_johto_headbutt.c"), str(temp / "helpers.c"),
                    "-o", str(library)], check=True, timeout=60)
    native = ctypes.CDLL(str(library))
    native.JohtoHeadbuttSelectEncounterFromSourceMap.argtypes = [
        ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8,
        ctypes.c_uint16, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8,
        ctypes.POINTER(Encounter)]
    native.JohtoHeadbuttSelectEncounterFromSourceMap.restype = ctypes.c_int
    native.GetJohtoHeadbuttTreeScore.argtypes = [ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint16]
    native.GetJohtoHeadbuttTreeScore.restype = ctypes.c_int
    native.SetDisabledSpecies.argtypes = [ctypes.c_uint16]
    return native


def main():
    sources = load_sources()
    pointers = parse_encounters(sources["encounters"])
    maps, rock_maps = parse_maps(sources["maps"])
    validate_audit(pointers, maps)
    source_maps = source_map_assignments(maps)
    by_group = {p["source_constant"]: p for p in pointers}
    constants = dict((name, int(value)) for name, value in re.findall(
        r"^\s*(SPECIES_[A-Z0-9_]+)\s*=\s*(\d+),", (ROOT / "include/constants/species.h").read_text(), re.M))
    bases = [int(n) for n in re.findall(r"^\s*db\s+(\d+)",
             (FIXTURES / "johto_headbutt_badge_levels.asm").read_text(), re.M)]
    assert len(bases) == 17
    with tempfile.TemporaryDirectory(prefix="johto-headbutt-pipeline-") as directory:
        native = build_native(Path(directory))
        select = native.JohtoHeadbuttSelectEncounterFromSourceMap
        # All coordinate byte values, all trainer-ID residues, plus full-width
        # trainer IDs at wrap boundaries. These call the real compiled helper.
        scores = 0
        for y in range(256):
            for x in range(256):
                for trainer in range(10):
                    delta = (((x * (y + 1) + y) & 65535) // 5 - trainer) % 10
                    expected = 2 if delta == 0 else 1 if delta < 5 else 0
                    assert native.GetJohtoHeadbuttTreeScore(x, y, trainer) == expected
                    scores += 1
        for x, y in ((0, 0), (1, 255), (255, 255)):
            for trainer in (255, 256, 65534, 65535):
                assert native.GetJohtoHeadbuttTreeScore(x, y, trainer) == native.GetJohtoHeadbuttTreeScore(x, y, trainer % 10)
        vectors = 0
        seen_species = set()
        for map_row in source_maps:
            group = by_group[map_row["source_group"]]
            for trainer, threshold, kind in ((1, 1, "normal"), (6, 5, "normal"), (0, 8, "rare")):
                # Independent source expectation: expand weights to a 100-slot
                # list rather than replaying the C subtraction loop.
                slots = [row for row in group[kind] for _ in range(row["weight_percent"])]
                assert len(slots) == 100
                for battle_roll in range(10):
                    for slot, row in enumerate(slots):
                        # Regional forms are explicit target species, not the
                        # source numeric form value copied to a target index.
                        expected_constant = "SPECIES_" + row["source_species"]
                        if row["source_form"]:
                            assert row["source_form"] == "HISUIAN_FORM"
                            expected_constant += "_HISUI"
                        for badges, base in enumerate(bases):
                            out = Encounter(65535, 255)
                            result = select(map_row["source_map_group"], map_row["source_map_number"],
                                            0, 0, trainer, badges, battle_roll, slot, ctypes.byref(out))
                            vectors += 1
                            if battle_roll >= threshold:
                                assert result == 2 and (out.species, out.level) == (65535, 255)
                                continue
                            expr = row["source_level_expression"]
                            if row["fixed_level"] is None:
                                offset = int(expr.split("LEVEL_FROM_BADGES", 1)[1].replace(" ", ""))
                                # Pinned AdjustLevelForBadges uses byte arithmetic.
                                raw = (base + offset) & 255
                                level = 2 if raw < 2 or raw >= 128 else 99 if raw >= 100 else raw
                            else:
                                level = row["fixed_level"]
                            assert result == 3 and (out.species, out.level) == (constants[expected_constant], level), (map_row, kind, slot, badges)
                            seen_species.add(out.species)
        # Every source map with no Headbutt set, including rock-only locations,
        # must remain distinguishable from a valid tree's unsuccessful roll.
        all_maps = parse_source_constants((FIXTURES / "johto_map_constants.asm").read_text())
        mapped = {(m["source_map_group"], m["source_map_number"]) for m in source_maps}
        missing = 0
        for ids, name in all_maps.items():
            if ids not in mapped:
                out = Encounter(65535, 255)
                assert select(*ids, 0, 0, 0, 0, 0, 0, ctypes.byref(out)) == 1, name
                assert (out.species, out.level) == (65535, 255)
                missing += 1
        assert missing == 562 and len(rock_maps) == 14
        first = source_maps[0]
        args = [first["source_map_group"], first["source_map_number"], 0, 0, 0, 0, 0, 0]
        for index, value in ((5, 17), (5, 255), (6, 10), (7, 100), (7, 255)):
            bad = args.copy()
            bad[index] = value
            out = Encounter(65535, 255)
            assert select(*bad, ctypes.byref(out)) == 0
            assert (out.species, out.level) == (65535, 255)
        assert select(*args, None) == 0
        # A configured-out species is an error; never substitute its base form
        # or reroll into a more common encounter.
        disabled_cases = 0
        for map_row in source_maps:
            group = by_group[map_row["source_group"]]
            for trainer, kind in ((6, "normal"), (0, "rare")):
                slot = 0
                for row in group[kind]:
                    name = "SPECIES_" + row["source_species"] + ("_HISUI" if row["source_form"] else "")
                    native.SetDisabledSpecies(constants[name])
                    out = Encounter(65535, 255)
                    assert select(map_row["source_map_group"], map_row["source_map_number"],
                                  0, 0, trainer, 0, 0, slot, ctypes.byref(out)) == 4
                    assert (out.species, out.level) == (65535, 255)
                    slot += row["weight_percent"]
                    disabled_cases += 1
        assert constants["SPECIES_VOLTORB_HISUI"] in seen_species
        assert constants["SPECIES_ELECTRODE_HISUI"] in seen_species
        assert vectors == 2193000 and scores == 655360
    print(f"PASS: {vectors} source-map/score/roll/badge cases, {scores} coordinate cases, "
          f"{missing} absent maps, {disabled_cases} disabled-species cases; dormant only")


if __name__ == "__main__":
    main()
