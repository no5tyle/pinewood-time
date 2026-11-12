#include "timer_core.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Replies, each a single character on its own line */
#define MSG_POWER 'P' /* powered on */
#define MSG_READY 'K' /* ready for a race */
#define MSG_ACK '.'   /* command done */
#define MSG_START 'B' /* race started */
#define MSG_GATE_OPEN 'O'

/* "No time": a lane masked out, or one that didn't finish before the race was ended */
#define NULL_TIME_TEXT "99.9990"

static void say(pt_timer *t, const char *text) {
    if (t->write) t->write(text, t->ctx);
}

static void say_line(pt_timer *t, const char *text) {
    say(t, text);
    say(t, "\r\n");
}

static void say_char(pt_timer *t, char c) {
    char s[2] = {c, 0};
    say_line(t, s);
}

static void debug(pt_timer *t, const char *text) {
    if (!t->debug) return;
    say(t, "dbg: ");
    say_line(t, text);
}

static uint8_t all_lanes(const pt_timer *t) {
    return (uint8_t)((1u << t->cfg.num_lanes) - 1u);
}

pt_config pt_default_config(void) {
    pt_config c = {.num_lanes = 4, .lane_trip_high = true, .gate_trip_high = true, .brightness = 6,
                   .button_next = false, .button_key = 0x28 /* enter */};
    return c;
}

void pt_init(pt_timer *t, const pt_config *cfg, pt_write_fn write, void *ctx) {
    memset(t, 0, sizeof *t);
    t->cfg = *cfg;
    if (t->cfg.num_lanes < 1 || t->cfg.num_lanes > PT_MAX_LANES) t->cfg.num_lanes = 4;
    t->write = write;
    t->ctx = ctx;
    t->mode = PT_INIT;
    say_char(t, MSG_POWER);
}

static void clear_race(pt_timer *t) {
    t->finished = 0;
    t->start_us = 0;
    memset(t->lane_us, 0, sizeof t->lane_us);
    memset(t->place, 0, sizeof t->place);
}

void pt_reset(pt_timer *t, const pt_inputs *in) {
    if (t->mode == PT_TEST) return;
    if (in && in->gate_open) {
        /* Can't arm with the gate down: the race would start straight away */
        say_char(t, MSG_GATE_OPEN);
        return;
    }
    clear_race(t);
    t->mode = PT_READY;
    say_char(t, MSG_READY);
}

/* Places from the times: ties share a place, and the next place follows on (1, 2, 2, 3) */
static void work_out_places(pt_timer *t) {
    memset(t->place, 0, sizeof t->place);
    uint8_t place = 0;
    uint32_t last = 0;
    uint8_t placed = 0;
    for (uint8_t n = 0; n < t->cfg.num_lanes; n++) {
        /* the fastest lane not yet placed */
        int best = -1;
        for (uint8_t lane = 0; lane < t->cfg.num_lanes; lane++) {
            uint8_t bit = (uint8_t)(1u << lane);
            if (!(t->finished & bit) || (t->mask & bit) || (placed & bit)) continue;
            if (best < 0 || t->lane_us[lane] < t->lane_us[best]) best = lane;
        }
        if (best < 0) break;
        if (place == 0 || t->lane_us[best] > last) place++;
        last = t->lane_us[best];
        t->place[best] = place;
        placed |= (uint8_t)(1u << best);
    }
}

static bool lane_has_time(const pt_timer *t, uint8_t lane) {
    uint8_t bit = (uint8_t)(1u << lane);
    return (t->finished & bit) && !(t->mask & bit);
}

void pt_format_result(const pt_timer *t, uint8_t lane, char *out, int size) {
    if (!lane_has_time(t, lane)) {
        snprintf(out, (size_t)size, "%u - %s", (unsigned)(lane + 1), NULL_TIME_TEXT);
        return;
    }
    /* Seconds to 4 places, rounded to the nearest 100 microseconds */
    uint32_t tenths_ms = (t->lane_us[lane] + 50u) / 100u;
    if (tenths_ms >= 1000000u) { /* 100 s or more: no time */
        snprintf(out, (size_t)size, "%u - %s", (unsigned)(lane + 1), NULL_TIME_TEXT);
        return;
    }
    snprintf(out, (size_t)size, "%u - %lu.%04lu", (unsigned)(lane + 1), (unsigned long)(tenths_ms / 10000u),
             (unsigned long)(tenths_ms % 10000u));
}

void pt_format_display(uint32_t us, bool has_time, char *out, int size) {
    if (!has_time) {
        snprintf(out, (size_t)size, "----");
        return;
    }
    uint32_t ms = (us + 500u) / 1000u;
    if (ms < 10000u)
        snprintf(out, (size_t)size, "%lu.%03lu", (unsigned long)(ms / 1000u), (unsigned long)(ms % 1000u));
    else if (ms < 100000u)
        snprintf(out, (size_t)size, "%lu.%02lu", (unsigned long)(ms / 1000u), (unsigned long)((ms % 1000u) / 10u));
    else
        snprintf(out, (size_t)size, "----");
}

static void send_results(pt_timer *t) {
    char line[24];
    for (uint8_t lane = 0; lane < t->cfg.num_lanes; lane++) {
        pt_format_result(t, lane, line, sizeof line);
        say_line(t, line);
    }
}

static void finish(pt_timer *t) {
    work_out_places(t);
    t->mode = PT_FINISH;
    send_results(t);
}

void pt_gate_tripped(pt_timer *t, uint64_t now_us) {
    if (t->mode != PT_READY) return;
    clear_race(t);
    t->start_us = now_us;
    t->mode = PT_RACING;
    say_char(t, MSG_START);
    /* Lanes left out of this race count as already in, so the race doesn't wait for them */
    if ((t->mask & all_lanes(t)) == all_lanes(t)) finish(t);
}

void pt_lane_tripped(pt_timer *t, uint8_t lane, uint64_t now_us) {
    if (t->mode != PT_RACING || lane >= t->cfg.num_lanes) return;
    uint8_t bit = (uint8_t)(1u << lane);
    if ((t->finished & bit) || (t->mask & bit)) return;
    uint64_t elapsed = now_us > t->start_us ? now_us - t->start_us : 0;
    if (elapsed > 0xFFFFFFFFu) elapsed = 0xFFFFFFFFu;
    t->lane_us[lane] = (uint32_t)elapsed;
    t->finished |= bit;
    if (t->debug) {
        char s[40];
        snprintf(s, sizeof s, "lane %u in at %lu us", (unsigned)(lane + 1), (unsigned long)elapsed);
        debug(t, s);
    }
    if (((t->finished | t->mask) & all_lanes(t)) == all_lanes(t)) finish(t);
}

void pt_force_end(pt_timer *t) {
    if (t->mode != PT_RACING) return;
    finish(t);
}

void pt_enter_test(pt_timer *t) {
    t->mode = PT_TEST;
    say_line(t, "TEST MODE");
}

void pt_test_report(pt_timer *t, const pt_inputs *in) {
    char s[48];
    int n = snprintf(s, sizeof s, "test lanes=");
    for (uint8_t lane = 0; lane < t->cfg.num_lanes && n < (int)sizeof s - 1; lane++) {
        bool high = (in->lane_raw >> lane) & 1u;
        bool tripped = high == t->cfg.lane_trip_high;
        s[n++] = tripped ? '1' : '.';
    }
    s[n] = 0;
    snprintf(s + n, sizeof s - (size_t)n, " gate=%s", in->gate_open ? "open" : "closed");
    say_line(t, s);
}

/* ---------------------------------------------------------------- Keyboard keys (USB HID usage codes) */
static const struct {
    const char *name;
    uint8_t code;
} KEYS[] = {
    {"none", 0x00},     {"enter", 0x28},    {"escape", 0x29}, {"tab", 0x2B},   {"space", 0x2C},
    {"pageup", 0x4B},   {"pagedown", 0x4E}, {"right", 0x4F},  {"left", 0x50},  {"down", 0x51},
    {"up", 0x52},       {"home", 0x4A},     {"end", 0x4D},
};

bool pt_key_code(const char *name, uint8_t *code) {
    for (size_t i = 0; i < sizeof KEYS / sizeof KEYS[0]; i++) {
        if (strcmp(name, KEYS[i].name) == 0) {
            *code = KEYS[i].code;
            return true;
        }
    }
    if (name[0] && !name[1]) { /* one letter or digit */
        char c = name[0];
        if (c >= 'a' && c <= 'z') { *code = (uint8_t)(0x04 + (c - 'a')); return true; }
        if (c >= '1' && c <= '9') { *code = (uint8_t)(0x1E + (c - '1')); return true; }
        if (c == '0') { *code = 0x27; return true; }
    }
    if (name[0] == 'f' && name[1]) { /* f1 to f12 */
        int n = atoi(name + 1);
        char check[4];
        snprintf(check, sizeof check, "%d", n);
        if (n >= 1 && n <= 12 && strcmp(check, name + 1) == 0) { *code = (uint8_t)(0x3A + n - 1); return true; }
    }
    return false;
}

const char *pt_key_name(uint8_t code, char *buf, int size) {
    for (size_t i = 0; i < sizeof KEYS / sizeof KEYS[0]; i++)
        if (KEYS[i].code == code) return KEYS[i].name;
    if (code >= 0x04 && code <= 0x1D) snprintf(buf, (size_t)size, "%c", 'a' + (code - 0x04));
    else if (code >= 0x1E && code <= 0x26) snprintf(buf, (size_t)size, "%c", '1' + (code - 0x1E));
    else if (code == 0x27) snprintf(buf, (size_t)size, "0");
    else if (code >= 0x3A && code <= 0x45) snprintf(buf, (size_t)size, "f%d", code - 0x3A + 1);
    else snprintf(buf, (size_t)size, "0x%02x", code);
    return buf;
}

/* ---------------------------------------------------------------- The reset button */
pt_button_result pt_button(pt_timer *t, const pt_inputs *in, bool host_listening) {
    switch (t->mode) {
    case PT_TEST:
        return PT_BUTTON_LEAVE_TEST;
    case PT_RACING:
        /* a car stuck on the track: end the race whichever mode the button is in */
        pt_force_end(t);
        return PT_BUTTON_NOTHING;
    default:
        if (t->cfg.button_next && host_listening) {
            say_line(t, "#next");
            t->key_to_press = t->cfg.button_key;
        } else {
            pt_reset(t, in);
        }
        return PT_BUTTON_NOTHING;
    }
}

static void send_info(pt_timer *t) {
    char s[64];
    say_line(t, "-----------------------------");
    say_line(t, " pinewood-time race timer");
    snprintf(s, sizeof s, " firmware %s, PDT protocol %s", PT_VERSION, PT_PROTOCOL_VERSION);
    say_line(t, s);
    snprintf(s, sizeof s, " lanes %u", (unsigned)t->cfg.num_lanes);
    say_line(t, s);
    snprintf(s, sizeof s, " lane trip %s, gate trip %s", t->cfg.lane_trip_high ? "high" : "low",
             t->cfg.gate_trip_high ? "high" : "low");
    say_line(t, s);
    snprintf(s, sizeof s, " display brightness %u", (unsigned)t->cfg.brightness);
    say_line(t, s);
    char key[8];
    if (t->cfg.button_next)
        snprintf(s, sizeof s, " button next, key %s", pt_key_name(t->cfg.button_key, key, sizeof key));
    else
        snprintf(s, sizeof s, " button reset");
    say_line(t, s);
    say_line(t, "-----------------------------");
}

/* "#lanes 6", "#lane-trip low", "#gate-trip high", "#brightness 8", "#button next", "#button-key space",
 * "#save", "#info" */
static void run_setting(pt_timer *t, char *line) {
    for (char *p = line; *p; p++) *p = (char)tolower((unsigned char)*p);
    while (*line == ' ') line++;
    char *arg = strchr(line, ' ');
    if (arg) {
        *arg++ = 0;
        while (*arg == ' ') arg++;
    }
    bool ok = true;
    if (strcmp(line, "lanes") == 0 && arg) {
        int n = atoi(arg);
        if (n >= 1 && n <= PT_MAX_LANES) t->cfg.num_lanes = (uint8_t)n;
        else ok = false;
    } else if (strcmp(line, "lane-trip") == 0 && arg && (strcmp(arg, "high") == 0 || strcmp(arg, "low") == 0)) {
        t->cfg.lane_trip_high = strcmp(arg, "high") == 0;
    } else if (strcmp(line, "gate-trip") == 0 && arg && (strcmp(arg, "high") == 0 || strcmp(arg, "low") == 0)) {
        t->cfg.gate_trip_high = strcmp(arg, "high") == 0;
    } else if (strcmp(line, "brightness") == 0 && arg) {
        int n = atoi(arg);
        if (n >= 0 && n <= 15) t->cfg.brightness = (uint8_t)n;
        else ok = false;
    } else if (strcmp(line, "button") == 0 && arg && (strcmp(arg, "reset") == 0 || strcmp(arg, "next") == 0)) {
        t->cfg.button_next = strcmp(arg, "next") == 0;
    } else if (strcmp(line, "button-key") == 0 && arg) {
        uint8_t code;
        if (pt_key_code(arg, &code)) t->cfg.button_key = code;
        else ok = false;
    } else if (strcmp(line, "save") == 0) {
        t->save_requested = true;
        say_line(t, "saved");
        return;
    } else if (strcmp(line, "info") == 0) {
        send_info(t);
        return;
    } else {
        ok = false;
    }
    if (ok) {
        t->config_changed = true;
        say_char(t, MSG_ACK);
    } else {
        say_line(t, "?");
    }
}

void pt_receive(pt_timer *t, char c, const pt_inputs *in) {
    if (t->in_line) {
        if (c == '\r' || c == '\n') {
            t->line[t->line_len] = 0;
            t->in_line = false;
            run_setting(t, t->line);
            t->line_len = 0;
        } else if (t->line_len < sizeof t->line - 1) {
            t->line[t->line_len++] = c;
        }
        return;
    }
    if (t->pending == 'M') {
        t->pending = 0;
        int lane = c - '0';
        if (lane >= 1 && lane <= t->cfg.num_lanes) t->mask |= (uint8_t)(1u << (lane - 1));
        say_char(t, MSG_ACK);
        return;
    }
    if (c == '\r' || c == '\n' || c == ' ') return;
    if (t->debug) {
        char s[24];
        snprintf(s, sizeof s, "ser rec = %d", (int)c);
        debug(t, s);
    }
    char s[48];
    switch (c) {
    case '#':
        t->in_line = true;
        t->line_len = 0;
        break;
    case 'R': /* reset: mid-race it ends the race (sending results); otherwise ready for the next */
        if (t->mode == PT_RACING) pt_force_end(t);
        else pt_reset(t, in);
        break;
    case 'F': /* force the race to end now */
        if (t->mode == PT_RACING) {
            say_char(t, MSG_ACK);
            pt_force_end(t);
        } else {
            say_char(t, MSG_ACK);
        }
        break;
    case 'Q': /* send the last results again */
        if (t->mode == PT_FINISH) send_results(t);
        else say_char(t, MSG_ACK);
        break;
    case 'G': /* is the gate open? */
        say_char(t, in && in->gate_open ? MSG_GATE_OPEN : MSG_ACK);
        break;
    case 'M': /* mask a lane: the lane number follows */
        t->pending = 'M';
        break;
    case 'U': /* unmask every lane */
        t->mask = 0;
        say_char(t, MSG_ACK);
        break;
    case 'S': /* start solenoid: this board has none, so just acknowledge */
        say_char(t, MSG_ACK);
        break;
    case 'V':
        say_line(t, "vert=" PT_PROTOCOL_VERSION);
        break;
    case 'N':
        snprintf(s, sizeof s, "numl=%u", (unsigned)t->cfg.num_lanes);
        say_line(t, s);
        break;
    case 'I':
        send_info(t);
        break;
    case 'D':
        t->debug = !t->debug;
        say_line(t, t->debug ? "dbg: on" : "dbg: off");
        break;
    case 'T': /* each lane input's level, lane 1 first */
        if (in) {
            int n = snprintf(s, sizeof s, "lanes ");
            for (uint8_t lane = 0; lane < t->cfg.num_lanes; lane++) s[n++] = ((in->lane_raw >> lane) & 1u) ? '1' : '0';
            s[n] = 0;
            say_line(t, s);
        }
        break;
    case 'X': /* no display multiplexer on this board */
        say_char(t, MSG_ACK);
        break;
    default:
        break;
    }
}
