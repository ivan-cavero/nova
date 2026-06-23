/* PIT Timer — 1000 Hz interrupt timer */
/* Nova kernel Phase 1 */

#include "timer.h"
#include "idt.h"
#include "io.h"
#include "ports.h"
#include "attributes.h"

/* Tick counter — incremented by IRQ0 handler */
static volatile uint64_t timer_ticks = 0;

/* Current frequency */
static uint32_t timer_freq = TIMER_DEFAULT_FREQ;

/* ==================== Init ==================== */

void timer_init(uint32_t frequency) {
    log_entry_t e = log_begin("PIT timer at");

    timer_freq = frequency;

    /* Calculate divisor: 1193182 / freq */
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / frequency);

    /* CW: channel 0, access mode lobyte/hibyte, mode 2 (rate generator), binary */
    outb(PIT_CMD, 0x36);

    /* Divisor low byte */
    outb(PIT_DATA, (uint8_t)(divisor & 0xFF));
    io_wait();

    /* Divisor high byte */
    outb(PIT_DATA, (uint8_t)((divisor >> 8) & 0xFF));
    io_wait();

    log_end(&e, true);
}

/* ==================== Queries ==================== */

uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

void timer_sleep(uint32_t milliseconds) {
    uint64_t target = timer_ticks + (uint64_t)(milliseconds / (1000 / timer_freq));
    while (timer_ticks < target) {
        halt();
    }
}

/* ==================== IRQ0 Handler ==================== */

void timer_tick(void) {
    timer_ticks++;
}