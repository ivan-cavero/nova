#include "suites.h"
/* Exception System Tests — Nova kernel */
#include "test_runner.h"
#include "idt.h"
static test_suite_t suite;
extern const uint32_t isr_stubs[32];
extern const uint32_t irq_stubs[16];

static const char *test_isr_linked(void) {
    for (uint32_t i = 0; i < 32; i++) if (isr_stubs[i] == 0) return "ISR stub not linked";
    return NULL;
}
static const char *test_irq_linked(void) {
    for (uint32_t i = 0; i < 16; i++) if (irq_stubs[i] == 0) return "IRQ stub not linked";
    return NULL;
}
static const char *test_handler(void) {
    struct { uint16_t lim; uint32_t base; } PACKED idtr;
    __asm__ volatile ("sidt %0" : "=m"(idtr));
    volatile uint16_t *lo = (volatile uint16_t *)(idtr.base);
    volatile uint16_t *hi = (volatile uint16_t *)(idtr.base + 6);
    uint32_t addr = *lo | ((uint32_t)(*hi) << 16);
    if (addr < 0x100000) return "Handler addr < 1MB";
    return NULL;
}
void run_suite_exceptions(void) {
    suite_init(&suite, "Exceptions");
    suite_add(&suite, "32 ISR linked","all stubs resolved",  test_isr_linked);
    suite_add(&suite, "16 IRQ linked","all stubs resolved",  test_irq_linked);
    suite_add(&suite, "Handler addr",  "vector 0 valid",     test_handler);
    suite_run(&suite);
}