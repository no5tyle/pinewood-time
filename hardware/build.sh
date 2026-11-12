#!/bin/sh
# Regenerates the whole board from generate_board.py: place, route, pour, check, then the files to order it.
# Needs KiCad 7 (its pcbnew Python module and kicad-cli) and, for the renders, Node with Playwright.
set -e
cd "$(dirname "$0")"
export KICAD7_FOOTPRINT_DIR="${KICAD7_FOOTPRINT_DIR:-/usr/share/kicad/footprints}"
PY=/usr/bin/python3
$PY generate_board.py place
$PY route.py
$PY generate_board.py finish
$PY drc.py                # stops here unless there are no errors and nothing is left unconnected
$PY tools/fab.py          # fab/pinewood-time-gerbers.zip, fab/bom.csv, fab/bom.md
python3 tools/schematic.py
if [ "${SKIP_RENDERS:-}" = "" ]; then
  $PY tools/render.py     # renders/pcb-front.png, renders/pcb-back.png
fi
