#include "display.h"

#include "board.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define I2C i2c0

/* Segments: bit 0 = a ... bit 6 = g, bit 7 = decimal point */
static uint8_t segments_for(char c) {
    switch (c) {
    case '0': return 0x3F;
    case '1': return 0x06;
    case '2': return 0x5B;
    case '3': return 0x4F;
    case '4': return 0x66;
    case '5': return 0x6D;
    case '6': return 0x7D;
    case '7': return 0x07;
    case '8': return 0x7F;
    case '9': return 0x6F;
    case '-': return 0x40;
    case 'P': return 0x73;
    case 'E': return 0x79;
    case 'r': return 0x50;
    case 'o': return 0x5C;
    case 'n': return 0x54;
    case 'F': return 0x71;
    default: return 0x00;
    }
}

static bool write_bytes(uint8_t addr, const uint8_t *data, size_t len) {
    return i2c_write_timeout_us(I2C, addr, data, len, false, 2000) == (int)len;
}

static bool command(uint8_t addr, uint8_t cmd) { return write_bytes(addr, &cmd, 1); }

uint8_t display_init(uint8_t lanes, uint8_t brightness) {
    i2c_init(I2C, 400 * 1000);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    /* The board has 4.7k pull-ups; these help if a display is plugged in without them */
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    uint8_t present = 0;
    for (uint8_t lane = 0; lane < lanes; lane++) {
        uint8_t addr = DISPLAY_ADDR_FIRST + lane;
        if (!command(addr, 0x21)) continue; /* oscillator on: no answer means no display */
        command(addr, 0x81);                /* display on, no blinking */
        present |= (uint8_t)(1u << lane);
    }
    display_brightness(present, brightness);
    return present;
}

void display_brightness(uint8_t present, uint8_t brightness) {
    if (brightness > 15) brightness = 15;
    for (uint8_t lane = 0; lane < 8; lane++)
        if (present & (1u << lane)) command(DISPLAY_ADDR_FIRST + lane, (uint8_t)(0xE0 | brightness));
}

void display_text(uint8_t lane, const char *text) {
    /* The backpack's digits live at RAM addresses 0, 2, 6 and 8 (4 is the colon) */
    static const uint8_t positions[4] = {0x00, 0x02, 0x06, 0x08};
    uint8_t digits[4] = {0};
    int d = 0;
    for (const char *c = text; *c && d < 4; c++) {
        if (*c == '.') {
            if (d > 0) digits[d - 1] |= 0x80;
            continue;
        }
        digits[d++] = segments_for(*c);
        if (c[1] == '.') {
            digits[d - 1] |= 0x80;
            c++;
        }
    }
    /* Right-align short text, e.g. "P1" */
    if (d < 4) {
        int shift = 4 - d;
        for (int i = 3; i >= 0; i--) digits[i] = i - shift >= 0 ? digits[i - shift] : 0;
    }
    uint8_t buf[11] = {0};
    buf[0] = 0x00; /* start at RAM address 0 */
    for (int i = 0; i < 4; i++) buf[1 + positions[i]] = digits[i];
    write_bytes(DISPLAY_ADDR_FIRST + lane, buf, sizeof buf);
}
