#include "suites.h"
/* Timer Runtime Tests — Nova kernel */
#include "test_runner.h"
#include "timer.h"
#include "ports.h"
#include "attributes.h"
static test_suite_t suite;

static const char *test_runs(void) {
    uint64_t t0 = timer_get_ticks();
    while (timer_get_ticks() < t0 + 5) { halt(); }
    if (timer_get_ticks() <= t0) return "Timer not incrementing";
    return NULL;
}
static const char *test_accuracy(void) {
    uint64_t t0 = timer_get_ticks();
    while (timer_get_ticks() < t0 + 1000) { halt(); }
    uint64_t elapsed = timer_get_ticks() - t0;
    test_print_u64("Ticks in ~1s", elapsed);
    if (elapsed < 980 || elapsed > 1020) return "Timer off >2%";
    return NULL;
}
static const char *test_monotonic(void) {
    uint64_t last = timer_get_ticks();
    for (int i = 0; i < 100; i++) {
        while (timer_get_ticks() == last) { cpu_pause(); }
        uint64_t cur = timer_get_ticks();
        if (cur <= last) return "Timer went backwards";
        last = cur;
    }
    return NULL;
}
static const char *test_stress(void) {
    uint64_t t0 = timer_get_ticks();
    while (timer_get_ticks() < t0 + 5000) { halt(); }
    uint64_t elapsed = timer_get_ticks() - t0;
    test_print_u64("Ticks in ~5s", elapsed);
    if (elapsed < 4900 || elapsed > 5100) return "Failed 5s stress";
    return NULL;
}
static const char *test_pit_hw(void) {
    outb(0x43, 0x00);
    uint16_t ctr = (uint16_t)(inb(0x40) | ((uint16_t)inb(0x40) << 8));
    if (ctr >= 1193) return "PIT not counting down";
    return NULL;
}
void run_suite_timer(void) {
    suite_init(&suite, "Timer");
    suite_add(&suite, "Timer runs",     "ticks increment",       test_runs);
    suite_add(&suite, "1s accuracy",    "1000 ticks +/-2%",     test_accuracy);
    suite_add(&suite, "Monotonic",      "100 ticks no reverse", test_monotonic);
    suite_add(&suite, "5s stress",      "5000 ticks +/-2%",     test_stress);
    suite_add(&suite, "PIT hardware",   "counter < 1193",       test_pit_hw);
    suite_run(&suite);
}