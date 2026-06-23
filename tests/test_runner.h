#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdint.h>
#include "io.h"

/* A test function returns NULL on pass, error string on fail, (char*)1 on skip */
typedef const char *(*test_fn)(void);

/* Suite collects a set of tests */
typedef struct {
    const char *name;
    const char *description;
    test_fn     func;
} test_entry_t;

#define TEST_SUITE_MAX 256

typedef struct {
    const char   *suite_name;
    test_entry_t  entries[TEST_SUITE_MAX];
    int           count;
    int           passed;
    int           failed;
} test_suite_t;

/* Framework API */
void test_init(void);
void suite_init(test_suite_t *suite, const char *name);
void suite_add(test_suite_t *suite, const char *name, const char *desc, test_fn func);
void suite_run(test_suite_t *suite);
void suite_summary(void);

/* Utilities for tests */
void test_assert_msg(const char *msg, int line);
void test_print_u64(const char *label, uint64_t val);
void test_print_x32(const char *label, uint32_t val);

/* Total counts */
extern int test_total_pass;
extern int test_total_fail;

/* Entry point */
extern void run_all_tests(void);

#endif /* TEST_RUNNER_H */