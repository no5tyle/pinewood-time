// pinewood-time finish gate: a 3D-printable finish-line gantry that clamps onto the track, holds a break-beam
// sensor over each lane, and houses the timer board inside its beam, under a removable top cover.
//
// Sized for the 3-lane track in "Pinewood Derby Track Build" (rev 0.5): 296 mm wide, 9 mm plywood board,
// 40 x 6 mm rails on the pattern 30 | 40 | 58 | 40 | 58 | 40 | 30 mm (lane centres 50, 148, 246 mm).
//
// Render one part with:  openscad -D 'part="left"' -o left.stl finish-gate.scad   (see build.sh)
// part = "assembly" (preview only), "left", "right", "cover", "mx_blank", "led_lens", "sensor_bar"
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
split_x = 99;                     // the beam splits between lanes 1 and 2
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

// ------------------------------------------------------------------ The timer board, in the beam
// The long board (hardware/, 178 x 28 mm) lies in the right half of the beam on two ledges, its left end
// screwed down, the Pico's end held by ribs under the top cover. The Pico's USB socket faces the right leg,
// and the cable runs down inside it.
pcb = [178, 28, 1.6];
pcb_x0 = 106;                     // the board's left end
pcb_z = beam_z0 + bot_wall + 4;   // its underside: 4 mm over the beam's floor, room for leads and cables
// board coordinates (x along it, y down from its back edge, as in KiCad) to the gate's
function pcb_xy(p) = [pcb_x0 + p[0], pcb[1] / 2 - p[1]];
pcb_holes = [[3.5, 5.5], [3.5, 22.5]];  // M2.5
pcb_led = [113.24, 15];           // D1, the status light
pcb_key = [150, 14];              // the MX switch sits over the Pico, clear of the tall parts
led_top = pcb_z + pcb[2] + 9.6;   // the top of D1
cover_x = [104, 292];             // the opening in the beam's top, over the board (and the USB plug)
cover_y = 14.5;                   // half its width
cover_screws_y = [[10, -10], [10, 0]]; // left end, right end (the USB socket's body passes the right one)
cover_t = 3;                      // the cover's deck, on top of the beam
join_bolts = [[95, 10], [95, -10]];   // M3 x 35, down through the joint (and the tongue)

// USB-C on the outside: Adafruit's panel-mount USB-C to Micro-B cable (ADA4056; drawing 4056_PCM-A-003).
// Its flange (28 x 8.5 mm, M3 screws 20 mm apart) screws to the right leg's outer face, standing upright;
// its 13 mm body runs 40.5 mm inside, along the beam, beside the Pico's plug and clear of the board.
usb_c = [-12, 140.5];             // the socket's centre on the leg's outer face: y, z
usb_cut = [9.2, 13.8];            // the hole the body goes through (y, z)
usb_screw = 10;                   // the screws, above and below the centre
usb_body = 40.5;

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
            // two rows of chequers along the top of the front, and FINISH below them
            translate([x0, -beam_d / 2, beam_z1 - 2 * chq - 1]) chequer(x1 - x0, 2 * chq);
            front_text("FINISH", 12, track_w / 2, beam_z0 + 10);
        }
        // the cable run inside, from leg to leg
        translate([x0 + 3, -beam_d / 2 + wall, beam_z0 + bot_wall]) cube([x1 - x0 - 6, beam_d - 2 * wall, beam_h - bot_wall - wall]);
        // over each lane: the receiver's pod (below), its screw, and a hole for its cable into the beam
        for (x = lanes) {
            translate([0, 0, beam_z0 - pod_s]) flat_pocket(x, pod_s - 0.2);
            translate([sen_hole_x(x), 0, beam_z0 - 1]) cylinder(d = 1.8, h = bot_wall + 0.5);
            translate([sen_x0(x) - 4, -3, beam_z0 - 1]) cube([4.5, 6, bot_wall + 2]);
        }
        // the opening in the top for the board, under the cover
        translate([cover_x[0], -cover_y, beam_z1 - wall - 1]) cube([cover_x[1] - cover_x[0], 2 * cover_y, wall + 2]);
        // the bolts that hold the two halves together, through the joint, nuts underneath
        join_holes();
    }
}

// The cable run inside, from leg to leg (cut after the legs are added, so it runs through them too)
module beam_cavity() {
    translate([x0 + 3, -beam_d / 2 + wall, beam_z0 + bot_wall]) cube([x1 - x0 - 6, beam_d - 2 * wall, beam_h - bot_wall - wall]);
}

// The USB-C socket's hole in the right leg's outer face, rounded at the corners
module usb_hole() {
    translate([x1 + 1, usb_c[0], usb_c[1]]) rotate([0, -90, 0]) linear_extrude(wall + 2)
        offset(r = 1.5) offset(delta = -1.5) square([usb_cut[1], usb_cut[0]], center = true);
}

// Inside the beam: what the board, the cover and the USB socket fix to
module beam_insides() {
    // bosses for the USB socket's two screws (they tap into 2.6 mm holes), clear of its body
    for (s = [-1, 1]) translate([x1 - wall - 6, usb_c[0], usb_c[1] + s * usb_screw]) rotate([0, 90, 0]) difference() {
        cylinder(d = 7, h = 6.01);
        translate([0, 0, -1]) cylinder(d = 2.6, h = 12);
    }
    // the board rests on a ledge along each side wall, and its left end screws to two bosses
    for (sy = [-1, 1]) translate([pcb_x0, sy > 0 ? pcb[1] / 2 - 0.5 : -beam_d / 2 + wall - 0.01, beam_z0 + bot_wall - 0.01])
        cube([pcb[0] + 4, beam_d / 2 - wall - pcb[1] / 2 + 0.51, pcb_z - beam_z0 - bot_wall + 0.01]);
    for (h = pcb_holes) let(p = pcb_xy(h)) translate([p[0], p[1], beam_z0 + bot_wall - 0.01]) difference() {
        cylinder(d = 6, h = pcb_z - beam_z0 - bot_wall + 0.01);
        cylinder(d = 2.2, h = 10);
    }
    // a block across the beam at each end of the opening for the cover's screws (cables pass under them)
    for (e = [0, 1]) let(x = e == 0 ? cover_x[0] - 4 : cover_x[1], y0 = e == 0 ? -beam_d / 2 + wall : -6)
        translate([x, y0 - 0.01, beam_z1 - wall - 9]) difference() {
            cube([4, beam_d / 2 - wall - y0 + 0.02, 9.01]);
            for (y = cover_screws_y[e]) translate([2, y - y0, -1]) cylinder(d = 2.6, h = 11);
        }
}

module join_holes() {
    for (b = join_bolts) translate([b[0], b[1], beam_z0 - 1]) {
        cylinder(d = 3.4, h = beam_h + 2);
        translate([0, 0, 0.99]) cylinder(d = 6.4, h = 2.6, $fn = 6);
    }
}

// A small pod under the beam over each lane, holding a receiver flat, LED looking down at the track
pod_s = sen[2] + 0.5;             // how far the pods hang below the beam
module sensor_pods() {
    for (i = [0 : len(lanes) - 1]) let(x = lanes[i]) difference() {
        union() {
            translate([sen_x0(x) - 2.5, -sen[1] / 2 - 2.5, beam_z0 - pod_s]) rbox([sen[0] + 5, sen[1] + 5, pod_s + 0.01], 2);
            // its lane number on the front
            translate([sen_x0(x) + sen[0] / 2, -sen[1] / 2 - 2.5 + 0.01, beam_z0 - pod_s / 2]) rotate([90, 0, 0])
                linear_extrude(relief) text(str(i + 1), size = 6, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
        }
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
// sensor bar's lead and the gate switch's cable on the left, and the USB cable on the right: 13 x 12 mm,
// so a USB-A plug goes through)
module leg_channels() {
    for (cx = [x0 + 8.5, x1 - 8.5]) {
        translate([cx - 6.5, -6, -8]) cube([13, 12, beam_z0 + bot_wall + 14]);
        translate([cx - 6.5, 0, -8]) cube([13, beam_d, 9]);
    }
}
module gantry() {
    difference() { union() { leg(-1); leg(1); beam(); sensor_pods(); } beam_cavity(); leg_channels(); usb_hole(); }
    beam_insides();
}

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
        join_holes();
    }
}
module left_half() { intersection() { gantry(); translate([x0 - 5, -100, -100]) cube([split_x - x0 + 5, 200, 400]); } }
module right_half() {
    union() {
        intersection() { gantry(); translate([split_x, -100, -100]) cube([x1 - split_x + 5, 200, 400]); }
        tongue();
    }
}

// ================================================================== The cover
// MX switch: a 14 mm square hole in a 1.5 mm plate (the switch clips into it)
mx_hole = 14;
mx_plate = 1.5;
module cover() {
    k = pcb_xy(pcb_key);
    l = pcb_xy(pcb_led);
    top = beam_z1 + cover_t;
    difference() {
        union() {
            // the deck, on top of the beam, and a plug that fills the opening
            translate([cover_x[0] - 4, -beam_d / 2, beam_z1]) rbox([cover_x[1] - cover_x[0] + 8, beam_d, cover_t], 2);
            translate([cover_x[0] + 0.3, -cover_y + 0.3, beam_z1 - wall]) cube([cover_x[1] - cover_x[0] - 0.6, 2 * cover_y - 0.6, wall + 0.01]);
            // ribs that hold the Pico's end of the board down by its edges (clear of the Pico itself)
            for (y = [11, -13.5]) translate([pcb_x0 + 140, y, pcb_z + pcb[2] + 0.2]) cube([30, 2.5, beam_z1 - wall - pcb_z - pcb[2] - 0.2 + 0.01]);
            // a light pipe down to the status LED
            translate([l[0], l[1], led_top + 1]) cylinder(d = 8, h = beam_z1 - wall - led_top - 1 + 0.01);
        }
        // screws into the blocks at each end, countersunk
        for (e = [0, 1], y = cover_screws_y[e]) translate([e == 0 ? cover_x[0] - 2 : cover_x[1] + 2, y, beam_z1 - 1]) {
            cylinder(d = 3.4, h = 10);
            translate([0, 0, 1 + cover_t - 1.8]) cylinder(d1 = 3.4, d2 = 6.6, h = 1.81);
        }
        // the MX switch: the deck is 1.5 mm thick here, with room underneath for the switch's housing
        translate([k[0] - mx_hole / 2, k[1] - mx_hole / 2, beam_z1 - wall - 1]) cube([mx_hole, mx_hole, wall + cover_t + 2]);
        translate([k[0] - 8, k[1] - 8, beam_z1 - wall - 0.01]) cube([16, 16, wall + cover_t - mx_plate]);
        // the status light's window (the lens pushes in)
        translate([l[0], l[1], led_top]) cylinder(d = 5.3, h = 40);
        // chequers along the deck, and the name, engraved
        for (i = [0 : 31], j = [0 : 1]) if ((i + j) % 2 == 0)
            translate([cover_x[0] + 2 + i * 6 + 0.1, -beam_d / 2 + 2 + j * 6 + 0.1, top - 0.6]) cube([5.8, 5.8, 1]);
        translate([cover_x[0] + 50, 10, top - 0.6]) linear_extrude(1)
            text("pinewood-time", size = 6, font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
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

// Status light lens: print in white or clear; pushes into the cover's LED window
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
    color("DimGray") cover();
    color("Black") { k = pcb_xy(pcb_key); translate([k[0] - 9, k[1] - 9, beam_z1 + cover_t]) cube([18, 18, 8]); }
    color("White") { l = pcb_xy(pcb_led); translate([l[0], l[1], beam_z1 + cover_t - 7]) led_lens(); }
    usb_socket();
    color("Orange") sensor_bar();
}
// Print orientations: the gantry halves lie on their backs, chequers up; the cover prints upside down
// (its deck on the bed)
if (part == "left") rotate([-90, 0, 0]) translate([0, -beam_d / 2, 0]) left_half();
if (part == "right") rotate([-90, 0, 0]) translate([0, -beam_d / 2, 0]) right_half();
if (part == "cover") rotate([180, 0, 0]) translate([0, 0, -beam_z1 - cover_t]) cover();
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

// The right half with its front wall cut away, the board on its ledges and the cover on top
if (part == "inside") {
    color("Orange") intersection() { right_half(); translate([90, -beam_d / 2 + wall + 0.5, 100]) cube([240, 40, 80]); }
    color("DimGray", 0.5) cover();
    color("DarkGreen") { p = pcb_xy([0, 0]); translate([p[0], p[1] - pcb[1], pcb_z]) cube(pcb); }
    color("Black") { p = pcb_xy([126, 3.5]); translate([p[0], p[1] - 21, pcb_z + pcb[2] + 2.5]) cube([51.5, 21, 1]); }
    color("Silver") { p = pcb_xy([178, 14]); translate([p[0] + 1.3, p[1] - 4, pcb_z + pcb[2] + 4]) cube([27.5, 8, 5]); }
    usb_socket();
}

// The USB-C panel socket (ADA4056) as fitted, for the previews and the clash check
module usb_socket() {
    color("DimGray") translate([x1, usb_c[0], usb_c[1]]) {
        hull() for (s = [-1, 1]) translate([0, 0, s * 9.75]) rotate([0, 90, 0]) cylinder(d = 8.5, h = 3);
        translate([-usb_body, -4.25, -6.5]) cube([usb_body, 8.5, 13]);
    }
}

// The right leg from outside, with the USB-C socket fitted
if (part == "usb") {
    color("Orange") right_half();
    color("DimGray") cover();
    usb_socket();
}

// Exploded: each part pulled out along the way it goes on, for the assembly pictures
if (part == "exploded") {
    color("Gold") translate([-45, 0, 0]) left_half();
    color("Orange") right_half();
    // the board and Pico lifted out of the beam, the cover above them, the key above that
    translate([0, 0, 42]) {
        color("DarkGreen") { p = pcb_xy([0, 0]); translate([p[0], p[1] - pcb[1], pcb_z]) cube(pcb); }
        color("Black") { p = pcb_xy([126, 3.5]); translate([p[0], p[1] - 21, pcb_z + pcb[2] + 2.5]) cube([51.5, 21, 1]); }
    }
    translate([0, 0, 75]) {
        color("DimGray") cover();
        color("White") { l = pcb_xy(pcb_led); translate([l[0], l[1], beam_z1 + cover_t + 8]) led_lens(); }
    }
    color("Black") { k = pcb_xy(pcb_key); translate([k[0] - 9, k[1] - 9, beam_z1 + cover_t + 98]) cube([18, 18, 8]); }
    // the receivers dropped out of their pods, the USB socket out of the leg
    color("Black") for (x = lanes) translate([sen_x0(x), -sen[1] / 2, beam_z0 - pod_s - 28]) cube(sen);
    translate([45, 0, 0]) usb_socket();
    // the sensor bar below the track, emitters below it
    translate([0, 0, -30]) color("OrangeRed") sensor_bar();
    color("Black") for (x = lanes) translate([sen_x0(x), -sen[1] / 2, -30 + bar_bot - 22]) cube(sen);
}
