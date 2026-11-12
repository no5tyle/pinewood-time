#!/usr/bin/python3
"""Runs KiCad's design rule check on the board and prints a summary: /usr/bin/python3 hardware/drc.py [report]"""
import os
import re
import sys
from collections import Counter

os.environ.setdefault("KICAD7_FOOTPRINT_DIR", "/usr/share/kicad/footprints")
import pcbnew  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
board = pcbnew.LoadBoard(os.path.join(HERE, "pinewood-time.kicad_pcb"))
report = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "build", "drc.rpt")
os.makedirs(os.path.dirname(report), exist_ok=True)
if not pcbnew.WriteDRCReport(board, report, pcbnew.EDA_UNITS_MILLIMETRES, True):
    raise SystemExit("DRC failed to run")
text = open(report).read()
kinds = Counter(re.findall(r"^\[(\w+)\]", text, re.M))
summary = re.findall(r"\*\* Found (\d+) (DRC violations|unconnected pads|Footprint errors) \*\*", text)
print("summary:", summary)
for k, n in kinds.most_common():
    print(f"  {n:4} {k}")
if any(n != "0" for n, _ in summary):
    sys.exit(1)
