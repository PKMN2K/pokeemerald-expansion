#!/usr/bin/env python3
"""Drive real PokéNav inputs in the disposable Condition-search fixture."""
import subprocess
import sys
from pathlib import Path


def capture(walker, rom, output):
    output.mkdir(parents=True, exist_ok=True)
    actions = ["run:700", "snap:main_menu"]

    def press(key, frames=160):
        actions.extend((f"press:{key}:1", f"run:{frames}"))

    def snap(name):
        actions.append(f"snap:{name}")

    def enter_search():
        press("A", 200)  # Condition menu
        press("DOWN", 80)
        press("A", 200)  # Search categories
        snap("search_menu")
        press("A", 450)  # Cool results; real party/PC sorting

    enter_search()
    snap("cool_first")
    press("UP", 100); snap("cool_first_after_up")
    press("DOWN", 100); snap("cool_tied")
    press("DOWN", 100); snap("cool_third")
    press("UP", 100); press("UP", 100)
    press("RIGHT"); snap("cool_page_02")
    press("RIGHT"); press("RIGHT"); snap("cool_page_04")
    press("RIGHT"); snap("cool_last")
    press("DOWN", 100); snap("cool_last_after_down")
    for _ in range(8):
        press("UP", 100)
    snap("cool_wrap_up")
    press("A", 360); snap("condition_graph")
    press("B", 300); snap("cool_return_from_graph")
    for _ in range(3):
        press("LEFT")
    snap("cool_back_at_start")
    for name in ("beauty", "cute", "smart", "tough"):
        press("B", 260)
        press("DOWN", 100)
        press("A", 450)
        snap(name)
        press("DOWN", 100); snap(name + "_second")
        press("DOWN", 100); snap(name + "_third")
    press("B", 260)  # Search categories
    press("B", 260)  # Condition menu
    press("B", 260)  # Main menu
    snap("main_return")
    enter_search(); snap("cool_reentered")
    press("B", 260); press("A", 450); snap("short_list")
    press("RIGHT"); snap("short_last")
    press("DOWN", 100); snap("short_last_after_down")
    press("B", 260); press("A", 450); snap("single_list")
    press("DOWN", 100); snap("single_after_down")
    press("UP", 100); snap("single_after_up")
    press("A", 360); snap("single_graph")
    press("B", 300); snap("single_return")
    subprocess.run([walker, rom, str(output), *actions], check=True, timeout=180)
    # Cold boot with R held selects four party and 100 boxed results. This
    # verifies three-digit rank text and page clamping across PC box limits.
    actions = ["press:R:90", "run:700"]
    enter_search()
    for _ in range(18):
        press("RIGHT")
    snap("rank104")
    press("DOWN", 100); snap("rank104_after_down")
    press("UP", 100); snap("rank103")
    subprocess.run([walker, rom, str(output), *actions], check=True, timeout=180)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: capture_hgss_condition_search.py GBA_WALK ROM OUTPUT_DIR")
    capture(sys.argv[1], sys.argv[2], Path(sys.argv[3]))
