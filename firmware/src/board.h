/*
 * The pinewood-time board's wiring: which Pico pin does what. Matches hardware/ (the PCB) and
 * docs/wiring.md. Lanes are on the Pico's left-hand pins, everything else on the right.
 */
#ifndef PT_BOARD_H
#define PT_BOARD_H

/* Lane finish sensors: lane 1 on GP0 to lane 8 on GP7, so all eight read in one go */
#define PIN_LANE_FIRST 0

#define PIN_I2C_SDA 16 /* lane displays (I2C0) */
#define PIN_I2C_SCL 17
#define PIN_GATE 18    /* start gate switch */
#define PIN_RESET 19   /* reset button (on the board, and the RESET socket) */
#define PIN_LED_R 20   /* status light */
#define PIN_LED_G 21
#define PIN_LED_B 22
/* Spare pins on the AUX header (buzzer, start solenoid driver, ...): GP26, GP27, GP28 */
#define PIN_AUX_1 26
#define PIN_AUX_2 27
#define PIN_AUX_3 28

/* Lane displays: HT16K33 4-digit LED backpacks, lane 1 at 0x70 to lane 8 at 0x77 */
#define DISPLAY_ADDR_FIRST 0x70

#endif
