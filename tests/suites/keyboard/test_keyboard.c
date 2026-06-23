#include "suites.h"
/* Keyboard Runtime Tests — inject scancodes, read buffer */
#include "test_runner.h"
#include "keyboard.h"
static test_suite_t suite;

static const char *test_empty(void) {
    keyboard_test_reset();
    if (keyboard_has_data()) return "Buffer not empty";
    return NULL;
}
static const char *test_key_a(void) {
    keyboard_test_reset();
    keyboard_test_inject(0x1E);
    char c; if (!keyboard_getchar(&c)) return "No char from 0x1E";
    if (c != 'a') return "Expected 'a'";
    return NULL;
}
static const char *test_shift_a(void) {
    keyboard_test_reset();
    keyboard_test_inject(0x2A); keyboard_test_inject(0x1E);
    char c; keyboard_getchar(&c);
    if (c != 'A') return "Expected 'A' with shift";
    return NULL;
}
static const char *test_shift_release(void) {
    keyboard_test_reset();
    keyboard_test_inject(0x2A); keyboard_test_inject(0xAA); keyboard_test_inject(0x1E);
    char c; keyboard_getchar(&c);
    if (c != 'a') return "Expected 'a' after shift release";
    return NULL;
}
static const char *test_overflow(void) {
    keyboard_test_reset();
    for (int i = 0; i < 300; i++) keyboard_test_inject(0x1E);
    int cnt = 0; char c;
    while (keyboard_getchar(&c)) cnt++;
    test_print_u64("Buffer count", (uint64_t)cnt);
    if (cnt > 256) return "Buffer overflow";
    return NULL;
}
static const char *test_digits(void) {
    keyboard_test_reset();
    for (uint8_t s = 0x02; s <= 0x0B; s++) keyboard_test_inject(s);
    const char exp[] = "1234567890";
    for (int i = 0; i < 10; i++) {
        char c; keyboard_getchar(&c);
        if (c != exp[i]) return "Digit mismatch";
    }
    return NULL;
}
static const char *test_shift_digits(void) {
    keyboard_test_reset();
    keyboard_test_inject(0x2A);
    keyboard_test_inject(0x02); keyboard_test_inject(0x05);
    char c1, c2; keyboard_getchar(&c1); keyboard_getchar(&c2);
    if (c1 != '!' || c2 != '$') return "Shift+digits wrong";
    return NULL;
}
void run_suite_keyboard(void) {
    suite_init(&suite, "Keyboard");
    suite_add(&suite, "Buffer empty",   "no data at boot",     test_empty);
    suite_add(&suite, "Key 'a'",        "0x1E -> 'a'",        test_key_a);
    suite_add(&suite, "Shift + 'a'",    "0x2A+0x1E -> 'A'",   test_shift_a);
    suite_add(&suite, "Shift release",  "back to 'a'",         test_shift_release);
    suite_add(&suite, "No overflow",    "300 injects safe",    test_overflow);
    suite_add(&suite, "Digits 1-0",     "0x02-0x0B correct",  test_digits);
    suite_add(&suite, "Shift+digits",   "! and $",             test_shift_digits);
    suite_run(&suite);
}