#include "link.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void link_frame(const char *msg, char *out, int size) {
    uint8_t x = 0;
    for (const char *p = msg; *p; p++) x ^= (uint8_t)*p;
    snprintf(out, (size_t)size, "%s*%02X\n", msg, (unsigned)x);
}

static void send(pt_link *l, bool up, const char *msg) {
    link_send_fn fn = up ? l->send_up : l->send_down;
    if (!fn) return;
    char line[72];
    link_frame(msg, line, sizeof line);
    fn(line, l->ctx);
}

/* Checks a received line's "*XX" and cuts it off. False if it's damaged */
static bool unframe(char *line) {
    char *star = strrchr(line, '*');
    if (!star || strlen(star) != 3) return false;
    uint8_t x = 0;
    for (char *p = line; p < star; p++) x ^= (uint8_t)*p;
    char *end;
    long want = strtol(star + 1, &end, 16);
    if (*end || want != x) return false;
    *star = 0;
    return true;
}

void link_init(pt_link *l, pt_timer *t, link_send_fn send_up, link_send_fn send_down, void *ctx) {
    memset(l, 0, sizeof *l);
    l->t = t;
    l->send_up = send_up;
    l->send_down = send_down;
    l->ctx = ctx;
    l->position = 1;
}

bool link_is_main(const pt_link *l) { return !l->upstream; }

static uint8_t own_lanes(const pt_link *l) { return l->t->cfg.num_lanes; }

/* A secondary tells the main gate's side how many lanes it and the gates after it have */
static void answer_lanes(pt_link *l) {
    char msg[16];
    snprintf(msg, sizeof msg, "L %u", (unsigned)(own_lanes(l) + l->down_lanes));
    send(l, true, msg);
}

static void ready_answer(pt_link *l, bool ok) {
    char msg[8];
    snprintf(msg, sizeof msg, "Y %d", ok ? 1 : 0);
    send(l, true, msg);
}

/* The next gate answered "get ready", or didn't in time */
static void arm_done(pt_link *l, bool down_ok) {
    l->arm_wait = false;
    if (link_is_main(l)) pt_chain_ready(l->t, down_ok);
    else ready_answer(l, l->arm_self_ok && down_ok);
}

/* ---------------------------------------------------------------- From the main gate's side */
static void up_message(pt_link *l, char *msg, uint64_t now_us, const pt_inputs *in) {
    char *arg = msg[0] && msg[1] == ' ' ? msg + 2 : msg + 1;
    switch (msg[0]) {
    case 'H': { /* hello: this gate's place in the chain */
        unsigned pos, first;
        if (sscanf(arg, "%u %u", &pos, &first) != 2 || pos < 2) return;
        if (pos > LINK_MAX_GATES || first + own_lanes(l) > PT_MAX_LANES) {
            /* one gate too many: stay out of the chain (and don't pass the hello on) */
            l->upstream = false;
            return;
        }
        if (!l->upstream) {
            /* just joined a chain: the main gate decides the lanes from now on */
            l->upstream = true;
            pt_set_remote_lanes(l->t, 0);
        }
        l->up_seen = now_us;
        l->position = (uint8_t)pos;
        l->first_lane = (uint8_t)first;
        answer_lanes(l);
        char next[24];
        snprintf(next, sizeof next, "H %u %u", pos + 1, first + own_lanes(l));
        send(l, false, next);
        break;
    }
    case 'A': { /* get ready, with every lane's mask */
        if (!l->upstream) return;
        unsigned long mask = strtoul(arg, NULL, 16);
        l->arm_self_ok = pt_link_arm(l->t, (uint16_t)(mask >> l->first_lane), in);
        if (l->downstream) {
            send(l, false, msg); /* the same mask on down the chain */
            l->arm_wait = true;
            l->arm_deadline = now_us + LINK_ARM_WAIT_US;
        } else {
            ready_answer(l, l->arm_self_ok);
        }
        break;
    }
    case 'E': /* ended early: send what we have */
        if (!l->upstream) return;
        pt_force_end(l->t);
        if (l->downstream) send(l, false, "E");
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------- From the next gate */
static void down_message(pt_link *l, char *msg, uint64_t now_us) {
    char *arg = msg[0] && msg[1] == ' ' ? msg + 2 : msg + 1;
    switch (msg[0]) {
    case 'L': {
        unsigned n = (unsigned)atoi(arg);
        if (n > PT_MAX_LANES) return;
        l->downstream = true;
        l->down_seen = now_us;
        l->down_lanes = (uint8_t)n;
        if (link_is_main(l)) pt_set_remote_lanes(l->t, l->down_lanes);
        break;
    }
    case 'Y':
        if (l->arm_wait) arm_done(l, atoi(arg) == 1);
        break;
    case 'S':
        if (link_is_main(l)) pt_remote_started(l->t);
        else send(l, true, msg);
        break;
    case 'T':
        if (!link_is_main(l)) {
            send(l, true, msg);
            break;
        }
        {
            /* results before the start message (if that was lost) still start the race */
            pt_remote_started(l->t);
            /* "T 3 2501234 - 2499876": results from lane 4 (counted from 0) on */
            char *p = arg;
            char *end;
            long lane = strtol(p, &end, 10);
            if (end == p || lane < 0) break;
            p = end;
            while (*p == ' ') {
                p++;
                if (*p == '-') {
                    pt_remote_result(l->t, (uint8_t)lane, false, 0);
                    p++;
                } else {
                    unsigned long us = strtoul(p, &end, 10);
                    if (end == p) break;
                    pt_remote_result(l->t, (uint8_t)lane, true, (uint32_t)us);
                    p = end;
                }
                lane++;
            }
        }
        break;
    default:
        break;
    }
}

static void feed(pt_link *l, bool from_up, char c, uint64_t now_us, const pt_inputs *in) {
    char *line = from_up ? l->up_line : l->down_line;
    uint8_t *len = from_up ? &l->up_len : &l->down_len;
    if (c == '\r') return;
    if (c != '\n') {
        if (*len < sizeof l->up_line - 1) line[(*len)++] = c;
        else *len = sizeof l->up_line; /* too long: drop it at the newline */
        return;
    }
    bool ok = *len < sizeof l->up_line;
    if (ok) line[*len] = 0;
    *len = 0;
    if (!ok || !unframe(line)) return;
    if (from_up) up_message(l, line, now_us, in);
    else down_message(l, line, now_us);
}

void link_from_up(pt_link *l, char c, uint64_t now_us, const pt_inputs *in) { feed(l, true, c, now_us, in); }
void link_from_down(pt_link *l, char c, uint64_t now_us, const pt_inputs *in) { feed(l, false, c, now_us, in); }

/* A secondary's results go towards the main gate, from its first lane */
static void send_results_up(pt_link *l) {
    char msg[64];
    int n = snprintf(msg, sizeof msg, "T %u", (unsigned)l->first_lane);
    for (uint8_t lane = 0; lane < own_lanes(l) && n < (int)sizeof msg - 12; lane++) {
        if (pt_lane_has_time(l->t, lane)) n += snprintf(msg + n, sizeof msg - (size_t)n, " %lu", (unsigned long)l->t->lane_us[lane]);
        else n += snprintf(msg + n, sizeof msg - (size_t)n, " -");
    }
    send(l, true, msg);
}

void link_poll(pt_link *l, uint64_t now_us) {
    /* lost touch with either side */
    if (l->upstream && now_us - l->up_seen > LINK_LOST_US) {
        l->upstream = false; /* unplugged from the chain: a timer on its own again */
        l->position = 1;
        l->first_lane = 0;
    }
    if (l->downstream && now_us - l->down_seen > LINK_LOST_US) {
        l->downstream = false;
        l->down_lanes = 0;
        if (link_is_main(l)) pt_set_remote_lanes(l->t, 0);
    }
    if (l->arm_wait && now_us >= l->arm_deadline) arm_done(l, false); /* the next gate didn't answer */

    /* the main gate says hello down the chain */
    if (link_is_main(l) && now_us - l->last_hello >= LINK_HELLO_US) {
        l->last_hello = now_us;
        char msg[24];
        snprintf(msg, sizeof msg, "H 2 %u", (unsigned)own_lanes(l));
        send(l, false, msg);
    }
    if (link_is_main(l)) pt_set_remote_lanes(l->t, l->down_lanes); /* applied between races */

    /* what the timer did */
    uint8_t ev = l->t->events;
    l->t->events = 0;
    if (link_is_main(l)) {
        if (ev & PT_EV_ARM) {
            if (l->downstream) {
                char msg[16];
                snprintf(msg, sizeof msg, "A %X", (unsigned)l->t->mask);
                send(l, false, msg);
                l->arm_wait = true;
                l->arm_deadline = now_us + LINK_ARM_WAIT_US + 200000; /* longer than the gates after it wait */
            } else {
                pt_chain_ready(l->t, false); /* nothing to ask */
            }
        }
        if (ev & PT_EV_END) send(l, false, "E");
        pt_tick(l->t, now_us);
    } else {
        if (ev & PT_EV_STARTED) send(l, true, "S");
        if (ev & PT_EV_FINISHED) send_results_up(l);
    }
}
