#!/bin/sh
# Exports every printable part of the finish gate to stl/, and redraws the pictures in renders/.
# Needs OpenSCAD (2021.01 or later); the 3D picture also needs xvfb-run on a machine with no display.
set -e
cd "$(dirname "$0")"
mkdir -p stl renders
for p in left right cover mx_blank led_lens sensor_bar link_in link_out; do
  openscad -D "part=\"$p\"" -o "stl/$p.stl" finish-gate.scad
done
for p in jig_left jig_right bush; do
  openscad -D "part=\"$p\"" -o "stl/drill_$p.stl" drill-jig.scad
done
RUN=""
command -v xvfb-run >/dev/null && [ -z "$DISPLAY" ] && RUN="xvfb-run -a -s '-screen 0 1600x1200x24'"
eval $RUN openscad -o renders/gate-front.png --imgsize=1600,1100 --colorscheme=Tomorrow \
  --camera=148,-60,80,62,0,330,900 --projection=p finish-gate.scad
python3 section.py
eval $RUN openscad -o renders/gate-underside.png -D "'part=\"underside\"'" --imgsize=1600,1000 --colorscheme=Tomorrow \
  --camera=148,60,-60,125,0,200,760 --projection=p finish-gate.scad
eval $RUN openscad -o renders/board-in-beam.png -D "'part=\"inside\"'" --imgsize=1600,700 --colorscheme=Tomorrow \
  --camera=205,-330,175,205,0,135 --projection=o finish-gate.scad
eval $RUN openscad -o renders/usb-socket.png -D "'part=\"usb\"'" --imgsize=1200,1000 --colorscheme=Tomorrow \
  --camera=450,-160,210,300,0,110 --projection=p finish-gate.scad
eval $RUN openscad -o renders/gate-exploded.png -D "'part=\"exploded\"'" --imgsize=1600,1300 --colorscheme=Tomorrow \
  --camera=150,0,80,62,0,328,1000 --projection=p finish-gate.scad
eval $RUN openscad -o renders/drill-jig.png --imgsize=1600,1000 --colorscheme=Tomorrow \
  --camera=150,70,0,50,0,20,640 --projection=p drill-jig.scad
eval $RUN openscad -o renders/link-sockets.png -D "'part=\"links\"'" --imgsize=1600,1000 --colorscheme=Tomorrow \
  --camera=-12,20,30,70,0,200,230 --projection=p finish-gate.scad
