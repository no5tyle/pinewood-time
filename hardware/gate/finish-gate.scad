// pinewood-time finish gate: a 3D-printable finish-line gantry that clamps onto the track, holds a break-beam
// sensor over each lane, and houses the timer board in a pod on top.
//
// Sized for the 3-lane track in "Pinewood Derby Track Build" (rev 0.5): 296 mm wide, 9 mm plywood board,
// 40 x 6 mm rails on the pattern 30 | 40 | 58 | 40 | 58 | 40 | 30 mm (lane centres 50, 148, 246 mm).
//
// Render one part with:  openscad -D 'part="left"' -o left.stl finish-gate.scad   (see build.sh)
// part = "assembly" (preview only), "left", "right", "pod", "lid", "mx_blank", "led_lens", "sensor_bar"
//
// Axes: X across the track (0 = the board's left edge, looking down the track towards the finish),
// Y along it (0 = the finish line, negative = up-track, towards the start), Z up (0 = the race surface,
// the top of the 9 mm board). All sizes in mm.

part = "assembly";
$fn = 48;

// ------------------------------------------------------------------ The track (from the build guide)
track_w = 296;
board_t = 9;
rail_t = 6;
rail_w = 40;
lanes = [50, 148, 246];           // rail (and lane) centre lines
under_clear = 19;                 // the pine supports hold the board 19 mm off the floor

// ------------------------------------------------------------------ The sensors
// Adafruit IR break-beam (3 mm LEDs, ADA2167; Omron housings): each half is 20 x 10 x 8 mm. The LED looks
// out of the broad 20 x 10 face, 5 mm from one end; 10 mm from it is a 2.4 mm mounting hole through the
// housing, and the cable leaves the end beside the hole. The sensors lie flat, held by an M2 screw
// through that hole, LED towards the beam.
sen = [20, 10, 8];                // long (x), wide (y), thick (z)
sen_led = 5;                      // LED centre from the far (non-cable) end
sen_hole = 15;                    // mounting hole centre from the same end
sen_fit = 0.25;                   // clearance each side
// Each sensor lies along x with its LED on the lane centre and its cable end towards -x (the left)
function sen_x0(lane_x) = lane_x - (sen[0] - sen_led);   // its cable end

// ------------------------------------------------------------------ The gate
clear_h = 120;                    // underside of the beam above the race surface (car clearance)
beam_d = 40;                      // front to back
beam_h = 33;
wall = 3;
bot_wall = 4;
leg_w = 24;                       // the legs stand outside the board, clear of every lane
edge_gap = 0.5;                   // clearance between each leg and the board edge
split_x = 99;                     // the beam splits between lanes 1 and 2, under the pod
jaw_top = 8;                      // how far the top clamp jaw reaches onto the board (wheels run from 15)
slot_h = board_t + 0.6;           // the clamp slot, a little over the board for plywood variation
chq = 6;                          // chequer square
relief = 0.8;                     // raised pattern height (change filament here for two colours)

beam_z0 = clear_h;
beam_z1 = clear_h + beam_h;
x0 = -leg_w - edge_gap;
x1 = track_w + leg_w + edge_gap;
// ------------------------------------------------------------------ The sensor bar (under the track)
// One solid bar under the board at the finish line, leg to leg. It holds the three emitters flat
// underneath, LED up, each looking up through a turret that rises into a 10 mm hole through the rail and
// board; a groove underneath takes their wiring to one lead. Each end is a dovetail
// tenon that slides up into a socket in the leg, locked by an M3 pin, and an M4 thumbscrew in each end
// presses the board up against the legs' top jaws.
bar_top = -slot_h;                // 0.6 mm under the board, taken up by the thumbscrews
bar_bot = -22;                    // 13 mm under the board, 6 mm above the floor
bar_w = 30;                       // along the track
tenon = [5, 10, 14];              // depth into the leg, width at the root, width at the tip
tenon_fit = 0.25;
pin_xz = [-edge_gap - 2.5, -16];  // the lock pin's place in the left tenon (mirrored on the right)
thumb = [[20, -9.5], [track_w - 20, -9.5]];
turret_top = rail_t - 1;          // the top of each turret (the light's way out), 1 mm below the rail top
turret_d = 9.6;                   // in a 10 mm hole
bore_d = 6;                       // the light's path up through the turret
groove_y = [6, 12];               // the wiring groove under the bar, beside the sensors
foot_z = bar_bot;                 // the legs reach down beside the bar

// ------------------------------------------------------------------ The pod (the timer board)
pcb = [112, 82, 1.6];
pod_in = [140, 86];               // inside: the board plus a 14 mm cable gutter each side
pod_wall = 3;
pod_floor = 3;
pod_h = 30;                       // floor to lid, inside: 6 mm standoffs, the board, 22 mm for its parts
pod_x = track_w / 2;
pod_out = [pod_in[0] + 2 * pod_wall, pod_in[1] + 2 * pod_wall];
pod_z0 = beam_z1;
pcb_z = pod_z0 + pod_floor + 6;   // underside of the board
lid_z = pod_z0 + pod_floor + pod_h;
lid_t = 3;
// The board's corner holes and parts, in its own coordinates (x right, y down, origin top-left)
pcb_holes = [[21, 78.5], [60, 78.5], [107.5, 78.5], [107.5, 30.5]];
pcb_usb = [62.5, 0];              // the Pico's USB socket, on the board's top edge
pcb_led = [84, 64];               // D1, the status light
pcb_key = [62, 64];               // an empty patch of board for the MX switch to sit over
function pod_xy(p) = [pod_x - pcb[0] / 2 + p[0], pcb[1] / 2 - p[1]];   // board -> gate coordinates
pod_bolts = [[pod_x - 63, 15], [pod_x - 63, -15], [pod_x + 63, 15], [pod_x + 63, -15]];

// ================================================================== Helpers
module rbox(size, r) {            // a box with rounded vertical edges, from the origin
    hull() for (x = [r, size[0] - r], y = [r, size[1] - r]) translate([x, y, 0]) cylinder(r = r, h = size[2]);
}

module chequer(w, h, n_off = 0) { // raised squares on a face in the XZ plane, facing -Y
    for (i = [0 : floor(w / chq) - 1], j = [0 : floor(h / chq) - 1])
        // inset a hair, so squares that meet at a corner don't share an edge (slicers need that)
        if ((i + j + n_off) % 2 == 0) translate([i * chq + 0.1, -relief, j * chq + 0.1]) cube([chq - 0.2, relief + 0.01, chq - 0.2]);
}

module front_text(s, size, x, z) { // raised lettering on the front face (Y = -beam_d/2)
    translate([x, -beam_d / 2 + 0.01, z]) rotate([90, 0, 0])
        linear_extrude(relief) text(s, size = size, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
}

// A pocket for one sensor standing upright, LED end at z = 0 looking down (-Z), body going up
// A pocket for one sensor lying flat, open downwards from z = 0 to depth d; lane_x is its LED's place
module flat_pocket(lane_x, d) {
    translate([sen_x0(lane_x) - sen_fit, -sen[1] / 2 - sen_fit, -0.01]) cube([sen[0] + 2 * sen_fit, sen[1] + 2 * sen_fit, d + 0.01]);
}
// The M2 screw's pilot, at the sensor's mounting hole
function sen_hole_x(lane_x) = lane_x - (sen_hole - sen_led);   // 10 mm from the LED, towards the cable

// ================================================================== The gantry
module leg(side) {                // side -1 = left (x < 0), +1 = right
    lx = side < 0 ? x0 : track_w + edge_gap;
    difference() {
        union() {
            // the post
            translate([lx, -beam_d / 2, foot_z]) cube([leg_w, beam_d, beam_z1 - foot_z]);
            // a pylon cap above the beam
            translate([lx + leg_w / 2, 0, beam_z1]) hull() {
                translate([-leg_w / 2, -beam_d / 2, 0]) cube([leg_w, beam_d, 0.01]);
                translate([-leg_w / 2 + 4, -beam_d / 2 + 4, 10]) cube([leg_w - 8, beam_d - 8, 0.01]);
            }
            // clamp jaws reaching in over and under the board edge
            jx = side < 0 ? -edge_gap : track_w - jaw_top;
            translate([jx, -beam_d / 2, 0]) cube([jaw_top + edge_gap, beam_d, 4]);
            // chequered front
            translate([lx, -beam_d / 2, 0]) chequer(leg_w, clear_h, side < 0 ? 0 : 1);
        }
        // the socket the sensor bar's end slides up into: open at the bottom, closed at the top, so the
        // bar stops just under the board
        mirror_side(side) translate([0, 0, bar_bot - 1]) linear_extrude(bar_top - bar_bot + 1) tenon_shape(tenon_fit);
        // the lock pin (M3 x 40), front to back through the leg and the tenon, nut at the back
        mirror_side(side) translate([pin_xz[0], 0, pin_xz[1]]) rotate([90, 0, 0]) {
            cylinder(d = 3.4, h = beam_d + 2, center = true);
            translate([0, 0, -beam_d / 2 - 0.01]) cylinder(d = 6.4, h = 2.6, $fn = 6);
        }
    }
}

module beam() {
    difference() {
        union() {
            translate([x0, -beam_d / 2, beam_z0]) cube([x1 - x0, beam_d, beam_h]);
            // two rows of chequers along the top of the front, and the lane numbers below them
            translate([x0, -beam_d / 2, beam_z1 - 2 * chq - 1]) chequer(x1 - x0, 2 * chq);
            for (i = [0 : len(lanes) - 1]) front_text(str(i + 1), 12, lanes[i], beam_z0 + 10);
        }
        // the cable run inside, from leg to leg
        translate([x0 + 3, -beam_d / 2 + wall, beam_z0 + bot_wall]) cube([x1 - x0 - 6, beam_d - 2 * wall, beam_h - bot_wall - wall]);
        // over each lane: the receiver's pod (below), its screw, and a hole for its cable into the beam
        for (x = lanes) {
            translate([0, 0, beam_z0 - pod_s]) flat_pocket(x, pod_s - 0.2);
            translate([sen_hole_x(x), 0, beam_z0 - 1]) cylinder(d = 1.8, h = bot_wall + 0.5);
            translate([sen_x0(x) - 4, -3, beam_z0 - 1]) cube([4.5, 6, bot_wall + 2]);
        }
        // bolts through the beam that hold the pod (and the two halves) together, and the cable holes
        for (b = pod_bolts) translate([b[0], b[1], beam_z0 - 1]) cylinder(d = 3.4, h = beam_h + 2);
        for (s = [-1, 1]) translate([pod_x + s * 63 - 5, -9, beam_z1 - wall - 1]) cube([10, 18, wall + 2]);
    }
}

// A small pod under the beam over each lane, holding a receiver flat, LED looking down at the track
pod_s = sen[2] + 0.5;             // how far the pods hang below the beam
module sensor_pods() {
    for (x = lanes) difference() {
        translate([sen_x0(x) - 2.5, -sen[1] / 2 - 2.5, beam_z0 - pod_s]) rbox([sen[0] + 5, sen[1] + 5, pod_s + 0.01], 2);
        translate([0, 0, beam_z0 - pod_s]) flat_pocket(x, pod_s - 0.2);
        translate([sen_hole_x(x), 0, beam_z0 - pod_s - 1]) cylinder(d = 1.8, h = pod_s + bot_wall);
        // the cable leaves the sensor's end: out through the pod's end wall and up into the beam
        translate([sen_x0(x) - 4, -3, beam_z0 - pod_s - 1]) cube([4.5, 6, pod_s + 2]);
    }
}

// the dovetail, in the left leg's coordinates: from the bar's end (x = 0) across the edge gap and
// tenon[0] into the leg, widening as it goes; grown by c for the socket
module tenon_shape(c) {
    polygon([[0.01, -tenon[1] / 2 - c], [-edge_gap, -tenon[1] / 2 - c], [-edge_gap - tenon[0] - c, -tenon[2] / 2 - c],
             [-edge_gap - tenon[0] - c, tenon[2] / 2 + c], [-edge_gap, tenon[1] / 2 + c], [0.01, tenon[1] / 2 + c]]);
}
module mirror_side(side) {
    if (side < 0) children();
    else translate([track_w, 0, 0]) mirror([1, 0, 0]) children();
}

// cable channel up each leg into the beam, with a way in at the back just under the board (for the
// receivers' cables, and the sensor bar's lead)
module leg_channels() {
    for (cx = [x0 + 8.5, x1 - 8.5]) {
        translate([cx - 5.5, -6, -8]) cube([11, 12, beam_z0 + bot_wall + 14]);
        translate([cx - 5.5, 0, -8]) cube([11, beam_d, 9]);
    }
}
module gantry() { difference() { union() { leg(-1); leg(1); beam(); sensor_pods(); } leg_channels(); } }

// The two printed halves. The right half carries a hollow tongue that slides 8 mm into the left half's
// beam; a collar filling the end of the right half's beam holds it, and cables pass through both
cav = [beam_d - 2 * wall, beam_h - bot_wall - wall];   // the beam's inside, front-back and up-down
module tongue() {
    t = 0.3;
    z = beam_z0 + bot_wall;
    difference() {
        union() {
            translate([split_x, -cav[0] / 2 - 0.01, z - 0.01]) cube([10, cav[0] + 0.02, cav[1] + 0.02]);
            translate([split_x - 8, -cav[0] / 2 + t, z + t]) cube([8.01, cav[0] - 2 * t, cav[1] - 2 * t]);
        }
        translate([split_x - 9, -cav[0] / 2 + t + 2, z + t + 2]) cube([20, cav[0] - 2 * t - 4, cav[1] - 2 * t - 4]);
    }
}
module left_half() { intersection() { gantry(); translate([x0 - 5, -100, -100]) cube([split_x - x0 + 5, 200, 400]); } }
module right_half() {
    union() {
        intersection() { gantry(); translate([split_x, -100, -100]) cube([x1 - split_x + 5, 200, 400]); }
        tongue();
    }
}

// ================================================================== The pod
module pod() {
    ox = pod_x - pod_out[0] / 2;
    oy = -pod_out[1] / 2;
    difference() {
        union() {
            translate([ox, oy, pod_z0]) rbox([pod_out[0], pod_out[1], pod_floor + pod_h], 6);
            // FINISH on the front, with a chequered band either side
            translate([pod_x, oy + 0.01, pod_z0 + 18]) rotate([90, 0, 0])
                linear_extrude(relief) text("FINISH", size = 13, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
            for (s = [-1, 1]) translate([pod_x + s * 52 - 12, oy, pod_z0 + 6]) chequer(24, 24, s < 0 ? 0 : 1);
        }
        // inside
        translate([ox + pod_wall, oy + pod_wall, pod_z0 + pod_floor]) rbox([pod_in[0], pod_in[1], pod_h + 1], 3);
        // the Pico's USB socket, through the back wall (tall enough for a Pico on headers or soldered flat)
        u = pod_xy(pcb_usb);
        translate([u[0] - 7, pod_out[1] / 2 - pod_wall - 1, pcb_z + pcb[2] - 0.5]) cube([14, pod_wall + 2, 18]);
        // bolts down through the beam, and the cable holes into it, in the gutters
        for (b = pod_bolts) translate([b[0], b[1], pod_z0 - 1]) cylinder(d = 3.4, h = pod_floor + 2);
        for (s = [-1, 1]) translate([pod_x + s * 63 - 5, -9, pod_z0 - 1]) cube([10, 18, pod_floor + 2]);
        // a cable gland at each end for the gate and reset leads (and USB, if you'd rather it came out the side)
        for (s = [-1, 1]) translate([pod_x + s * (pod_out[0] / 2), 25, pod_z0 + pod_floor + 8]) rotate([0, 90, 0]) cylinder(d = 7, h = 10, center = true);
    }
    // board standoffs (M3 self-tapping screws, 6 mm)
    for (h = pcb_holes) let(p = pod_xy(h)) translate([p[0], p[1], pod_z0 + pod_floor - 0.01]) difference() {
        cylinder(d = 7, h = 6);
        cylinder(d = 2.6, h = 7);
    }
    // lid screw posts in the corners
    for (sx = [-1, 1], sy = [-1, 1]) translate([pod_x + sx * 64, sy * 37, pod_z0 + pod_floor - 0.01]) difference() {
        cylinder(d = 7, h = pod_h);
        cylinder(d = 2.6, h = pod_h + 1);
    }
}

// MX switch: a 14 mm square hole in a 1.5 mm plate (the switch clips into it)
mx_hole = 14;
mx_plate = 1.5;
module lid() {
    ox = pod_x - pod_out[0] / 2;
    oy = -pod_out[1] / 2;
    k = pod_xy(pcb_key);
    l = pod_xy(pcb_led);
    difference() {
        union() {
            translate([ox, oy, lid_z]) rbox([pod_out[0], pod_out[1], lid_t], 6);
            // a lip that locates the lid inside the walls
            translate([ox + pod_wall + 0.3, oy + pod_wall + 0.3, lid_z - 2]) difference() {
                rbox([pod_in[0] - 0.6, pod_in[1] - 0.6, 2.01], 3);
                translate([1.6, 1.6, -1]) rbox([pod_in[0] - 3.8, pod_in[1] - 3.8, 4], 2);
            }
            // a light pipe down towards the status LED
            translate([l[0], l[1], lid_z - 12]) cylinder(d = 8, h = 12.01);
        }
        // corner screws, countersunk
        for (sx = [-1, 1], sy = [-1, 1]) translate([pod_x + sx * 64, sy * 37, lid_z - 3]) {
            cylinder(d = 3.4, h = 10);
            translate([0, 0, 3 + lid_t - 1.8]) cylinder(d1 = 3.4, d2 = 6.6, h = 1.81);
        }
        // the MX switch: plate 1.5 mm thick here, and room underneath for its housing
        translate([k[0] - mx_hole / 2, k[1] - mx_hole / 2, lid_z - 1]) cube([mx_hole, mx_hole, lid_t + 2]);
        translate([k[0] - 8, k[1] - 8, lid_z - 2.01]) cube([16, 16, 2 + lid_t - mx_plate]);
        // status LED window
        translate([l[0], l[1], lid_z - 13]) cylinder(d = 5.3, h = 20);
        // name, engraved
        translate([pod_x, 26, lid_z + lid_t - 0.6]) linear_extrude(1)
            text("pinewood-time", size = 7, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
        // chequered strip along the front edge, engraved
        for (i = [0 : 19], j = [0 : 1]) if ((i + j) % 2 == 0)
            translate([pod_x - 60 + i * 6 + 0.1, oy + 6 + j * 6 + 0.1, lid_z + lid_t - 0.6]) cube([5.8, 5.8, 1]);
    }
}

// Fills the MX hole when there's no key: sits flush, held by two small snap bumps under the plate
module mx_blank() {
    union() {
        translate([-(mx_hole + 3) / 2, -(mx_hole + 3) / 2, 0]) cube([mx_hole + 3, mx_hole + 3, 1]);
        translate([-(mx_hole - 0.3) / 2, -(mx_hole - 0.3) / 2, -mx_plate - 1.2]) cube([mx_hole - 0.3, mx_hole - 0.3, mx_plate + 1.21]);
        for (s = [-1, 1]) translate([s * (mx_hole - 0.3) / 2, 0, -mx_plate - 0.6]) rotate([90, 0, 0]) cylinder(d = 1.0, h = 6, center = true);
    }
}

// Status light lens: print in white or clear; pushes into the lid's LED window
module led_lens() {
    union() {
        cylinder(d = 5.1, h = 6);
        translate([0, 0, 6]) cylinder(d = 7.5, h = 1);
        translate([0, 0, 7]) scale([1, 1, 0.5]) sphere(d = 7.5);
    }
}

// ================================================================== The sensor bar
// The track needs one 10 mm hole at the finish line on each rail centre line, through rail and board. The
// bar's turrets push up into them from underneath.
pocket_d = sen[2] + 0.3;          // the emitters' pockets, up from the bar's underside
module sensor_bar() {
    difference() {
        union() {
            translate([0, -bar_w / 2, bar_bot]) rbox([track_w, bar_w, bar_top - bar_bot], 3);
            for (x = lanes) translate([x, 0, bar_top - 0.01]) cylinder(d = turret_d, h = turret_top - bar_top + 0.01);
            for (side = [-1, 1]) mirror_side(side) translate([0, 0, bar_bot]) linear_extrude(bar_top - bar_bot) tenon_shape(0);
        }
        for (x = lanes) {
            // the emitter, in from underneath, LED face up against the top of its pocket
            translate([0, 0, bar_bot - 1]) flat_pocket(x, pocket_d + 1);
            // its light's way up through the bar and turret
            translate([x, 0, bar_bot + pocket_d - 0.01]) cylinder(d = bore_d, h = turret_top - bar_bot);
            // the M2 screw through its mounting hole into the bar (M2 x 10, self-tapping)
            translate([sen_hole_x(x), 0, bar_bot]) cylinder(d = 1.8, h = pocket_d + 3);
            // its cable, from the pocket's left end into the wiring groove
            translate([sen_x0(x) - 4, -3, bar_bot - 1]) cube([4.01, groove_y[1] + 3, 6]);
        }
        // the wiring groove underneath, from lane 3 to the left end, where the lead leaves
        translate([4, groove_y[0], bar_bot - 1]) cube([sen_x0(lanes[2]) - 4, groove_y[1] - groove_y[0], 6]);
        // thumbscrews (M4 x 16) up through each end, nuts trapped underneath
        for (t = thumb) translate([t[0], t[1], bar_bot - 1]) {
            cylinder(d = 4.4, h = bar_top - bar_bot + 2);
            cylinder(d = 8.1, h = 4.5, $fn = 6);
        }
        // the lock pins through the tenons
        for (side = [-1, 1]) mirror_side(side) translate([pin_xz[0], 0, pin_xz[1]]) rotate([90, 0, 0]) cylinder(d = 3.4, h = bar_w + 2, center = true);
    }
}

// ================================================================== Track and cars, for the preview
module track_preview() {
    color("BurlyWood") translate([0, -350, -board_t]) cube([track_w, 600, board_t]);
    color("Tan") for (x = lanes) difference() {
        translate([x - rail_w / 2, -350, 0]) cube([rail_w, 600, rail_t]);
        translate([x, 0, -1]) cylinder(d = 10, h = 10);
    }
    // a car about to cross the line in lane 2
    color("SteelBlue") translate([lanes[1] - 33, -150, 10]) cube([66, 178, 30]);
}

// ================================================================== Output
module place_print(r) { rotate(r) children(); }

if (part == "assembly") {
    track_preview();
    color("Gold") left_half();
    color("Orange") right_half();
    color("DimGray") pod();
    color("Gray") lid();
    color("Black") { k = pod_xy(pcb_key); translate([k[0], k[1], lid_z + lid_t]) translate([-9, -9, 0]) cube([18, 18, 8]); }
    color("White") { l = pod_xy(pcb_led); translate([l[0], l[1], lid_z + lid_t - 7]) led_lens(); }
    color("DarkGreen") { p = pod_xy([0, 0]); translate([p[0], p[1] - pcb[1], pcb_z]) cube(pcb); }
    color("Orange") sensor_bar();
}
// Print orientations: the gantry halves lie on their backs, chequers up; the pod stands on its floor;
// the lid prints upside down (top on the bed)
if (part == "left") rotate([-90, 0, 0]) translate([0, -beam_d / 2, 0]) left_half();
if (part == "right") rotate([-90, 0, 0]) translate([0, -beam_d / 2, 0]) right_half();
if (part == "pod") translate([0, 0, -pod_z0]) pod();
if (part == "lid") rotate([180, 0, 0]) translate([0, 0, -lid_z - lid_t]) lid();
if (part == "mx_blank") rotate([180, 0, 0]) mx_blank();
if (part == "led_lens") led_lens();
// the bar prints standing on its underside; at 306 mm long, lay it diagonally on a 256 mm bed
if (part == "sensor_bar") translate([0, 0, -bar_bot]) sensor_bar();


// From underneath, with the board see-through: the sensor bar and its dovetails into the legs
if (part == "underside") {
    color("BurlyWood", 0.35) translate([0, -150, -board_t]) cube([track_w, 300, board_t]);
    color("Gold") left_half();
    color("Orange") right_half();
    color("OrangeRed") sensor_bar();
}
