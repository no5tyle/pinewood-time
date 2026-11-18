/* Unit tests for the timer core, run on a computer: make -C firmware/test */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/timer_core.h"

static char out[4096];

static void capture(const char *text, void *ctx) {
    (void)ctx;
    strncat(out, text, sizeof out - strlen(out) - 1);
}

static void clear(void) { out[0] = 0; }

static void send(pt_timer *t, const char *chars, const pt_inputs *in) {
    for (const char *c = chars; *c; c++) pt_receive(t, *c, in);
}

static int failures = 0;
#define EXPECT_STR(actual, expected)                                                                          \
    do {                                                                                                      \
        if (strcmp((actual), (expected)) != 0) {                                                              \
            printf("FAIL %s:%d\n  expected: \"%s\"\n  actual:   \"%s\"\n", __FILE__, __LINE__, (expected),     \
                   (actual));                                                                                 \
            failures++;                                                                                       \
        }                                                                                                     \
    } while (0)
#define EXPECT(cond)                                                                                          \
    do {                                                                                                      \
        if (!(cond)) {                                                                                        \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
            failures++;                                                                                       \
        }                                                                                                     \
    } while (0)

static pt_timer timer4(void) {
    pt_timer t;
    pt_config c = pt_default_config();
    c.num_lanes = 4;
    clear();
    pt_init(&t, &c, capture, NULL);
    return t;
}

static const pt_inputs closed = {.gates_open = 0, .lane_raw = 0};
static const pt_inputs open_gate = {.gates_open = 1, .lane_raw = 0};

static void test_power_on_and_reset(void) {
    pt_timer t = timer4();
    EXPECT_STR(out, "P\r\n");
    EXPECT(t.mode == PT_INIT);
    clear();
    send(&t, "R", &open_gate);
    EXPECT_STR(out, "O\r\n"); /* won't arm with the gate open */
    EXPECT(t.mode == PT_INIT);
    clear();
    send(&t, "R", &closed);
    EXPECT_STR(out, "K\r\n");
    EXPECT(t.mode == PT_READY);
}

static void test_a_race(void) {
    pt_timer t = timer4();
    send(&t, "R", &closed);
    clear();
    pt_gate_tripped(&t, 0, 1000000);
    EXPECT_STR(out, "B\r\n");
    clear();
    pt_lane_tripped(&t, 2, 1000000 + 2345650); /* lane 3 first */
    pt_lane_tripped(&t, 0, 1000000 + 2401234);
    pt_lane_tripped(&t, 0, 1000000 + 2900000); /* a second break on lane 1 doesn't count */
    pt_lane_tripped(&t, 3, 1000000 + 2401234); /* a tie with lane 1 */
    EXPECT_STR(out, ""); /* nothing until every lane is in */
    pt_lane_tripped(&t, 1, 1000000 + 3000049);
    EXPECT_STR(out, "1 - 2.4012\r\n2 - 3.0000\r\n3 - 2.3457\r\n4 - 2.4012\r\n");
    EXPECT(t.mode == PT_FINISH);
    EXPECT(t.place[2] == 1 && t.place[0] == 2 && t.place[3] == 2 && t.place[1] == 3);
    clear();
    send(&t, "Q", &closed); /* results again */
    EXPECT_STR(out, "1 - 2.4012\r\n2 - 3.0000\r\n3 - 2.3457\r\n4 - 2.4012\r\n");
}

static void test_masked_lanes_and_forced_end(void) {
    pt_timer t = timer4();
    send(&t, "R", &closed);
    clear();
    send(&t, "M2M4", &closed);
    EXPECT_STR(out, ".\r\n.\r\n");
    EXPECT(t.mask == 0x0A);
    clear();
    pt_gate_tripped(&t, 0, 0);
    pt_lane_tripped(&t, 1, 1500000); /* masked: ignored */
    pt_lane_tripped(&t, 0, 2000000);
    EXPECT_STR(out, "B\r\n"); /* still waiting for lane 3 */
    clear();
    send(&t, "F", &closed);
    EXPECT_STR(out, ".\r\n1 - 2.0000\r\n2 - 99.9990\r\n3 - 99.9990\r\n4 - 99.9990\r\n");
    clear();
    send(&t, "U", &closed);
    EXPECT(t.mask == 0);
    EXPECT_STR(out, ".\r\n");
}

static void test_masked_race_finishes_when_the_rest_are_in(void) {
    pt_timer t = timer4();
    send(&t, "RM3M4", &closed);
    clear();
    pt_gate_tripped(&t, 0, 0);
    pt_lane_tripped(&t, 0, 2100000);
    pt_lane_tripped(&t, 1, 2200000);
    EXPECT_STR(out, "B\r\n1 - 2.1000\r\n2 - 2.2000\r\n3 - 99.9990\r\n4 - 99.9990\r\n");
}

static void test_reset_mid_race_ends_it_then_rearms(void) {
    pt_timer t = timer4();
    send(&t, "R", &closed);
    pt_gate_tripped(&t, 0, 0);
    pt_lane_tripped(&t, 0, 2500000);
    clear();
    send(&t, "R", &closed);
    EXPECT_STR(out, "1 - 2.5000\r\n2 - 99.9990\r\n3 - 99.9990\r\n4 - 99.9990\r\n");
    EXPECT(t.mode == PT_FINISH);
    clear();
    send(&t, "R", &closed);
    EXPECT_STR(out, "K\r\n");
    EXPECT(t.mode == PT_READY);
}

static void test_gate_ignored_unless_ready(void) {
    pt_timer t = timer4();
    clear();
    pt_gate_tripped(&t, 0, 5); /* not reset yet */
    pt_lane_tripped(&t, 0, 10);
    EXPECT_STR(out, "");
    EXPECT(t.mode == PT_INIT);
}

static void test_queries(void) {
    pt_timer t = timer4();
    clear();
    send(&t, "V", &closed);
    EXPECT_STR(out, "vert=3.40\r\n");
    clear();
    send(&t, "N", &closed);
    EXPECT_STR(out, "numl=4\r\n");
    clear();
    send(&t, "G", &open_gate);
    EXPECT_STR(out, "O\r\n");
    clear();
    send(&t, "G\r\n", &closed); /* line endings after a command are ignored */
    EXPECT_STR(out, ".\r\n");
    clear();
    pt_inputs in = {.gates_open = 0, .lane_raw = 0x05};
    send(&t, "T", &in);
    EXPECT_STR(out, "lanes 1010\r\n");
}

static void test_settings(void) {
    pt_timer t = timer4();
    clear();
    send(&t, "#lanes 6\r\n", &closed);
    EXPECT_STR(out, ".\r\n");
    EXPECT(t.cfg.num_lanes == 6 && t.config_changed);
    clear();
    send(&t, "#lanes 13\n", &closed);
    EXPECT_STR(out, "?\r\n");
    EXPECT(t.cfg.num_lanes == 6);
    send(&t, "#lane-trip low\n#gate-trip low\n#brightness 12\n", &closed);
    EXPECT(!t.cfg.lane_trip_high && !t.cfg.gate_trip_high && t.cfg.brightness == 12);
    clear();
    send(&t, "#save\n", &closed);
    EXPECT(t.save_requested);
    EXPECT_STR(out, "saved\r\n");
    clear();
    send(&t, "N", &closed);
    EXPECT_STR(out, "numl=6\r\n");
}

static void test_formatting(void) {
    char s[16];
    pt_format_display(2345499, true, s, sizeof s);
    EXPECT_STR(s, "2.345");
    pt_format_display(2345500, true, s, sizeof s);
    EXPECT_STR(s, "2.346");
    pt_format_display(12345000, true, s, sizeof s);
    EXPECT_STR(s, "12.34");
    pt_format_display(0, false, s, sizeof s);
    EXPECT_STR(s, "----");
    pt_timer t = timer4();
    send(&t, "R", &closed);
    pt_gate_tripped(&t, 0, 0);
    pt_lane_tripped(&t, 0, 100000000); /* 100 s or more: no time */
    pt_format_result(&t, 0, s, sizeof s);
    EXPECT_STR(s, "1 - 99.9990");
}

static void test_mode_test(void) {
    pt_timer t = timer4();
    clear();
    pt_enter_test(&t);
    pt_inputs in = {.gates_open = 1, .lane_raw = 0x09};
    pt_test_report(&t, &in);
    EXPECT_STR(out, "TEST MODE\r\ntest lanes=1..1 gate=open\r\n");
    clear();
    send(&t, "R", &closed); /* no racing from test mode */
    EXPECT_STR(out, "");
}

static void test_button_reset_mode(void) {
    pt_timer t = timer4();
    clear();
    EXPECT(pt_button(&t, &closed, true) == PT_BUTTON_NOTHING);
    EXPECT_STR(out, "K\r\n"); /* default: the button resets */
    EXPECT(t.mode == PT_READY);
    EXPECT(t.key_to_press == 0);
    pt_gate_tripped(&t, 0, 1000);
    clear();
    pt_button(&t, &closed, true); /* mid-race: ends it */
    EXPECT(t.mode == PT_FINISH);
    EXPECT_STR(out, "1 - 99.9990\r\n2 - 99.9990\r\n3 - 99.9990\r\n4 - 99.9990\r\n");
    t.mode = PT_TEST;
    EXPECT(pt_button(&t, &closed, true) == PT_BUTTON_LEAVE_TEST);
}

static void test_button_next_mode(void) {
    pt_timer t = timer4();
    send(&t, "#button next\r", &closed);
    EXPECT(t.cfg.button_next);
    EXPECT(t.cfg.button_key == 0x28); /* enter, by default */
    send(&t, "R", &closed);
    pt_gate_tripped(&t, 0, 1000);
    pt_lane_tripped(&t, 0, 2001000);
    clear();
    pt_button(&t, &closed, true); /* racing: still ends the race */
    EXPECT(t.mode == PT_FINISH);
    EXPECT(t.key_to_press == 0);
    clear();
    pt_button(&t, &closed, true); /* finished: asks for the next heat */
    EXPECT_STR(out, "#next\r\n");
    EXPECT(t.key_to_press == 0x28);
    EXPECT(t.mode == PT_FINISH); /* the software resets it when it's ready */
    t.key_to_press = 0;
    clear();
    pt_button(&t, &closed, false); /* nobody listening: falls back to a reset */
    EXPECT_STR(out, "K\r\n");
    EXPECT(t.key_to_press == 0);
    send(&t, "#button-key none\r", &closed);
    clear();
    pt_button(&t, &closed, true);
    EXPECT_STR(out, "#next\r\n");
    EXPECT(t.key_to_press == 0); /* no key: the serial message only */
    send(&t, "#button reset\r", &closed);
    EXPECT(!t.cfg.button_next);
}

static void test_button_key_setting(void) {
    pt_timer t = timer4();
    clear();
    send(&t, "#button-key PageDown\r", &closed);
    EXPECT_STR(out, ".\r\n");
    EXPECT(t.cfg.button_key == 0x4E);
    clear();
    send(&t, "#button-key banana\r", &closed);
    EXPECT_STR(out, "?\r\n");
    EXPECT(t.cfg.button_key == 0x4E);
    uint8_t code;
    char buf[8];
    EXPECT(pt_key_code("n", &code) && code == 0x11);
    EXPECT(pt_key_code("0", &code) && code == 0x27);
    EXPECT(pt_key_code("f5", &code) && code == 0x3E);
    EXPECT(pt_key_code("f12", &code) && code == 0x45);
    EXPECT(!pt_key_code("f13", &code));
    EXPECT(!pt_key_code("f05", &code));
    EXPECT(!pt_key_code("", &code));
    EXPECT_STR(pt_key_name(0x11, buf, sizeof buf), "n");
    EXPECT_STR(pt_key_name(0x3E, buf, sizeof buf), "f5");
    EXPECT_STR(pt_key_name(0x2C, buf, sizeof buf), "space");
    EXPECT_STR(pt_key_name(0, buf, sizeof buf), "none");
    send(&t, "#button next\r", &closed);
    clear();
    send(&t, "I", &closed);
    EXPECT(strstr(out, " button next, key pagedown\r\n") != NULL);
}


/* ---------------------------------------------------------------- Several tracks, a start gate each */
static pt_timer timer_tracks(void) { /* 8 lanes, four 2-lane tracks on one board */
    pt_timer t;
    pt_config c = pt_default_config();
    c.num_lanes = 8;
    clear();
    pt_init(&t, &c, capture, NULL);
    send(&t, "#gate2 3-4\n#gate3 5-6\n#gate4 7-8\n", &closed);
    clear();
    return t;
}

static void test_one_gate_by_default(void) {
    pt_config c = pt_default_config();
    c.num_lanes = 8;
    EXPECT(pt_gates_in_use(&c) == 1);
    for (int lane = 0; lane < PT_MAX_LANES; lane++) EXPECT(c.lane_gate[lane] == 0);
    /* the spare gates being "open" (nothing plugged in) doesn't matter while no lane uses them */
    pt_timer t;
    clear();
    pt_init(&t, &c, capture, NULL);
    pt_inputs spare_open = {.gates_open = 0x0E, .lane_raw = 0};
    clear();
    send(&t, "R", &spare_open);
    EXPECT_STR(out, "K\r\n");
    /* and they don't start anything */
    clear();
    pt_gate_tripped(&t, 1, 1000);
    EXPECT_STR(out, "");
    EXPECT(t.mode == PT_READY);
}

static void test_gate_settings(void) {
    pt_timer t = timer_tracks();
    EXPECT(pt_gates_in_use(&t.cfg) == 0x0F);
    EXPECT(t.cfg.lane_gate[1] == 0 && t.cfg.lane_gate[2] == 1 && t.cfg.lane_gate[5] == 2 && t.cfg.lane_gate[7] == 3);
    clear();
    send(&t, "#gate2 3,4,5-6\n", &closed); /* lists and ranges; lanes move from gate 3 */
    EXPECT_STR(out, ".\r\n");
    EXPECT(t.cfg.lane_gate[2] == 1 && t.cfg.lane_gate[4] == 1 && t.cfg.lane_gate[5] == 1 && t.cfg.lane_gate[6] == 3);
    clear();
    send(&t, "#gate4 none\n", &closed); /* gate 4's lanes back to gate 1 */
    EXPECT_STR(out, ".\r\n");
    EXPECT(t.cfg.lane_gate[6] == 0 && t.cfg.lane_gate[7] == 0 && t.cfg.lane_gate[5] == 1);
    clear();
    send(&t, "#gate5 1-3\n#gate2 0-3\n#gate2 4-13\n#gate2 6-4\n#gate2\n#gate1 none\n#gate2 4-x\n", &closed);
    EXPECT_STR(out, "?\r\n?\r\n?\r\n?\r\n?\r\n?\r\n?\r\n");
    clear();
    send(&t, "#gate1 1-8\n", &closed); /* everything back on gate 1 */
    EXPECT_STR(out, ".\r\n");
    EXPECT(pt_gates_in_use(&t.cfg) == 1);
    /* #info lists the gates when there's more than one */
    t = timer_tracks();
    send(&t, "#info\n", &closed);
    EXPECT(strstr(out, " gate 1: lanes 1-2\r\n") && strstr(out, " gate 2: lanes 3-4\r\n") &&
           strstr(out, " gate 4: lanes 7-8\r\n"));
}

static void test_race_from_four_gates(void) {
    pt_timer t = timer_tracks();
    pt_inputs gate3_open = {.gates_open = 0x04, .lane_raw = 0};
    send(&t, "R", &gate3_open);
    EXPECT_STR(out, "O\r\n"); /* any gate in use open: not ready */
    clear();
    send(&t, "G", &gate3_open);
    EXPECT_STR(out, "O\r\n");
    clear();
    send(&t, "R", &closed);
    EXPECT_STR(out, "K\r\n");
    clear();
    /* the gates drop a little apart; each track's cars are timed from their own gate */
    pt_gate_tripped(&t, 1, 1000000);
    EXPECT_STR(out, "B\r\n"); /* the first gate starts the race */
    clear();
    pt_lane_tripped(&t, 0, 1500000); /* lane 1 before gate 1 opened: not a finish */
    pt_gate_tripped(&t, 0, 1002000);
    pt_gate_tripped(&t, 2, 1004000);
    pt_gate_tripped(&t, 3, 1010000);
    pt_gate_tripped(&t, 1, 1500000); /* a gate opening twice: the first counts */
    EXPECT_STR(out, "");
    for (uint8_t lane = 0; lane < 8; lane++) {
        uint64_t start[4] = {1002000, 1000000, 1004000, 1010000};
        pt_lane_tripped(&t, lane, start[lane / 2] + 2500000 + lane * 1000u);
    }
    EXPECT_STR(out, "1 - 2.5000\r\n2 - 2.5010\r\n3 - 2.5020\r\n4 - 2.5030\r\n5 - 2.5040\r\n6 - 2.5050\r\n"
                    "7 - 2.5060\r\n8 - 2.5070\r\n");
    EXPECT(t.place[0] == 1 && t.place[7] == 8);
}

static void test_eight_local_lanes_at_most(void) {
    pt_timer t = timer4();
    clear();
    send(&t, "#lanes 9\n#lanes 8\n", &closed);
    EXPECT_STR(out, "?\r\n.\r\n");
}

static void test_mode_test_gates(void) {
    pt_timer t = timer_tracks();
    pt_enter_test(&t);
    clear();
    pt_inputs in = {.gates_open = 0x02, .lane_raw = 0x81};
    pt_test_report(&t, &in);
    EXPECT_STR(out, "test lanes=1......1 gates=1:closed,2:open,3:closed,4:closed\r\n");
}

int main(void) {
    test_power_on_and_reset();
    test_a_race();
    test_masked_lanes_and_forced_end();
    test_masked_race_finishes_when_the_rest_are_in();
    test_reset_mid_race_ends_it_then_rearms();
    test_gate_ignored_unless_ready();
    test_queries();
    test_settings();
    test_formatting();
    test_mode_test();
    test_button_reset_mode();
    test_button_next_mode();
    test_button_key_setting();
    test_one_gate_by_default();
    test_gate_settings();
    test_race_from_four_gates();
    test_eight_local_lanes_at_most();
    test_mode_test_gates();
    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all timer core tests passed\n");
    return 0;
}
