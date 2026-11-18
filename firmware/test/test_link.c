/* Tests for linked gates (link.c with timer_core.c), run on a computer: make -C firmware/test
 * Up to four simulated gates, each a timer and a link, wired LINK OUT to LINK IN through in-memory buffers */
#include <stdio.h>
#include <string.h>

#include "../src/link.h"

typedef struct {
    pt_timer t;
    pt_link l;
    char usb[4096];               /* what it says to the computer */
    char to_up[4096], to_down[4096]; /* waiting to go out of LINK IN and LINK OUT */
    bool cut_down;                /* its LINK OUT cable unplugged */
    pt_inputs in;
} gate;

#define MAX_GATES 4
static gate g[MAX_GATES];
static int n_gates;
static uint64_t now;

static int failures = 0;
#define EXPECT(cond)                                                                                          \
    do {                                                                                                      \
        if (!(cond)) {                                                                                        \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
            failures++;                                                                                       \
        }                                                                                                     \
    } while (0)
#define EXPECT_STR(actual, expected)                                                                          \
    do {                                                                                                      \
        if (strcmp((actual), (expected)) != 0) {                                                              \
            printf("FAIL %s:%d\n  expected: \"%s\"\n  actual:   \"%s\"\n", __FILE__, __LINE__, (expected),     \
                   (actual));                                                                                 \
            failures++;                                                                                       \
        }                                                                                                     \
    } while (0)

static void to_usb(const char *text, void *ctx) {
    gate *x = ctx;
    strncat(x->usb, text, sizeof x->usb - strlen(x->usb) - 1);
}
static void to_up(const char *line, void *ctx) {
    gate *x = ctx;
    strncat(x->to_up, line, sizeof x->to_up - strlen(x->to_up) - 1);
}
static void to_down(const char *line, void *ctx) {
    gate *x = ctx;
    strncat(x->to_down, line, sizeof x->to_down - strlen(x->to_down) - 1);
}

/* Moves everything waiting on the cables to the other end, until it's all delivered */
static void pump(void) {
    for (int round = 0; round < 20; round++) {
        for (int i = 0; i < n_gates; i++) link_poll(&g[i].l, now); /* what the timers did goes out first */
        bool moved = false;
        for (int i = 0; i < n_gates; i++) {
            gate *a = &g[i];
            if (a->to_down[0]) {
                char buf[4096];
                strcpy(buf, a->to_down);
                a->to_down[0] = 0;
                if (i + 1 < n_gates && !a->cut_down)
                    for (char *c = buf; *c; c++) link_from_up(&g[i + 1].l, *c, now, &g[i + 1].in);
                moved = true;
            }
            if (a->to_up[0]) {
                char buf[4096];
                strcpy(buf, a->to_up);
                a->to_up[0] = 0;
                if (i > 0 && !g[i - 1].cut_down)
                    for (char *c = buf; *c; c++) link_from_down(&g[i - 1].l, *c, now, &g[i - 1].in);
                moved = true;
            }
        }
        if (!moved) return;
    }
}

/* Time passes, in steps of 10 ms */
static void run_ms(int ms) {
    for (int k = 0; k < ms / 10; k++) {
        now += 10000;
        for (int i = 0; i < n_gates; i++) link_poll(&g[i].l, now);
        pump();
    }
}

static void clear_usb(void) {
    for (int i = 0; i < n_gates; i++) g[i].usb[0] = 0;
}

static void chain(int n, int lanes) {
    memset(g, 0, sizeof g);
    n_gates = n;
    now = 1000000;
    for (int i = 0; i < n; i++) {
        pt_config c = pt_default_config();
        c.num_lanes = (uint8_t)lanes;
        pt_init(&g[i].t, &c, to_usb, &g[i]);
        link_init(&g[i].l, &g[i].t, to_up, to_down, &g[i]);
    }
    run_ms(600);
    clear_usb();
}

static void send(gate *x, const char *chars) {
    for (const char *c = chars; *c; c++) pt_receive(&x->t, *c, &x->in);
    pump();
}

static void test_chain_forms(void) {
    chain(3, 4);
    EXPECT(link_is_main(&g[0].l) && !link_is_main(&g[1].l) && !link_is_main(&g[2].l));
    EXPECT(g[1].l.position == 2 && g[1].l.first_lane == 4);
    EXPECT(g[2].l.position == 3 && g[2].l.first_lane == 8);
    EXPECT(g[0].t.remote_lanes == 8);
    send(&g[0], "N");
    EXPECT_STR(g[0].usb, "numl=12\r\n");
    /* gates with different lane counts: 3 + 4 + 2 */
    chain(3, 3);
    g[1].t.cfg.num_lanes = 4;
    g[2].t.cfg.num_lanes = 2;
    run_ms(600);
    EXPECT(g[1].l.first_lane == 3 && g[2].l.first_lane == 7 && g[0].t.remote_lanes == 6);
}

static void test_ready_needs_every_gate(void) {
    chain(3, 4);
    g[2].in.gates_open = 1; /* the last track's start gate is down */
    send(&g[0], "R");
    run_ms(50);
    EXPECT_STR(g[0].usb, "O\r\n");
    clear_usb();
    g[2].in.gates_open = 0;
    send(&g[0], "R");
    run_ms(50);
    EXPECT_STR(g[0].usb, "K\r\n");
    EXPECT(g[1].t.mode == PT_READY && g[2].t.mode == PT_READY);
}

/* Each gate opens its start gate at its own moment, and times its lanes from it */
static void race(const uint64_t start[], int gates, int lanes) {
    for (int i = 0; i < gates; i++) pt_gate_tripped(&g[i].t, 0, start[i]);
    pump();
    for (int i = 0; i < gates; i++)
        for (int lane = 0; lane < lanes; lane++)
            pt_lane_tripped(&g[i].t, (uint8_t)lane, start[i] + 2500000 + (uint64_t)(i * lanes + lane) * 1000u);
    pump();
    run_ms(20);
}

static void test_a_twelve_lane_race(void) {
    chain(3, 4);
    send(&g[0], "R");
    run_ms(50);
    clear_usb();
    /* gate 2's track opens first, then the main gate's, then gate 3's: each lane is timed from its own */
    uint64_t start[3] = {now + 2000, now, now + 5000};
    pt_gate_tripped(&g[1].t, 0, start[1]);
    pump();
    EXPECT_STR(g[0].usb, "B\r\n"); /* the first gate to open starts the race */
    clear_usb();
    race(start, 3, 4);
    EXPECT_STR(g[0].usb, "1 - 2.5000\r\n2 - 2.5010\r\n3 - 2.5020\r\n4 - 2.5030\r\n5 - 2.5040\r\n6 - 2.5050\r\n"
                         "7 - 2.5060\r\n8 - 2.5070\r\n9 - 2.5080\r\n10 - 2.5090\r\n11 - 2.5100\r\n12 - 2.5110\r\n");
    EXPECT(g[0].t.mode == PT_FINISH && g[0].t.place[0] == 1 && g[0].t.place[11] == 12);
    /* and again: Q repeats them */
    clear_usb();
    send(&g[0], "Q");
    EXPECT(strstr(g[0].usb, "12 - 2.5110\r\n") != NULL);
}

static void test_masks_reach_the_other_gates(void) {
    chain(3, 4);
    send(&g[0], "M9MAMBMCM2"); /* lanes 9 to 12 (gate 3, all of it) and lane 2 */
    send(&g[0], "R");
    run_ms(50);
    EXPECT(g[2].t.mask == 0x0F && g[1].t.mask == 0 && g[0].t.mask == ((1u << 1) | (0xFu << 8)));
    clear_usb();
    /* gate 3's track doesn't race; the heat doesn't wait for it */
    uint64_t start[2] = {now, now};
    race(start, 2, 4);
    EXPECT(g[0].t.mode == PT_FINISH);
    EXPECT(strstr(g[0].usb, "2 - 99.9990\r\n") && strstr(g[0].usb, "10 - 99.9990\r\n") && strstr(g[0].usb, "8 - 2.5070\r\n"));
}

static void test_ending_early(void) {
    chain(3, 4);
    send(&g[0], "R");
    run_ms(50);
    for (int i = 0; i < 3; i++) pt_gate_tripped(&g[i].t, 0, now);
    pump();
    pt_lane_tripped(&g[0].t, 0, now + 2400000);
    pt_lane_tripped(&g[2].t, 3, now + 2600000); /* lane 12 */
    pump();
    clear_usb();
    send(&g[0], "F"); /* a car stuck: end it */
    run_ms(20);
    EXPECT(g[0].t.mode == PT_FINISH);
    EXPECT(strstr(g[0].usb, "1 - 2.4000\r\n") && strstr(g[0].usb, "5 - 99.9990\r\n") && strstr(g[0].usb, "12 - 2.6000\r\n"));
}

static void test_unplugged_gate(void) {
    chain(3, 4);
    g[1].cut_down = true; /* gate 3 unplugged */
    run_ms(1200);
    EXPECT(g[0].t.remote_lanes == 4);  /* only gate 2's lanes now */
    EXPECT(link_is_main(&g[2].l));     /* gate 3 is a timer on its own again */
    clear_usb();
    send(&g[0], "N");
    EXPECT_STR(g[0].usb, "numl=8\r\n");
    /* gate 2 unplugged mid-race: the end waits a second for it, then gives its lanes no time */
    send(&g[0], "R");
    run_ms(50);
    pt_gate_tripped(&g[0].t, 0, now);
    pump();
    g[0].cut_down = true;
    clear_usb();
    send(&g[0], "F");
    run_ms(500);
    EXPECT(g[0].t.mode == PT_RACING);
    run_ms(600);
    EXPECT(g[0].t.mode == PT_FINISH && strstr(g[0].usb, "8 - 99.9990\r\n") != NULL);
}

static void test_no_answer_means_not_ready(void) {
    chain(2, 4);
    g[0].cut_down = true; /* the cable goes, just as the race software resets */
    send(&g[0], "R");
    run_ms(600);
    EXPECT_STR(g[0].usb, "O\r\n");
}

static void test_three_gates_at_most(void) {
    chain(4, 4);
    EXPECT(g[0].t.remote_lanes == 8);
    EXPECT(link_is_main(&g[3].l)); /* the fourth stays out */
}

static void test_damaged_lines_are_ignored(void) {
    chain(2, 4);
    char line[32];
    link_frame("L 7", line, sizeof line);
    EXPECT_STR(line, "L 7*5B\n");
    line[2] = '9'; /* garbled on the way */
    for (char *c = line; *c; c++) link_from_down(&g[0].l, *c, now, &g[0].in);
    EXPECT(g[0].l.down_lanes == 4);
}

static void test_single_gate_unchanged(void) {
    chain(1, 4);
    send(&g[0], "R");
    run_ms(20);
    EXPECT_STR(g[0].usb, "K\r\n");
    clear_usb();
    uint64_t start[1] = {now};
    race(start, 1, 4);
    EXPECT_STR(g[0].usb, "B\r\n1 - 2.5000\r\n2 - 2.5010\r\n3 - 2.5020\r\n4 - 2.5030\r\n");
}

int main(void) {
    test_chain_forms();
    test_ready_needs_every_gate();
    test_a_twelve_lane_race();
    test_masks_reach_the_other_gates();
    test_ending_early();
    test_unplugged_gate();
    test_no_answer_means_not_ready();
    test_three_gates_at_most();
    test_damaged_lines_are_ignored();
    test_single_gate_unchanged();
    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("all link tests passed\n");
    return 0;
}
