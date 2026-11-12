/*
 * pinewood-time: the race timer's logic, with no hardware in it.
 *
 * The Pico code (main.c) feeds it what happens (a character from the computer, the start gate opening, a
 * lane's beam breaking, each with a microsecond timestamp) and it answers with text to send back. Being
 * plain C, it's unit tested on a computer (test/test_core.c).
 *
 * It speaks the serial protocol of the PDT open-source pinewood derby timer, so race software that
 * supports a PDT timer (Pinewood Control, DerbyNet, Grand Prix Race Manager, ...) works with it. This is
 * a fresh implementation written from the protocol's description, not a copy of the PDT firmware.
 * See docs/protocol.md.
 */
#ifndef PT_TIMER_CORE_H
#define PT_TIMER_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define PT_MAX_LANES 8
#define PT_VERSION "1.0.0"
/* The PDT protocol version this timer implements, reported for the V command */
#define PT_PROTOCOL_VERSION "3.40"

typedef enum {
    PT_INIT = 1,   /* just powered on: waiting for a reset (R, or the reset button) */
    PT_READY = 2,  /* waiting for the start gate */
    PT_RACING = 3, /* timing, until every lane in use has finished */
    PT_FINISH = 4, /* results sent; they can be sent again with Q */
    PT_TEST = 5,   /* sensor test: reports what each sensor sees */
} pt_mode;

typedef struct {
    uint8_t num_lanes;      /* lanes on the track, 1 to 8 */
    bool lane_trip_high;    /* a lane triggers when its input goes high (beam broken, this board's sensors) */
    bool gate_trip_high;    /* the race starts when the gate input goes high */
    uint8_t brightness;     /* lane displays, 0 to 15 */
} pt_config;

/* Where the timer's replies go: text to the computer */
typedef void (*pt_write_fn)(const char *text, void *ctx);

typedef struct {
    pt_config cfg;
    pt_mode mode;
    uint8_t mask;            /* lanes left out of this race (bit 0 = lane 1) */
    uint8_t finished;        /* lanes that have crossed the line */
    uint64_t start_us;
    uint32_t lane_us[PT_MAX_LANES]; /* time from the start, in microseconds */
    uint8_t place[PT_MAX_LANES];    /* 1 = first; 0 = no finish */
    bool debug;
    bool config_changed;     /* set by a # command; main.c saves it to flash on #save */
    bool save_requested;
    /* a command waiting for its next character (M takes a lane number) */
    char pending;
    /* a # setting command being typed, up to its newline */
    char line[40];
    uint8_t line_len;
    bool in_line;
    pt_write_fn write;
    void *ctx;
} pt_timer;

/* What the hardware tells the core about, when it asks */
typedef struct {
    bool gate_open;          /* the start gate is in its tripped (open) position */
    uint8_t lane_raw;        /* each lane input's level, bit 0 = lane 1 (for the T command and test mode) */
} pt_inputs;

pt_config pt_default_config(void);
/* Starts up: sends P (power on) and waits in PT_INIT for a reset */
void pt_init(pt_timer *t, const pt_config *cfg, pt_write_fn write, void *ctx);
/* A reset (R or the button): ready for the next race, unless the gate is still open (then O) */
void pt_reset(pt_timer *t, const pt_inputs *in);
/* One character from the computer */
void pt_receive(pt_timer *t, char c, const pt_inputs *in);
/* The start gate opened at this time */
void pt_gate_tripped(pt_timer *t, uint64_t now_us);
/* A lane's beam broke at this time (lane 0 = lane 1). Only the first break in a race counts */
void pt_lane_tripped(pt_timer *t, uint8_t lane, uint64_t now_us);
/* Ends the race now (F, or the reset button while racing): lanes still out get no time */
void pt_force_end(pt_timer *t);
/* Enters sensor test mode (reset button held at power on) */
void pt_enter_test(pt_timer *t);
/* In test mode, a report of what the sensors see, e.g. "test lanes=1.0. gate=closed" */
void pt_test_report(pt_timer *t, const pt_inputs *in);

/* "1 - 2.3456": a lane's result line, as the protocol sends it (no time is 99.9990) */
void pt_format_result(const pt_timer *t, uint8_t lane, char *out, int size);
/* "2.345" or "12.34": a time for a 4-digit display; "----" for none */
void pt_format_display(uint32_t us, bool has_time, char *out, int size);

#endif
