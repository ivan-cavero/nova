/* Suite declarations — Nova kernel */
/* Every suite entry point is declared here */

#ifndef TESTS_SUITES_H
#define TESTS_SUITES_H

void run_suite_gdt(void);
void run_suite_idt(void);
void run_suite_timer(void);
void run_suite_pic(void);
void run_suite_keyboard(void);
void run_suite_exceptions(void);

#endif /* TESTS_SUITES_H */