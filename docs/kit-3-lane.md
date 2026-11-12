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
| ~6 m | 4-core cable (alarm or phone cable is fine) | about 1.5 m a lane, and the gate |

Optional: a panel-mount push button on 2-core cable, for a button at the finish line. It resets the
timer, or with `#button next` calls up the next heat in the race software
([details](protocol.md#the-reset-button-next-heat)).

About A$60 plus the boards. The resistor and connector kits have plenty left over for more boards.

Notes on the kits:

- The DFRobot JST kit's parts are on a 2.54 mm pitch rather than JST XH's 2.5 mm. With 4 pins or fewer the
  difference doesn't matter.
- R1-R8 (180 Ω) aren't needed with these sensors: they only power bare IR LEDs (option B in
  [wiring](wiring.md)). If you build that version, the kit's 150 Ω resistors work (about 25 mA).

## What to solder

| Fit | Leave empty |
|---|---|
| U1 (Pico) | J4-J8 (lanes 4-8) |
| J1, J2, J3 (lane sockets) | R1-R8 |
| R11, R12, R13 (10k) | R14-R18, R24-R28 |
| R21, R22, R23 (1k) | J11, R35, R36 (displays) |
| J9, R31 (10k), R32 (1k): gate | |
| J10, R33 (10k), R34 (1k): reset | |
| J12: 5V and GND for the finish gate's emitters | |
| D1, R37, R38, R39 (220 Ω): status light | |
| C1, C2 | |

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

**Gate:** wire the microswitch's common and normally-closed (NC) terminals to the gate plug, so the switch is
closed while the gate is up and opens as it drops. If it works the other way round, send `#gate-trip low`
then `#save`.

## Setting up

1. Flash `dist/pinewood-time-pico.uf2`: hold BOOTSEL while plugging the Pico in, then copy the file onto
   the drive that appears.
2. In a serial terminal, send `#lanes 3`, then `#save`.
3. Hold the reset button while plugging in for test mode. Block each beam in turn and check the right
   lane shows `1`.
4. In the race software, choose the PDT timer and 3 lanes.

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
