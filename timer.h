#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* PIT I/O ports */
#define PIT_CMD  0x43
#define PIT_DATA 0x40

/* PIT base frequency (raw crystal / 12) */
#define PIT_BASE_FREQ 1193182UL

/* Default tick rate */
#define TIMER_DEFAULT_FREQ 1000

/* Public API */
void timer_init(uint32_t frequency);
void timer_tick(void);
uint64_t timer_get_ticks(void);
void timer_sleep(uint32_t milliseconds);

#endif /* TIMER_H */