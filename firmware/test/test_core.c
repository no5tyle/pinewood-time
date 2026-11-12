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

static const pt_inputs closed = {.gate_open = false, .lane_raw = 0};
static const pt_inputs open_gate = {.gate_open = true, .lane_raw = 0};

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
    pt_gate_tripped(&t, 1000000);
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
    pt_gate_tripped(&t, 0);
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
    pt_gate_tripped(&t, 0);
    pt_lane_tripped(&t, 0, 2100000);
    pt_lane_tripped(&t, 1, 2200000);
    EXPECT_STR(out, "B\r\n1 - 2.1000\r\n2 - 2.2000\r\n3 - 99.9990\r\n4 - 99.9990\r\n");
}

static void test_reset_mid_race_ends_it_then_rearms(void) {
    pt_timer t = timer4();
    send(&t, "R", &closed);
    pt_gate_tripped(&t, 0);
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
    pt_gate_tripped(&t, 5); /* not reset yet */
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
    pt_inputs in = {.gate_open = false, .lane_raw = 0x05};
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
    send(&t, "#lanes 9\n", &closed);
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
    pt_gate_tripped(&t, 0);
    pt_lane_tripped(&t, 0, 100000000); /* 100 s or more: no time */
    pt_format_result(&t, 0, s, sizeof s);
    EXPECT_STR(s, "1 - 99.9990");
}

static void test_mode_test(void) {
    pt_timer t = timer4();
    clear();
    pt_enter_test(&t);
    pt_inputs in = {.gate_open = true, .lane_raw = 0x09};
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
    pt_gate_tripped(&t, 1000);
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
    pt_gate_tripped(&t, 1000);
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
    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all timer core tests passed\n");
    return 0;
}
