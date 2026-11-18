# Finish gate

A 3D-printed finish-line gantry for the 3-lane track in *Pinewood Derby Track Build* (rev 0.5). It clamps
onto the end of Track E and holds a break-beam receiver over each lane. The timer board (the long version,
178 × 28 mm) lives inside the beam under a removable top cover, which carries a socket for an MX keyboard
switch as the reset (or "next heat") button and a window for the status light. Underneath, a single sensor
bar holds the three emitters in the rails and dovetails into the legs.

![The gate on the track](renders/gate-front.png)

Everything is in [`finish-gate.scad`](finish-gate.scad) (OpenSCAD), with the track's dimensions at the top
so it can be adjusted for another track. The jigs for drilling the track are in [`drill-jig.scad`](drill-jig.scad),
with the same dimensions. `./build.sh` re-exports the STLs and pictures.

## What it fits

From the build guide:

| | |
|---|---|
| Track board | 296 mm wide, 9 mm plywood |
| Rails | 40 × 6 mm, at 30 \| 40 \| 58 \| 40 \| 58 \| 40 \| 30 mm across the board |
| Lane (rail) centres | 50, 148 and 246 mm from the left edge |
| Under the board at the finish | 19 mm: the pine supports hold it off the floor |
| Track E tail support | 42-84 mm from the tail end, screwed at 63 mm |

The gate's legs stand just outside the board edges (0.5 mm clear), with a jaw reaching 8 mm onto the top
of the board (the outer wheels run from 15 mm in). The sensor bar runs under the board from leg to leg and
locks into both, so the gate and bar together make a frame around the board; an M4 thumbscrew in each end
of the bar presses the board up against the top jaws. The beam is 40 × 33 mm and 120 mm above the race
surface; the sensor pods under it leave 111 mm for the cars. Put the finish line at least
150 mm from the tail end, clear of the tail support and its screws; the drawings use 200 mm.

The track has no stopping area after the finish, so plan something to catch the cars (a towel, or a
padded board across the end).

## Parts

| File | Print | Notes |
|---|---|---|
| `stl/left.stl` | 1 | left leg and the beam to between lanes 1 and 2 (124 × 185 mm on the bed) |
| `stl/right.stl` | 1 | right leg and the rest of the beam (230 × 185 mm on the bed) |
| `stl/cover.stl` | 1 | the top cover over the board (204 × 40 mm); prints upside down. MX switch socket and status light window |
| `stl/mx_blank.stl` | 1 | fills the switch socket if you don't fit a key |
| `stl/led_lens.stl` | 1 | print in white or clear; pushes into the status light window |
| `stl/panel_left.stl`, `stl/panel_right.stl` | 1 each | the faceplates for the [sockets](#the-sockets-on-the-back-of-the-legs) on the back of the legs (left: SENSORS, START, LINK IN; right: LINK OUT). Print face down |
| `stl/sensor_bar.stl` | 1 | under the track: the three emitters, and a dovetail into each leg. 307 × 30 mm: lay it diagonally on a 256 × 256 mm bed |
| `stl/drill_jig_left.stl` | 1 | drilling jig for lanes 1 and 2 (167 × 217 mm on the bed). Only needed once, to drill the track |
| `stl/drill_jig_right.stl` | 1 | drilling jig for lane 3 (69 × 217 mm) |
| `stl/drill_bush.stl` | 1 | the pilot bush that drops into the jigs' guides |

All of it prints without supports at 0.2 mm in PLA or PETG, 3 walls, 20% infill. The halves lie on their
backs with the chequers up. For two colours, change filament for the last 0.8 mm of each half, where
the chequers and FINISH start (at 40.0 mm).

**Hardware:**

| Qty | Item | For |
|---|---|---|
| 2 | M3 × 35 bolts and nuts | down through the joint, holding the two halves together |
| 2 | M2.5 × 6 self-tapping screws | the board's left end onto its bosses |
| 4 | M3 × 12 countersunk self-tapping screws | the cover |
| 2 | M3 × 40 bolts and nuts | the lock pins through the legs and the bar's dovetails |
| 6 | M2 × 10 self-tapping screws | one through each sensor's mounting hole |
| 2 | M4 × 16 bolts and M4 nuts | the thumbscrews: the nuts press into the underside of the bar's ends, and the bolts screw up against the board until snug |
| 1 | MX switch (any) and a 1u keycap | optional: the button |
| 1 | Adafruit panel-mount USB-C to Micro-B cable (ADA4056), with its two M3 screws | optional: USB-C on the right leg |
| 1 | GX12 2-pin panel socket and plug | the SENSORS socket, and the sensor bar's plug |
| 1 | RJ11 keystone jack (6P4C), and an RJ11 phone socket for the start gate | the START socket |
| 2 | RJ45 keystone jacks, Cat5e or Cat6, unshielded (punch-down or tool-less) | LINK IN and LINK OUT (only to link gates) |
| 4 | M3 × 10 countersunk self-tapping screws | the faceplates |

## Sensors: will they fit?

Yes, lying flat. Each half of the Adafruit 3 mm break-beam sensor is an Omron housing 20 × 10 × 8 mm.
Its LED looks out of the broad face, 5 mm from one end. 10 mm from the LED is a 2.4 mm mounting hole
through the housing, and the cable leaves the end beside it. Both mounts hold the sensor by an M2 × 10
self-tapping screw through that hole, with its LED on the lane centre and its cable towards the left.

- **In the gantry:** a small pod under the beam over each lane holds a receiver flat, LED looking down.
  Its cable goes out of the pod's end and up into the beam. The pods hang 8.5 mm below the beam, so the
  clearance over the track is 111 mm.
- **In the track:** the sensor bar holds each emitter flat in a pocket on its underside, LED up. Its light
  goes up a 6 mm bore through the bar and a round turret that rises into a 10 mm hole through the rail
  and board, ending 1 mm below the rail top. The bar is 13 mm deep under the board, leaving 6 mm to the
  floor of the 19 mm the supports allow.

![Sensor in the track](renders/sensor-fit.svg)

![The sensor bar under the track](renders/gate-underside.png)

Put the **emitters in the track, shining up, and the receivers in the gantry, looking down**. That way the
receivers face the track, not the hall's lights or a sunny window.

### The sensor bar

One solid piece. Each emitter goes into its pocket from underneath, LED face up, and is screwed in with
its M2 screw. Its cable leaves the pocket's left end into a groove along the underside of the bar. Emitters
only need power, so join all three in the groove (reds together, blacks together) into one 2-core lead,
and tape over the groove to keep the wires in. The lead leaves the left end, with a GX12 plug on it, and
plugs into the **SENSORS** socket on the back of the left leg (see [the sockets](#the-sockets-on-the-back-of-the-legs)),
so the bar comes off for packing without opening anything.

Each end of the bar is a dovetail that slides up into a matching socket in the leg. The socket is closed
at the top, so the bar stops just under the board. An M3 pin through the leg and the dovetail locks it.

### Modifying the track

Each lane needs a 10 mm hole through the rail and board at the finish line, 200 mm from the tail end. The
sensor bar's turrets are 9.6 mm across, so the holes have to be within about 0.2 mm of where the bar
expects them. Two printed jigs ([`drill-jig.scad`](drill-jig.scad)) put them there.

![The drilling jigs on the track](renders/drill-jig.png)

Each jig is an L. Its long side is a fence that runs down the board's side edge, with a lip at the end
that hooks over the tail end. Its short side crosses the track, sitting on the rail tops, with a 16 mm
tall drill guide over each rail. The fence sets how far across the track each hole is, from the same board
edges the gate's legs stand against. The lip sets the finish line at 200 mm. The fence also keeps the
holes square to the track.

- **`drill_jig_left`** goes against the left edge and drills lanes 1 and 2 (50 and 148 mm in).
- **`drill_jig_right`** goes against the right edge and drills lane 3 (50 mm in from that side).
- **`drill_bush`** drops into either guide to steer a 3 mm pilot drill through the middle.

Both jigs print upside down on their flat top, with no supports. Check the bush slides into a guide and
a 10 mm bit turns freely in one before using them. Printers vary, so open the guide with a 10 mm drill
by hand if it's tight.

On Track E:

1. Check the tail end is square to the board edges. The lip measures from it, so a crooked end puts
   the lanes at different distances.
2. Set the left jig on the track, the fence against the left edge and the lip against the tail end, and
   clamp it there through the arm.
3. For each of its guides: drop the bush in and drill a 3 mm pilot right through the rail and board.
   Lift the bush out.
4. Do the same with the right jig against the right edge, for lane 3.
5. Lift the jigs off. From underneath, drill 10 mm a few millimetres in at each pilot, so the
   underside doesn't splinter.
6. Clamp the jigs back on and finish each hole from the top with the 10 mm bit through the guide. A
   brad-point bit follows the pilot best.
7. Try the sensor bar in the holes before fitting the gate. If a turret binds, ease that hole with a
   round file rather than forcing it.

The sensor bar's turrets go up into these holes when the gate is fitted (below). Nothing is screwed to
the track, so the gate and bar lift off for packing.

That leaves 15 mm of rail either side of the hole, so the rail still guides the wheels. The turrets stop
1 mm below the rail top, out of reach of the cars.

The bottom of a car body breaks each beam as the car passes over the rail.

## The board in the beam

![The board inside the beam](renders/board-in-beam.png)

The board lies in the right half of the beam, from just past the joint to near the right leg. It rests on
a ledge along each side wall, 4 mm above the beam's floor so its leads and the sensor cables pass under
it; two M2.5 screws hold its left end to bosses, and two ribs under the cover hold the Pico's end down by
its edges. The Pico's USB socket faces the right leg.

**USB-C on the outside:** Adafruit's [panel-mount USB-C to Micro-B cable](https://core-electronics.com.au/panel-mount-cable-usb-c-to-micro-b-male-30cm.html)
(ADA4056, about A$10.60 at Core Electronics) makes the gate all-in-one. Its USB-C socket screws to the
outer face of the right leg at beam height, standing upright, and its Micro-B end plugs into the Pico; the
race computer then connects with any USB-C cable. The socket's 13 mm body runs 40.5 mm inside the beam,
in front of the Pico's plug and clear of the board, and its two M3 screws tap into bosses inside the leg.
Push the spare length of its 30 cm lead down the right leg's channel.

![The USB-C socket on the right leg](renders/usb-socket.png)

Without it, plug an ordinary Micro-B cable into the Pico and feed its far end down the right leg's
channel (13 × 12 mm, so a USB-A plug goes through) and out of the slot at the back, at track level.

Above the board there's 20 mm to the cover. That's room for the upright resistors, the JST sockets with
their plugs in, and the Pico on short headers; the MX key sits over the Pico. Solder the Pico flat or on
2.5 mm headers, not on tall sockets.

## The sockets on the back of the legs

![The sockets on the back of the left leg](renders/link-sockets.png)

Everything that's plugged in at setup goes into sockets set into the back of the legs, behind faceplates
that sit flush with them. Each socket's body sits in its own pocket inside the leg, which opens into the
leg's cable channel; a short lead goes from it up the channel and along the beam to the board, and stays
there.

| Socket | Where | Connector | Plug in | Inside, to the board |
|---|---|---|---|---|
| **SENSORS** | left leg, bottom | GX12 2-pin (an M12 locking "aviation" connector) | the sensor bar's lead | AUX (J12): pin 1 GND, pin 3 5V |
| **START** | left leg, middle | RJ11 keystone jack (6P4C) | an ordinary phone extension cable from the start gate | GATE (J9) |
| **LINK IN** | left leg, top | RJ45 keystone jack | a network patch cable from the gate on the left ([linking gates](#linking-gates)) | LINK IN (J13) |
| **LINK OUT** | right leg | RJ45 keystone jack | a network patch cable to the gate on the right | LINK OUT (J14) |

SENSORS (which carries 5V) and START (a switch) use different connectors, so the start gate can't be plugged
into the 5V and short it.

- **Faceplates:** `panel_left.stl` (three sockets) and `panel_right.stl` (one), printed face down. Each
  socket's name is engraved under it.
- **Fitting:** snap the keystone jacks into the faceplate from behind, and fit the GX12 socket through its
  round hole with its nut behind. Make each one's lead (below), feed the leads into the pockets in the back
  of the leg and up its channel to the board, push the sockets into their pockets until the faceplate sits
  flush in its recess, and screw it in with two M3 × 10 countersunk self-tapping screws.
- **Keystone jacks** vary a little: the pockets allow 33 mm behind the jack's front. The RJ11 jack is the
  same keystone size as the RJ45 ones.

**SENSORS lead:** from the GX12 socket's pins 1 and 2 to a 3-way 2.54 mm housing on the AUX header: pin 1
(GND) and pin 3 (5V), with the middle empty. GX12 pin 1 is 5V, pin 2 GND. On the sensor bar's lead, fit the
matching GX12 plug (pin 1 red, pin 2 black).

**START lead:** from the RJ11 jack's middle pair (pins 3 and 4) to a 2-pin JST XH plug for GATE (J9). Which
way round doesn't matter for a switch. At the start gate, fit a matching RJ11 socket (a surface-mount phone
socket is fine) wired to the switch, so a phone extension cable joins the two. Phone cables come ready-made
in 10 and 15 m lengths.

## Linking gates

Tracks side by side each get a gate, and the gates [link](../../docs/wiring.md#linking-gates) into one
timer: every gate is printed and built the same. On race day, joining them is just ordinary network patch
cables, from one gate's **LINK OUT** to the next gate's **LINK IN**.

![The link sockets on the back of the legs](renders/link-sockets.png)

The link sockets are RJ45 keystone jacks (the snap-in kind for wall plates) among the
[sockets](#the-sockets-on-the-back-of-the-legs) on the back of the legs: **LINK IN** at the top of the left
leg's faceplate, **LINK OUT** on the right leg's. Put the main gate
(the one plugged into the computer) on the left-hand track, so the links run left to right. The main gate's
USB powers the gates after it, so they don't need a USB lead.

Each jack joins the board's link socket by a short lead (about 40 cm: up the leg's channel and
along the beam). Four wires, the same for LINK IN and LINK OUT; the jack's other pins stay empty:

| Board link socket (JST XH) | RJ45 jack pin | T568B colour |
|---|---|---|
| 1 (5V) | 4 | blue |
| 2 (GND) | 7 | white-brown |
| 3 | 1 | white-orange |
| 4 | 3 | white-green |

A length cut from a patch cable makes a good lead: punch its colours down on the jack by the T568B labels
(or click them into a tool-less jack), crimp the four above into a 4-pin JST XH plug, and cut the other four
back.

Wired this way, plugging a gate into a router or a laptop's network port by mistake does no harm: a network
port joins only the two wires of a pair, and each pair here has one wire connected. It just won't link. Never
plug a gate into a passive PoE injector (one that's always on), which puts 24-48 V on the cable.

## Assembly

1. Press the M4 nuts into the hex pockets under the ends of the sensor bar, the M3 nuts into the hex
   pockets at the back of the legs' feet and under the beam at the joint.
2. Slide the right half's tongue into the left half's beam and bolt them together with the two M3 × 35
   bolts down through the joint.
3. Feed each receiver's cable up through its pod's end into the beam, then press the receiver into the
   pod, LED down, and screw it in with an M2 × 10.
4. Screw the emitters into the sensor bar's pockets, LED up, join their wires in the groove into one
   2-core lead out of the left end, tape the groove over, and fit the GX12 plug. Fit the sockets into
   the faceplates with their leads, run the leads up the legs' channels and along the beam, and screw the
   faceplates on ([the sockets](#the-sockets-on-the-back-of-the-legs)). SENSORS goes to the AUX header
   (J12), START to GATE (J9), the links to J13 and J14.
5. Crimp each receiver to its lane's 4-pin JST XH plug: red to pin 4 (5V), black to pin 3 (GND), white
   to pin 2 (SIG), pin 1 empty. The emitters don't go to the lane plugs; they're powered by the bar's lead.
6. Wire the MX switch's two pins to a 2-pin JST XH plug for the RESET socket (J10).
7. USB-C socket: feed its Micro-B end in through the hole in the right leg from outside, and screw the
   socket's flange on with its two M3 screws. (Or, without it, feed an ordinary cable down the right leg
   from inside.) Lower the board onto its ledges, Pico to the right, plug the Micro-B into the Pico and
   everything else into its sockets, push the spare USB lead down the right leg, and screw the board's
   left end down.
8. Push the lens into the cover, clip in the switch (or the blank), and screw the cover down: its ribs
   hold the Pico's end of the board.
9. On the track: set the gate on the board at the finish line with its legs either side. From
   underneath, push the sensor bar up so its turrets go into the rail holes and its ends slide up into
   the legs' sockets, until it stops. Push the M3 pins through the legs and the bar, fit their nuts,
   and tighten the thumbscrews up against the board. On the back of the left leg, plug the bar's lead into
   SENSORS and the start gate's phone cable into START.

The gate switch is at the far end of the track, about 9.7 m away, so its 2-core cable is long. That's
fine for a switch to ground with the board's pull-up, but run it along the underside of the track, away
from mains leads.

## Timing note

Adafruit gives the receiver's response time as under 2 ms. If every lane's receiver responds equally
quickly, the delay cancels out between lanes, but that isn't guaranteed. So treat finishes within a couple
of thousandths of a second as ties until it has been tested on the track: run one car down each lane in
turn, and swap sensors between lanes to compare.
