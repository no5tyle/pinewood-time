/*
 * The pinewood-time board's wiring: which Pico pin does what. Matches hardware/ (the PCB) and
 * docs/wiring.md. Lanes are on the Pico's left-hand pins, everything else on the right.
 */
#ifndef PT_BOARD_H
#define PT_BOARD_H

/* Lane finish sensors: lane 1 on GP0 to lane 8 on GP7, so they all read in one go. Boards from v1.2 have
 * sockets for lanes 1 to 4 (more lanes come from linked gates); v1.1 boards have all eight */
#define PIN_LANE_FIRST 0

#define PIN_I2C_SDA 16 /* lane displays (I2C0) */
#define PIN_I2C_SCL 17
#define PIN_GATE 18    /* start gate switch (gate 1) */
#define PIN_RESET 19   /* reset button (on the board, and the RESET socket) */
#define PIN_LED_R 20   /* status light */
#define PIN_LED_G 21
#define PIN_LED_B 22
/* Spare pins on the AUX header: GP26, GP27, GP28. Start gates 2 to 4 for tracks side by side ("#gate2 4-6"),
 * otherwise free (buzzer, start solenoid driver, ...) */
#define PIN_AUX_1 26
#define PIN_AUX_2 27
#define PIN_AUX_3 28
#define PIN_GATE_2 PIN_AUX_1
#define PIN_GATE_3 PIN_AUX_2
#define PIN_GATE_4 PIN_AUX_3

/* Links to other gates (link.c), on the Pico's two UARTs; unconnected on v1.1 boards.
 * LINK IN (towards the main gate) on UART0, LINK OUT (towards the next gate) on UART1 */
#define PIN_LINK_IN_TX 12
#define PIN_LINK_IN_RX 13
#define PIN_LINK_OUT_TX 8
#define PIN_LINK_OUT_RX 9
#define LINK_BAUD 115200

/* Lane displays: HT16K33 4-digit LED backpacks, lane 1 at 0x70 to lane 8 at 0x77 */
#define DISPLAY_ADDR_FIRST 0x70

#endif
