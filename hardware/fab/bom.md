# pinewood-time board: parts for one board

All through-hole. Resistors are 1/4 W, 5% or better, 0207 size (6.3 mm body, 7.62 mm hole spacing).

| Qty | References | Value | Description | Part number | LCSC |
|---|---|---|---|---|---|
| 1 | U1 | Raspberry Pi Pico | Raspberry Pi Pico (RP2040) or Pico 2 (RP2350), on 2x20 pin headers or soldered flat | SC0915 |  |
| 8 | J1 J2 J3 J4 J5 J6 J7 J8 | B4B-XH-A | Lane sensor socket, JST XH 4-pin, 2.5 mm | JST B4B-XH-A | C144395 |
| 8 | R1 R2 R3 R4 R5 R6 R7 R8 | 180 | IR emitter current limit, 1/4 W (about 20 mA at 5 V) |  |  |
| 10 | R11 R12 R13 R14 R15 R16 R17 R18 R31 R33 | 10k | Sensor pull-up to 3.3 V, 1/4 W; Gate pull-up to 3.3 V, 1/4 W; Reset pull-up to 3.3 V, 1/4 W |  |  |
| 10 | R21 R22 R23 R24 R25 R26 R27 R28 R32 R34 | 1k | Series protection for the Pico input, 1/4 W; Series protection, 1/4 W |  |  |
| 2 | J9 J10 | B2B-XH-A | Start gate switch socket, JST XH 2-pin; Reset button socket (the gate's MX key), JST XH 2-pin | JST B2B-XH-A | C158012 |
| 2 | R35 R36 | 4.7k | I2C pull-up, 1/4 W |  |  |
| 3 | R37 R38 R39 | 220 | Status light, red, 1/4 W; Status light, green, 1/4 W; Status light, blue, 1/4 W |  |  |
| 1 | J11 | 1x04 header | I2C header for lane displays (HT16K33 4-digit backpacks, address 0x70 + lane - 1), 1x4 2.54 mm |  |  |
| 1 | J12 | 1x06 header | Spare pins: GND, 3V3, 5V (powers the gate's track emitters), GP26, GP27, GP28, 1x6 2.54 mm |  |  |
| 1 | D1 | RGB | Status light, 5 mm RGB LED, common cathode, pins R-K-G-B (cathode longest) |  |  |
| 1 | C1 | 100uF 10V | 5 V supply bulk, electrolytic, 6.3 mm, 2.5 mm pitch |  |  |
| 1 | C2 | 100nF | 3.3 V decoupling, ceramic, 5 mm pitch |  |  |
