#!/usr/bin/env python3
"""Exercise the actual Condition graph and markings menus in headless mGBA."""
import subprocess
import sys
from pathlib import Path


def capture(walker, rom, output):
    output.mkdir(parents=True, exist_ok=True)
    actions = []

    def boot(key=None):
        actions.clear()
        if key:
            actions.append(f"press:{key}:90")
        actions.append("run:700")

    def press(key, frames=180):
        actions.extend((f"press:{key}:1", f"run:{frames}"))

    def snap(name):
        actions.append(f"snap:{name}")

    def run():
        subprocess.run([walker, rom, str(output), *actions], check=True, timeout=180)

    def party():
        press("A", 200)
        press("A", 360)

    def search():
        press("A", 200)
        press("DOWN", 80)
        press("A", 200)
        press("A", 450)

    boot()
    party(); snap("party_first")
    for i in range(32):
        actions.append("run:8"); snap(f"sheen_{i:02d}")
    press("A"); snap("party_a_ignored")
    for name in ("party_zero", "party_genderless", "party_fourth", "party_last", "party_cancel"):
        press("DOWN"); snap(name)
    press("DOWN"); snap("party_wrap_first")
    press("UP"); snap("party_wrap_cancel")
    press("UP"); snap("party_reverse_last")
    press("DOWN"); press("A", 260); snap("party_cancel_return")
    press("A", 360); snap("party_reentered")
    press("B", 260); snap("party_b_return")
    press("B", 260)  # Main menu
    search(); snap("search_first_list")
    press("A", 360); snap("search_first")
    press("UP"); snap("search_first_clamped")
    press("A"); snap("marks_open")
    # Visual slots are circle/triangle/square/heart; engine bits are 0/2/1/3.
    press("A"); snap("marks_circle")
    press("RIGHT"); press("A"); snap("marks_triangle")
    press("DOWN"); press("A"); snap("marks_heart")
    press("LEFT"); press("A"); snap("marks_all")
    press("DOWN"); snap("marks_ok")
    press("A"); snap("marks_saved_party")
    press("A"); snap("marks_party_reopened")
    press("A"); snap("marks_party_pending")
    press("B"); snap("marks_b_discarded")
    press("A"); press("A"); press("UP"); snap("marks_cancel")
    press("A"); snap("marks_cancel_discarded")
    for name in ("search_ditto", "search_bulbasaur", "search_pc"):
        press("DOWN"); snap(name)
    press("A"); press("RIGHT"); press("A"); press("DOWN"); press("DOWN"); press("A")
    snap("marks_saved_pc")  # Triangle only: bit 2.
    press("DOWN"); snap("search_pc_next")
    press("UP"); snap("search_pc_reloaded")
    press("A"); snap("marks_pc_reopened")
    press("B"); press("B", 300); snap("search_return_list")
    press("A", 360); snap("search_pc_reentered")
    press("B", 300); press("B", 260); press("B", 260); press("B", 260)
    party(); snap("party_saved_marks")
    run()

    boot("L"); party(); snap("single_first")
    press("DOWN"); snap("single_cancel")
    press("DOWN"); snap("single_wrap")
    press("B", 260); snap("single_return")
    run()

    boot("SELECT"); party(); snap("empty_cancel")
    press("UP"); snap("empty_after_up")
    press("DOWN"); snap("empty_after_down")
    press("A", 260); snap("empty_return")
    run()

    boot("R"); search()
    for _ in range(18):
        press("RIGHT")
    press("A", 360); snap("rank104")
    press("DOWN"); snap("rank104_clamped")
    press("UP"); snap("rank103")
    press("B", 300); snap("rank103_return")
    run()


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: capture_hgss_condition_graph.py GBA_WALK ROM OUTPUT_DIR")
    capture(sys.argv[1], sys.argv[2], Path(sys.argv[3]))
