# A 3-lane kit

Everything to build a 3-lane timer with off-the-shelf IR break-beam sensors, bought from Core Electronics
(Australia) with the boards from JLCPCB. Lane displays are left out; [adding them](#adding-lane-displays)
later doesn't change the board.

Prices are rough, in Australian dollars with GST, from late 2026. Check the listings before ordering.

## Shopping list

**JLCPCB:** 5 boards from [`hardware/fab/pinewood-time-gerbers.zip`](../hardware/fab/pinewood-time-gerbers.zip)
(5 is the minimum order). 2 layers, 178 × 28 mm, 1.6 mm, anything else at the defaults.

**Core Electronics:**

| Qty | Item | For |
|---|---|---|
| 1 | Raspberry Pi Pico H (headers fitted) | the timer |
| 3 | [Adafruit IR break-beam sensor, 3 mm (ADA2167)](https://core-electronics.com.au/catalog/product/view/sku/ADA2167) | lanes 1-3 |
| 1 | [SparkFun resistor kit, 1/4 W, 500 pcs (COM-10969)](https://core-electronics.com.au/resistor-kit-1-4w-500-total.html) | 5 × 10k, 5 × 1k, 3 × 220 Ω |
| 1 | DFRobot JST connector kit (FIT0255) | lane, gate and reset sockets, plugs and crimps |
| 1 | [100 nF ceramic capacitor (CE05188)](https://core-electronics.com.au/100nf-ceramic-disc-capacitor.html) | C2 |
| 1 | [100 µF radial capacitor (CE05258)](https://core-electronics.com.au/radial-capacitor-100uf.html) | C1: check it's 6.3 mm across with legs 2.5 mm apart |
| 1 | [SparkFun RGB LED, common cathode (COM-09264)](https://core-electronics.com.au/catalog/product/view/sku/COM-09264) | status light: check its legs run R, K, G, B (K longest) |
| 1 | An MX keyboard switch and a 1u keycap | the button, in the finish gate's cover (or any push-to-make button on a cable) |
| 1 | Lever microswitch | start gate |
| 1 | [Adafruit panel-mount USB-C to Micro-B cable, 30 cm (ADA4056)](https://core-electronics.com.au/panel-mount-cable-usb-c-to-micro-b-male-30cm.html) | USB-C socket on the finish gate's right leg |
| 1 | USB-C data cable (not a charge-only one) | gate to the race computer |
| ~5 m | 4-core cable (alarm or phone cable is fine) | about 1.5 m a lane |

**For the sockets on the back of the finish gate's legs** (SENSORS and START; see
[the gate](../hardware/gate/README.md#the-sockets-on-the-back-of-the-legs)). These aren't at Core
Electronics; Jaycar or Altronics have them, or AliExpress for less:

| Qty | Item | For | About |
|---|---|---|---|
| 1 | GX12 2-pin panel socket and plug ("GX12 aviation connector, 2 pin") | SENSORS: the sensor bar's lead | A$3-5 |
| 1 | RJ11 keystone jack, 6P4C | START, in the left leg | A$4-6 |
| 1 | RJ11 phone wall socket (surface mount) | at the start gate, wired to the microswitch | A$5 |
| 1 | RJ11 phone extension cable, 10 m (15 m if the track's longer) | start gate to finish gate | A$8-12 |
| 4 | M3 × 10 countersunk self-tapping screws | the two faceplates | |

Optional: a panel-mount push button on 2-core cable, for a button at the finish line. It resets the
timer, or with `#button next` calls up the next heat in the race software
([details](protocol.md#the-reset-button-next-heat)).

About A$85 plus the boards and the gate's printing and fasteners. The resistor and connector kits have
plenty left over for more boards.

Notes on the kits:

- The DFRobot JST kit's parts are on a 2.54 mm pitch rather than JST XH's 2.5 mm. With 4 pins or fewer the
  difference doesn't matter.
- R1-R8 (180 Ω) aren't needed with these sensors: they only power bare IR LEDs (option B in
  [wiring](wiring.md)). If you build that version, the kit's 150 Ω resistors work (about 25 mA).

## What to solder

| Fit | Leave empty |
|---|---|
| U1 (Pico) | J4 (lane 4) |
| J1, J2, J3 (lane sockets) | R1-R4 |
| R11, R12, R13 (10k) | R14, R24 |
| R21, R22, R23 (1k) | J11, R35, R36 (displays) |
| J9, R31 (10k), R32 (1k): gate | |
| J10, R33 (10k), R34 (1k): reset | |
| J12: 5V and GND for the finish gate's emitters | |
| D1, R37, R38, R39 (220 Ω): status light | |
| C1, C2 | J13, J14, D2, R41-R46 (links: fit these to [join gates](wiring.md#linking-gates)) |

The empty places do no harm and can be filled later for more lanes. Solder the lowest parts first, and see
the [README](../README.md#3-solder-it) for which way round the parts go.

## Wiring

**Each lane:** join the emitter's and receiver's wires at the finish line and run one cable back:

| Sensor wires | Lane plug |
|---|---|
| both red | pin 4 (5V) |
| both black | pin 3 (GND) |
| receiver white | pin 2 (SIG) |

Pin 1 stays empty. Mount each pair just past the finish line, either facing across the lane or with the
emitter on a bridge above pointing down at the receiver set into the track. The overhead layout stops one
lane's beam reaching the next lane's receiver.

**Start gate:** wire the microswitch's common and normally-closed (NC) terminals to the middle pair of the
RJ11 phone socket at the start (the two middle screws), so the switch is closed while the gate is up and
opens as it drops. The phone extension cable joins it to the **START** socket on the finish gate. If it works
the other way round, send `#gate-trip low` then `#save`.

**Inside the finish gate**, once: START's RJ11 jack (middle pair, pins 3 and 4) to a 2-pin plug for the gate
socket (J9), and the SENSORS socket (GX12 pin 1 5V, pin 2 GND) to the AUX header (J12): GND on pin 1, 5V on
pin 3. The sensor bar's lead gets the GX12 plug: pin 1 red, pin 2 black. See
[the gate](../hardware/gate/README.md#the-sockets-on-the-back-of-the-legs).

## Setting up

1. Flash `dist/pinewood-time-pico.uf2`: hold BOOTSEL while plugging the Pico in, then copy the file onto
   the drive that appears.
2. In a serial terminal, send `#lanes 3`, then `#save`.
3. Hold the reset button while plugging in for test mode. Block each beam in turn and check the right
   lane shows `1`.
4. In the race software, choose the PDT timer and 3 lanes.

## Linking more tracks

Each extra track gets its own finish gate with the same parts as above, and the gates link with network patch
cables ([how](wiring.md#linking-gates)). Per gate, add:

| Qty | Item | For | About |
|---|---|---|---|
| 1 | 1N5817 Schottky diode | D2: power down the link | A$0.50 |
| 2 | RJ45 keystone jacks, Cat5e or Cat6, unshielded | LINK IN and LINK OUT | A$4-6 each |
| 1 | Network patch cable, 1-2 m | to the next gate (one fewer than the gates) | A$5 |
| 1 | Network patch cable, about 1 m, to cut up | the two leads inside the gate | |

From the kits: two more 4-pin JST sockets (J13, J14), and R41-R46 (four 1k, two 10k).

## Adding lane displays

One 4-digit display per lane at the finish line: a running clock while the car is on the track, then its
time, then (once the race is over) its time and place in turn. See [wiring](wiring.md#lane-displays-displays).

| Qty | Item | About |
|---|---|---|
| 3 | [Adafruit 0.56" 4-digit display with I2C backpack, red (ADA878)](https://core-electronics.com.au/adafruit-0-56-4-digit-7-segment-display-w-i2c-backpack-red.html), a kit to solder | A$18.50 each |
| | or the [assembled STEMMA QT version](https://core-electronics.com.au/assembled-adafruit-056-4-digit-7-segment-display-w-i2c-backpack-qt-green.html), chained with QT cables | A$25 each |
| ~4 m | 4-core cable to chain them | |

On the board, fit J11 (a 1×4 header) and R35, R36 (4.7k, in the resistor kit). On the displays, bridge the
address pads: lane 1 none, lane 2 A0, lane 3 A1. Generic HT16K33 modules work too if they have A0-A2 pads.
