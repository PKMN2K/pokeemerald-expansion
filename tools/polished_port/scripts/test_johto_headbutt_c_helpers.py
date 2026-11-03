#!/usr/bin/env python3
"""Execute dormant production Headbutt C helpers against pinned source tables.

Build a standalone host executable from exact function bodies in src/field_move.c.
No game code is linked, no ROM maps or encounters are enabled.
"""
import json
import re
import subprocess
import tempfile
from pathlib import Path

from export_johto_headbutt_encounter_tables import ROOT, load_sources, parse_encounters


FUNCTIONS = (
    "JohtoHeadbuttSelectWeightedEntry",
    "JohtoHeadbuttResolveSourceEncounter",
    "JohtoHeadbuttGetBadgeBaseLevel",
    "JohtoHeadbuttResolveLevel",
    "JohtoHeadbuttResolveLevelFromBadgeCount",
)
BADGE_BASES = (4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 43, 46, 49, 52, 55, 58, 61)


def extract_function(source, name):
    match = re.search(r"(?m)^(?:s16|bool8)\\s+" + name + r"\\s*\\(", source)
    assert match is not None, name
    start = source.find("{", match.end())
    assert start != -1, name
    depth = 0
    for i in range(start, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():i + 1]
    raise AssertionError("Unclosed C function: " + name)


def c_string(value):
    return json.dumps(value) if value is not None else "NULL"


def main():
    src = (ROOT / "src/field_move.c").read_text(encoding="utf-8")
    tables = parse_encounters(load_sources()["encounters"])
    pieces = ["""#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
enum JohtoHeadbuttLevelMode {
    JOHTO_HEADBUTT_LEVEL_FIXED,
    JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET
};
struct JohtoHeadbuttSourceEncounter {
    u8 weight;
    const char *sourceSpecies;
    const char *sourceForm;
    const char *sourceLevelExpression;
};
"""]
    pieces += [extract_function(src, name) + "\n" for name in FUNCTIONS]
    pieces.append("int main(void) {\n")
    pieces.append("u8 level = 0;\nstruct JohtoHeadbuttSourceEncounter chosen;\n")
    for badges, base in enumerate(BADGE_BASES):
        pieces.append(
            f"assert(JohtoHeadbuttGetBadgeBaseLevel({badges}, &level) && level == {base});\n"
        )
    pieces.append("assert(!JohtoHeadbuttGetBadgeBaseLevel(17, &level));\n")
    pieces.append("assert(!JohtoHeadbuttGetBadgeBaseLevel(0, NULL));\n")
    pieces.append("assert(!JohtoHeadbuttResolveLevelFromBadgeCount(JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET, 0, 17, &level));\n")
    pieces.append("assert(!JohtoHeadbuttResolveLevelFromBadgeCount(99, 0, 0, &level));\n")
    pieces.append("assert(!JohtoHeadbuttResolveLevelFromBadgeCount(JOHTO_HEADBUTT_LEVEL_FIXED, 10, 0, NULL));\n")
    count = 0
    unique = set()
    for group in tables:
        if group["is_rock_smash_table"] or group["shares_canyon_table"]:
            continue
        for distribution in ("normal", "rare"):
            rows = group[distribution]
            assert rows
            key = (group["effective_table_label"], distribution)
            assert key not in unique
            unique.add(key)
            name = f"rows_{len(unique)}"
            weights = f"weights_{len(unique)}"
            expected = f"expected_{len(unique)}"
            parts = []
            for row in rows:
                parts.append(
                    "{" + ", ".join((str(row["weight_percent"]),
                                     c_string(row["source_species"]),
                                     c_string(row["source_form"]),
                                     c_string(row["source_level_expression"]))) + "}"
                )
            indexes = [idx for idx, row in enumerate(rows)
                       for _ in range(row["weight_percent"])]
            assert len(indexes) == 100
            pieces.append(f"static const struct JohtoHeadbuttSourceEncounter {name}[] = {{{','.join(parts)}}};\n")
            pieces.append(f"static const u8 {weights}[] = {{{','.join(str(row['weight_percent']) for row in rows)}}};\n")
            pieces.append(f"static const u8 {expected}[100] = {{{','.join(map(str, indexes))}}};\n")
            pieces.append("for (int roll = 0; roll < 100; ++roll) {\n")
            pieces.append(f"int idx = {expected}[roll];\n")
            pieces.append(f"assert(JohtoHeadbuttSelectWeightedEntry({weights}, ARRAY_COUNT({weights}), roll) == idx);\n")
            pieces.append(f"assert(JohtoHeadbuttResolveSourceEncounter({name}, ARRAY_COUNT({name}), roll, &chosen));\n")
            pieces.append(f"assert(chosen.weight == {name}[idx].weight);\n")
            pieces.append(f"assert(strcmp(chosen.sourceSpecies, {name}[idx].sourceSpecies) == 0);\n")
            pieces.append(f"assert(strcmp(chosen.sourceLevelExpression, {name}[idx].sourceLevelExpression) == 0);\n")
            pieces.append(f"assert((chosen.sourceForm == NULL && {name}[idx].sourceForm == NULL) || "
                          f"(chosen.sourceForm != NULL && {name}[idx].sourceForm != NULL "
                          f"&& strcmp(chosen.sourceForm, {name}[idx].sourceForm) == 0));\n")
            for idx, row in enumerate(rows):
                expr = row["source_level_expression"]
                if row["fixed_level"] is None:
                    match = re.fullmatch(r"LEVEL_FROM_BADGES\s*([+-])\s*(\d+)", expr)
                    assert match, expr
                    offset = int(match.group(2)) * (1 if match.group(1) == "+" else -1)
                    mode = "JOHTO_HEADBUTT_LEVEL_BADGE_OFFSET"
                else:
                    offset = row["fixed_level"]
                    mode = "JOHTO_HEADBUTT_LEVEL_FIXED"
                pieces.append(f"if (idx == {idx}) {{\n")
                for badges, base in enumerate(BADGE_BASES):
                    target = max(2, min(99, base + offset)) if row["fixed_level"] is None else offset
                    pieces.append(f"assert(JohtoHeadbuttResolveLevelFromBadgeCount({mode}, {offset}, {badges}, &level) && level == {target});\n")
                pieces.append("}\n")
            pieces.append("}\n")
            count += 100 * len(BADGE_BASES)
    assert len(unique) == 16 and count == 27200
    pieces.append("return 0;\n}\n")
    with tempfile.TemporaryDirectory(prefix="headbutt-c-") as temp:
        source_path = Path(temp) / "headbutt_test.c"
        executable = Path(temp) / "headbutt_test"
        source_path.write_text("\n".join(pieces), encoding="utf-8")
        subprocess.run(["cc", "-std=c99", "-O0", "-Wall", "-Wextra", "-Werror",
                        "-o", str(executable), str(source_path)], check=True)
        subprocess.run([str(executable)], check=True)
    print(f"PASS: executed production C Headbutt helpers on {count} source/badge vectors "
          f"across {len(unique)} distributions; no live encounters")


if __name__ == "__main__":
    main()
