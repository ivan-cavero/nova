#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

/* PS/2 keyboard I/O ports */
#define KBD_DATA    0x60
#define KBD_STATUS  0x64
#define KBD_CMD     0x64

/* Status register bits */
#define KBD_STATUS_OUT_FULL  0x01   /* Output buffer has data */
#define KBD_STATUS_IN_FULL   0x02   /* Input buffer is full */

/* Circular buffer size (power of 2 for masking) */
#define KBD_BUFFER_SIZE 256

/* Public API */
void keyboard_init(void);
bool keyboard_getchar(char *c);
char keyboard_readchar(void);
bool keyboard_has_data(void);

/* Test injection (bypasses hardware) */
void keyboard_test_reset(void);
void keyboard_test_inject(uint8_t scancode);

/* Called from IRQ1 handler */
void keyboard_irq(void);

#endif /* KEYBOARD_H */