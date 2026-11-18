/*
 * pinewood-time: linking finish gates, so tracks side by side race as one heat.
 *
 * Each gate has two link sockets: LINK IN (towards the main gate) and LINK OUT (towards the next). Gates are
 * chained IN to OUT: the main gate (the one with nothing on its LINK IN, plugged into the computer) and up to
 * two more. Each gate times its own lanes from its own start gate, on its own clock, and sends the times
 * on; the main gate adds them after its own lanes and reports one heat to the race software.
 *
 * Plain C with no hardware, like timer_core.c: main.c feeds it each character from the two serial links
 * and calls link_poll often; it writes lines back through the send functions. Tested in test/test_link.c.
 *
 * Messages, one per line, each ending "*" and a 2-digit hex XOR of the characters before it:
 *   down (towards the end of the chain):  H <position> <first lane>   hello, 4 times a second, from the main gate
 *                                         A <mask>                    get ready; mask = lanes left out (hex, all lanes)
 *                                         E                           the race was ended early
 *   up (towards the main gate):           L <lanes>                   lanes from this gate to the end of the chain
 *                                         Y <1|0>                     ready (1), or not: a gate open, or no answer
 *                                         S                           a gate's race started
 *                                         T <first lane> <us|-> ...   a gate's results, from its first lane on
 * Lanes are counted from 0 across every gate. A gate in the middle answers for itself and the gates after it.
 */
#ifndef PT_LINK_H
#define PT_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "timer_core.h"

#define LINK_MAX_GATES 3      /* the main gate and two more */
#define LINK_HELLO_US 250000  /* how often the main gate says hello */
#define LINK_LOST_US 1000000  /* no hello (or answer) for this long: that side is unplugged */
#define LINK_ARM_WAIT_US 300000 /* a gate waits this long for the next one to say it's ready */

typedef void (*link_send_fn)(const char *line, void *ctx);

typedef struct {
    pt_timer *t;
    link_send_fn send_up, send_down; /* a whole line, with its newline */
    void *ctx;

    /* towards the main gate: a hello heard recently means this gate is a secondary */
    bool upstream;
    uint64_t up_seen;
    uint8_t position;        /* 1 = the main gate */
    uint8_t first_lane;      /* this gate's lane 1, counted across every gate from 0 */

    /* towards the end of the chain */
    bool downstream;
    uint64_t down_seen;
    uint8_t down_lanes;      /* lanes on the gates after this one */

    /* getting ready: waiting for the next gate's answer */
    bool arm_wait;
    bool arm_self_ok;
    uint64_t arm_deadline;

    uint64_t last_hello;

    char up_line[64], down_line[64];
    uint8_t up_len, down_len;
} pt_link;

void link_init(pt_link *l, pt_timer *t, link_send_fn send_up, link_send_fn send_down, void *ctx);
/* A character from LINK IN (from the main gate's side), or from LINK OUT (from the next gate) */
void link_from_up(pt_link *l, char c, uint64_t now_us, const pt_inputs *in);
void link_from_down(pt_link *l, char c, uint64_t now_us, const pt_inputs *in);
/* Call often: hellos, timeouts, and passing on what the timer did */
void link_poll(pt_link *l, uint64_t now_us);
/* The main gate (nothing on LINK IN), as opposed to a secondary */
bool link_is_main(const pt_link *l);

/* "A 1F" -> "A 1F*0D\n": a message with its check, ready to send */
void link_frame(const char *msg, char *out, int size);

#endif
