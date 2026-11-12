#!/bin/sh
# Exports every printable part of the finish gate to stl/, and redraws the pictures in renders/.
# Needs OpenSCAD (2021.01 or later); the 3D picture also needs xvfb-run on a machine with no display.
set -e
cd "$(dirname "$0")"
mkdir -p stl renders
for p in left right pod lid mx_blank led_lens sensor_bar; do
  openscad -D "part=\"$p\"" -o "stl/$p.stl" finish-gate.scad
done
RUN=""
command -v xvfb-run >/dev/null && [ -z "$DISPLAY" ] && RUN="xvfb-run -a -s '-screen 0 1600x1200x24'"
eval $RUN openscad -o renders/gate-front.png --imgsize=1600,1100 --colorscheme=Tomorrow \
  --camera=148,-60,80,62,0,330,900 --projection=p finish-gate.scad
python3 section.py
eval $RUN openscad -o renders/gate-underside.png -D "'part=\"underside\"'" --imgsize=1600,1000 --colorscheme=Tomorrow \
  --camera=148,60,-60,125,0,200,760 --projection=p finish-gate.scad
