/* Test runner — Nova kernel — VGA + Serial output */
#include "test_runner.h"
#include "attributes.h"

int test_total_pass = 0;
int test_total_fail = 0;

/* Number printer to both VGA and serial */
static void print_num_both(uint64_t val) {
    char buf[22]; int idx = 20; buf[21] = '\0';
    if (val == 0) buf[--idx] = '0';
    while (val && idx > 0) { buf[--idx] = (char)('0' + (val % 10)); val /= 10; }
    vga_writestr(&buf[idx]); serial_writestr(&buf[idx]);
}

void test_init(void) {
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr("\n  ========================================\n");
    vga_writestr("  === NOVA RUNTIME TEST SUITE ===\n");
    vga_writestr("  ========================================\n\n");
    serial_writestr("\n========================================\n");
    serial_writestr("=== NOVA RUNTIME TEST SUITE ===\n");
    serial_writestr("========================================\n\n");
}

void suite_init(test_suite_t *suite, const char *name) {
    suite->suite_name = name; suite->count = 0; suite->passed = 0; suite->failed = 0;
    vga_setcolor(CLR_LCYAN, CLR_BLACK);
    vga_writestr("\n  >>> "); vga_writestr(name); vga_writestr(" <<<\n");
    serial_writestr("\n>>> "); serial_writestr(name); serial_writestr(" <<<\n");
    vga_setcolor(CLR_LGREY, CLR_BLACK);
}

void suite_add(test_suite_t *suite, const char *name, const char *desc, test_fn func) {
    if (suite->count < TEST_SUITE_MAX) {
        suite->entries[suite->count].name = name;
        suite->entries[suite->count].description = desc;
        suite->entries[suite->count].func = func;
        suite->count++;
    }
}

void suite_run(test_suite_t *suite) {
    for (int i = 0; i < suite->count; i++) {
        test_entry_t *te = &suite->entries[i];
        vga_writestr("  "); vga_writestr(te->name); vga_writestr("... ");
        serial_writestr("  "); serial_writestr(te->name); serial_writestr("... ");

        const char *result = te->func();

        if (result == NULL) {
            suite->passed++;
            vga_setcolor(CLR_LBLUE, CLR_BLACK); vga_writestr("PASS\n");
            serial_writestr("PASS\n");
        } else {
            suite->failed++;
            vga_setcolor(CLR_RED, CLR_BLACK); vga_writestr("FAIL\n");
            serial_writestr("FAIL\n");
            if (result != (const char *)1) {
                vga_setcolor(CLR_LRED, CLR_BLACK); vga_writestr("       ");
                vga_writestr(result); vga_putchar('\n');
                serial_writestr("       "); serial_writestr(result); serial_putchar('\n');
            }
        }
        vga_setcolor(CLR_LGREY, CLR_BLACK);
    }
    test_total_pass += suite->passed; test_total_fail += suite->failed;

    vga_setcolor(CLR_DGREY, CLR_BLACK);
    vga_writestr("  -> "); vga_writestr(suite->suite_name); vga_writestr(": ");
    serial_writestr("  -> "); serial_writestr(suite->suite_name); serial_writestr(": ");
    print_num_both((uint64_t)suite->passed);
    vga_writestr(" passed, "); serial_writestr(" passed, ");
    print_num_both((uint64_t)suite->failed);
    vga_writestr(" failed\n"); serial_writestr(" failed\n");
}

void suite_summary(void) {
    vga_putchar('\n'); serial_putchar('\n');
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr("  ========================================\n");
    vga_writestr("  === SUMMARY ===\n");
    vga_writestr("  ========================================\n");
    serial_writestr("========================================\n");
    serial_writestr("=== SUMMARY ===\n");
    serial_writestr("========================================\n");

    vga_writestr("  Total: "); serial_writestr("  Total: ");
    print_num_both((uint64_t)(test_total_pass + test_total_fail));
    vga_setcolor(CLR_LBLUE, CLR_BLACK);
    vga_writestr(" | Passed: "); serial_writestr(" | Passed: ");
    print_num_both((uint64_t)test_total_pass);
    vga_setcolor(test_total_fail > 0 ? CLR_RED : CLR_DGREY, CLR_BLACK);
    vga_writestr(" | Failed: "); serial_writestr(" | Failed: ");
    print_num_both((uint64_t)test_total_fail);
    vga_putchar('\n'); serial_putchar('\n');
    vga_setcolor(CLR_LGREY, CLR_BLACK);
}

void test_print_u64(const char *label, uint64_t val) {
    char buf[24]; int idx = 22; buf[23] = '\0';
    if (val == 0) buf[--idx] = '0';
    while (val && idx > 0) { buf[--idx] = (char)('0' + (val % 10)); val /= 10; }
    vga_writestr("    "); vga_writestr(label); vga_writestr(": "); vga_writestr(&buf[idx]); vga_putchar('\n');
    serial_writestr("    "); serial_writestr(label); serial_writestr(": "); serial_writestr(&buf[idx]); serial_putchar('\n');
}

void test_print_x32(const char *label, uint32_t val) {
    vga_writestr("    "); vga_writestr(label); vga_writestr(": 0x");
    serial_writestr("    "); serial_writestr(label); serial_writestr(": 0x");
    for (int s = 28; s >= 0; s -= 4) {
        uint8_t n = (uint8_t)((val >> s) & 0xF);
        char c = (char)(n < 10 ? '0' + n : 'A' + n - 10);
        vga_putchar(c); serial_putchar(c);
    }
    vga_putchar('\n'); serial_putchar('\n');
}