/*
 * pinewood-time: a pinewood derby race timer for the Raspberry Pi Pico.
 *
 * Talks to race software over USB (it shows up as a serial port) using the PDT timer protocol. The timing
 * itself happens in an interrupt: each lane's beam break and the start gate opening are stamped with the
 * Pico's microsecond clock the moment they happen, then handed to the timer core (timer_core.c).
 */
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "display.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "settings.h"
#include "timer_core.h"

/* ---------------------------------------------------------------- Events from the interrupt */
typedef struct {
    uint8_t kind; /* 0..7 = that lane, EVENT_GATE, EVENT_RESET */
    uint64_t at_us;
} event;

#define EVENT_GATE 0x10
#define EVENT_RESET 0x11
#define QUEUE_SIZE 64

static volatile event queue[QUEUE_SIZE];
static volatile uint32_t queue_head, queue_tail;
static volatile uint64_t last_reset_us;
static pt_config cfg;

static void push(uint8_t kind, uint64_t at_us) {
    uint32_t next = (queue_head + 1) % QUEUE_SIZE;
    if (next == queue_tail) return; /* full: can't happen at race speeds */
    queue[queue_head].kind = kind;
    queue[queue_head].at_us = at_us;
    queue_head = next;
}

static void on_edge(uint gpio, uint32_t events) {
    uint64_t now = time_us_64(); /* first, so the time is when it happened */
    if (gpio < PIN_LANE_FIRST + PT_MAX_LANES) {
        push((uint8_t)(gpio - PIN_LANE_FIRST), now);
    } else if (gpio == PIN_GATE) {
        push(EVENT_GATE, now);
    } else if (gpio == PIN_RESET && (events & GPIO_IRQ_EDGE_FALL)) {
        if (now - last_reset_us > 250000) { /* debounce the button */
            last_reset_us = now;
            push(EVENT_RESET, now);
        }
    }
    (void)events;
}

static uint32_t trip_edge(bool trip_high) { return trip_high ? GPIO_IRQ_EDGE_RISE : GPIO_IRQ_EDGE_FALL; }

static void set_up_inputs(void) {
    for (uint8_t lane = 0; lane < PT_MAX_LANES; lane++) {
        uint pin = PIN_LANE_FIRST + lane;
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin); /* the board has 10k pull-ups too */
        gpio_set_input_hysteresis_enabled(pin, true);
        /* clear any edge set before (the polarity or lane count may have changed) */
        gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
        gpio_set_irq_enabled_with_callback(pin, lane < cfg.num_lanes ? trip_edge(cfg.lane_trip_high) : 0,
                                           lane < cfg.num_lanes, on_edge);
    }
    gpio_init(PIN_GATE);
    gpio_set_dir(PIN_GATE, GPIO_IN);
    gpio_pull_up(PIN_GATE);
    gpio_set_irq_enabled(PIN_GATE, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled_with_callback(PIN_GATE, trip_edge(cfg.gate_trip_high), true, on_edge);
    gpio_init(PIN_RESET);
    gpio_set_dir(PIN_RESET, GPIO_IN);
    gpio_pull_up(PIN_RESET);
    gpio_set_irq_enabled_with_callback(PIN_RESET, GPIO_IRQ_EDGE_FALL, true, on_edge);
}

static pt_inputs read_inputs(void) {
    uint32_t all = gpio_get_all();
    pt_inputs in = {
        .gate_open = (((all >> PIN_GATE) & 1u) != 0) == cfg.gate_trip_high,
        .lane_raw = (uint8_t)((all >> PIN_LANE_FIRST) & 0xFFu),
    };
    return in;
}

/* ---------------------------------------------------------------- Status light */
static void led(bool r, bool g, bool b) {
    gpio_put(PIN_LED_R, r);
    gpio_put(PIN_LED_G, g);
    gpio_put(PIN_LED_B, b);
}

static void set_up_led(void) {
    const uint pins[3] = {PIN_LED_R, PIN_LED_G, PIN_LED_B};
    for (int i = 0; i < 3; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
    }
}

/* Waiting for a reset: amber blink. Ready: green. Racing: blue. Finished: red. Test: white */
static void show_status(const pt_timer *t, uint64_t now_us) {
    bool blink = (now_us / 500000) % 2;
    switch (t->mode) {
    case PT_INIT: led(blink, blink, false); break;
    case PT_READY: led(false, true, false); break;
    case PT_RACING: led(false, false, true); break;
    case PT_FINISH: led(true, false, false); break;
    case PT_TEST: led(true, true, true); break;
    }
}

/* ---------------------------------------------------------------- USB serial */
static void write_usb(const char *text, void *ctx) {
    (void)ctx;
    fputs(text, stdout);
}

/* ---------------------------------------------------------------- Lane displays */
static uint8_t displays;

static void show_lanes(const pt_timer *t, uint64_t now_us) {
    if (!displays) return;
    static uint64_t last;
    if (now_us - last < 50000) return; /* 20 times a second is plenty */
    last = now_us;
    char text[8];
    for (uint8_t lane = 0; lane < t->cfg.num_lanes; lane++) {
        if (!(displays & (1u << lane))) continue;
        uint8_t bit = (uint8_t)(1u << lane);
        if (t->mode == PT_TEST) {
            pt_inputs in = read_inputs();
            bool tripped = (((in.lane_raw >> lane) & 1u) != 0) == t->cfg.lane_trip_high;
            display_text(lane, tripped ? "8888" : "----");
        } else if (t->mask & bit) {
            display_text(lane, "");
        } else if (t->mode == PT_RACING && !(t->finished & bit)) {
            /* running clock while the car is on the track */
            pt_format_display((uint32_t)(now_us - t->start_us), true, text, sizeof text);
            display_text(lane, text);
        } else if ((t->mode == PT_RACING || t->mode == PT_FINISH) && (t->finished & bit)) {
            /* the time, then (once the race is over) the place, swapping every 3 seconds */
            if (t->mode == PT_FINISH && t->place[lane] && (now_us / 3000000) % 2) {
                snprintf(text, sizeof text, "P%u", (unsigned)t->place[lane]);
            } else {
                pt_format_display(t->lane_us[lane], true, text, sizeof text);
            }
            display_text(lane, text);
        } else {
            display_text(lane, "----");
        }
    }
}

/* ---------------------------------------------------------------- Main loop */
int main(void) {
    stdio_usb_init();
    stdio_set_translate_crlf(&stdio_usb, false); /* the protocol sends its own \r\n */
    cfg = settings_load();
    set_up_led();
    set_up_inputs();
    displays = display_init(cfg.num_lanes, cfg.brightness);
    for (uint8_t lane = 0; lane < cfg.num_lanes; lane++)
        if (displays & (1u << lane)) display_text(lane, "8.8.8.8.");

    /* Give the computer a moment to open the port, so it sees the power-on message */
    for (int i = 0; i < 20 && !stdio_usb_connected(); i++) sleep_ms(100);

    static pt_timer timer;
    pt_init(&timer, &cfg, write_usb, NULL);
    /* Holding the reset button while powering on starts sensor test mode */
    if (!gpio_get(PIN_RESET)) pt_enter_test(&timer);

    uint64_t last_test_report = 0;
    while (true) {
        uint64_t now = time_us_64();
        pt_inputs in = read_inputs();

        while (queue_tail != queue_head) {
            event e = {queue[queue_tail].kind, queue[queue_tail].at_us};
            queue_tail = (queue_tail + 1) % QUEUE_SIZE;
            if (e.kind < PT_MAX_LANES) {
                pt_lane_tripped(&timer, e.kind, e.at_us);
            } else if (e.kind == EVENT_GATE) {
                pt_gate_tripped(&timer, e.at_us);
            } else if (e.kind == EVENT_RESET) {
                if (timer.mode == PT_TEST) {
                    /* leaving test mode: start as if just powered on */
                    pt_init(&timer, &cfg, write_usb, NULL);
                } else if (timer.mode == PT_RACING) {
                    pt_force_end(&timer);
                } else {
                    pt_reset(&timer, &in);
                }
            }
        }

        int c;
        while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) pt_receive(&timer, (char)c, &in);

        if (timer.config_changed) {
            timer.config_changed = false;
            bool inputs_changed = timer.cfg.num_lanes != cfg.num_lanes ||
                                  timer.cfg.lane_trip_high != cfg.lane_trip_high ||
                                  timer.cfg.gate_trip_high != cfg.gate_trip_high;
            bool brightness_changed = timer.cfg.brightness != cfg.brightness;
            cfg = timer.cfg;
            if (inputs_changed) set_up_inputs();
            if (brightness_changed) display_brightness(displays, cfg.brightness);
        }
        if (timer.save_requested) {
            timer.save_requested = false;
            settings_save(&timer.cfg);
            displays = display_init(timer.cfg.num_lanes, timer.cfg.brightness);
        }

        if (timer.mode == PT_TEST && now - last_test_report > 250000) {
            last_test_report = now;
            pt_test_report(&timer, &in);
        }
        show_status(&timer, now);
        show_lanes(&timer, now);
        stdio_flush();
        tight_loop_contents();
    }
}
