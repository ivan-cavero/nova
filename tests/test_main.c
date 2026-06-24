/* Test suite entry point — Nova kernel */
/* Calls every suite in order */

#include "test_runner.h"

/* Suite entry points */
void run_suite_gdt(void);
void run_suite_idt(void);
void run_suite_timer(void);
void run_suite_pic(void);
void run_suite_keyboard(void);
void run_suite_exceptions(void);
void run_suite_heap(void);

void run_all_tests(void) {
    test_init();

    run_suite_gdt();        /* 8  tests: sgdt, entries, selectors, CR0 */
    run_suite_idt();        /* 6  tests: sidt, gates, selectors, types */
    run_suite_timer();      /* 5  tests: accuracy, monotonic, stress */
    run_suite_pic();        /* 4  tests: IMR readback, EOI safety */
    run_suite_keyboard();   /* 7  tests: scancode injection, buffer */
    run_suite_exceptions(); /* 3  tests: stubs linked, gates valid */
    run_suite_heap();       /* 11 tests: slab alloc/free/stress/OOM */

    suite_summary();
}