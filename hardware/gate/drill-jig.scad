// pinewood-time drilling jig: guides the 10 mm sensor holes at the finish line, so they line up with the
// sensor bar's turrets (which sit 9.6 mm wide in them, so there's only 0.2 mm to spare each side).
//
// The jig sits on the rail tops and registers on two faces of the board, the same faces the gate relies on:
// - a fence down the board's side edge sets how far across the track each hole is (the gate's legs
//   register on the edges too), and keeps the line of holes square to the track;
// - a lip at the end of the fence hooks over the board's tail end, setting the finish line (200 mm in).
// Two of them: "jig_left" drills lanes 1 and 2 from the left edge, "jig_right" drills lane 3 from the
// right edge (lanes are 50 mm in from each edge). Each hole has a printed bush that drops into it to guide
// a 3 mm pilot drill first.
//
// Render one part with:  openscad -D 'part="jig_left"' -o jig_left.stl drill-jig.scad   (see build.sh)
// part = "assembly" (preview on the track), "jig_left", "jig_right", "bush"
//
// Axes as finish-gate.scad: X across the track (0 = the board's left edge), Y along it (0 = the finish
// line, positive towards the tail end), Z up (0 = the board's top). All sizes in mm.

part = "assembly";
$fn = 64;

// ------------------------------------------------------------------ The track (as finish-gate.scad)
track_w = 296;
board_t = 9;
rail_t = 6;
rail_w = 40;
lanes = [50, 148, 246];
finish_from_end = 200;            // the finish line, from the tail end of the board

// ------------------------------------------------------------------ The jig
guide_h = 16;                     // how tall each drill guide is: the length that keeps the drill upright
bore = 10.3;                      // for a 10 mm bit (printed holes come out a little small)
pilot = 3.3;                      // the bush's bore, for a 3 mm pilot drill
bush_od = 10.1;                   // slides into the 10.3 bore
block = [30, 26];                 // each guide block's footprint on the rail (rails are 40 wide)
bar_y = 26;                       // the bar across the track, along the track
top_t = 5;                        // the bar's and arm's top plate
fence_t = 4;                      // the fence's thickness, outside the board edge
fence_gap = 0.2;                  // clearance between the fence and the board's edge
fence_down = 7;                   // how far the fence and lip reach down the board's edge (board is 9)
lip_t = 4;                        // the lip over the tail end
arm_w = 12;                       // the arm's top plate, from the fence inwards (outer wheels run from 15 mm in)
top_z = rail_t + guide_h;         // the top face (flat: it's the face the jig prints on)
holes = [50, 148];                // from the fence (the left edge): lanes 1 and 2; the right jig uses only 50

// ------------------------------------------------------------------ Parts
module guide_block(x) {
    translate([x - block[0] / 2, -block[1] / 2, rail_t]) cube([block[0], block[1], guide_h]);
}
module guide_hole(x) {
    translate([x, 0, -1]) cylinder(d = bore, h = top_z + 2);
    // a chamfer at the top, to start the drill
    translate([x, 0, top_z - 1.2]) cylinder(d1 = bore, d2 = bore + 2.4, h = 1.21);
    // cross-hairs on the top: line them up with a pencil mark, as a check
    for (r = [0, 90]) translate([x, 0, top_z - 0.6]) rotate([0, 0, r]) cube([block[0] + 1, 0.8, 1.21], center = true);
}

// One jig, drawn for the left edge; the right one is its mirror image with one hole
module jig(hs) {
    x_end = max(hs) + block[0] / 2;
    difference() {
        union() {
            for (x = hs) guide_block(x);
            // the bar across, joining the blocks to the fence; it only touches the track at the blocks
            translate([-fence_gap - fence_t, -bar_y / 2, top_z - top_t]) cube([x_end + fence_gap + fence_t, bar_y, top_t]);
            // a rib under the bar for stiffness, stopping above the rails (it doesn't touch them)
            translate([-fence_gap - fence_t, -2, rail_t + 1]) cube([x_end + fence_gap + fence_t, 4, top_z - rail_t - 1]);
            // the fence, down the board's edge from the bar to the lip at the tail end
            translate([-fence_gap - fence_t, -bar_y / 2, -fence_down]) cube([fence_t, bar_y / 2 + finish_from_end + lip_t, top_z + fence_down]);
            // the arm's top plate along the fence, clear of the board's top (and any screw heads)
            translate([-fence_gap - fence_t, 0, top_z - top_t]) cube([fence_t + fence_gap + arm_w, finish_from_end + lip_t, top_t]);
            // the lip over the tail end
            translate([-fence_gap - fence_t, finish_from_end, -fence_down]) cube([fence_t + fence_gap + arm_w, lip_t, top_z + fence_down]);
        }
        for (x = hs) guide_hole(x);
    }
}

module label(s, x, y, size = 6, angle = 0) {   // engraved in the top face
    translate([x, y, top_z - 0.6]) rotate([0, 0, angle]) linear_extrude(1)
        text(s, size = size, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
}

module jig_left() {
    difference() {
        jig(holes);
        label("1", holes[0] + 10.5, 8, 6);
        label("2", holes[1] + 10.5, 8, 6);
        label("LEFT EDGE", 3.5, 100, 5, 90);
        label("TAIL END", 3.5, 170, 5, 90);
    }
}
module jig_right() {   // the mirror image, laid out so it reads from the right edge
    difference() {
        mirror([1, 0, 0]) jig([holes[0]]);
        label("3", -holes[0] - 10.5, 8, 6);
        label("RIGHT EDGE", -3.5, 100, 5, -90);
        label("TAIL END", -3.5, 170, 5, -90);
    }
}

// The pilot bush: drops into a guide, flange on top, to guide a 3 mm drill through the middle
module bush() {
    difference() {
        union() {
            cylinder(d = bush_od, h = guide_h);
            translate([0, 0, guide_h]) cylinder(d = 16, h = 3);
        }
        translate([0, 0, -1]) cylinder(d = pilot, h = guide_h + 5);
        translate([0, 0, guide_h + 3 - 1]) cylinder(d1 = pilot, d2 = pilot + 2, h = 1.01);
    }
}

// ------------------------------------------------------------------ The track, for the preview
module track_end() {
    color("#e7c99a") translate([0, -60, -board_t]) cube([track_w, finish_from_end + 60, board_t]);
    color("#d6ae74") for (x = lanes) translate([x - rail_w / 2, -60, 0]) cube([rail_w, finish_from_end + 60, rail_t]);
}

// ------------------------------------------------------------------ Output
if (part == "assembly") {
    track_end();
    color("#2f80ed") jig_left();
    color("#f39c12") translate([track_w, 0, 0]) jig_right();
    color("#555") translate([holes[0], 0, rail_t]) bush();
}
// Printed upside down, on the flat top face: no supports
if (part == "jig_left") rotate([180, 0, 0]) translate([0, 0, -top_z]) jig_left();
if (part == "jig_right") rotate([180, 0, 0]) translate([0, 0, -top_z]) jig_right();
if (part == "bush") bush();
