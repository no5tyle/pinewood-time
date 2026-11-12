/* Optional lane displays: an HT16K33 4-digit 7-segment backpack per lane, on the I2C header */
#ifndef PT_DISPLAY_H
#define PT_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

/* Looks for a display at each lane's address; returns which lanes have one (bit 0 = lane 1) */
uint8_t display_init(uint8_t lanes, uint8_t brightness);
void display_brightness(uint8_t present, uint8_t brightness);
/* Shows text such as "2.345", "12.34", "----", "  P1" or "8.8.8.8." on a lane's display */
void display_text(uint8_t lane, const char *text);

#endif
