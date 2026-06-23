/* PS/2 Keyboard driver — Nova kernel Phase 1 */

#include "keyboard.h"
#include "idt.h"
#include "io.h"
#include "ports.h"
#include "attributes.h"

/* ==================== Scancode Set 1 → ASCII ==================== */

/* Normal (unshifted) scancode -> ASCII mapping */
static const char scancode_ascii[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8',     /* 0-9 */
    '9', '0', '-', '=', 0,   0,   'q', 'w', 'e', 'r',     /* 10-19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',0,       /* 20-29 */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 30-39 */
    '\'','`', 0,   '\\','z', 'x', 'c', 'v', 'b', 'n',      /* 40-49 */
    'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,       /* 50-59 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 60-69 */
    0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 70-79 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 80-89 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 90-99 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 100-109 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 110-119 */
    0,   0,   0,   0,   0,   0,   0,   0                  /* 120-127 */
};

/* Shifted scancode -> ASCII mapping */
static const char scancode_shift[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*',     /* 0-9 */
    '(', ')', '_', '+', 0,   0,   'Q', 'W', 'E', 'R',     /* 10-19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',0,       /* 20-29 */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',     /* 30-39 */
    '"', '~', 0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N',     /* 40-49 */
    'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,       /* 50-59 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 60-69 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 70-79 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,       /* 80-89 */
};

/* ==================== Circular Buffer ==================== */

static volatile char kbd_buffer[KBD_BUFFER_SIZE];
static volatile uint16_t kbd_head = 0;  /* Write position (ISR writes here) */
static volatile uint16_t kbd_tail = 0;  /* Read position (main reads here) */

/* Mask for circular buffer index */
#define KBD_MASK (KBD_BUFFER_SIZE - 1)

/* ==================== Keyboard State ==================== */

static bool shift_pressed = false;
static bool ctrl_pressed  = false;
static bool alt_pressed   = false;

/* ==================== Scancode Handling ==================== */

static void keyboard_handle_scancode(uint8_t scancode) {
    char ascii;

    /* Key release = scancode | 0x80 */
    if (scancode & 0x80) {
        uint8_t release = scancode & 0x7F;

        /* Track modifier keys */
        if (release == 0x2A || release == 0x36) shift_pressed = false;
        if (release == 0x1D) ctrl_pressed  = false;
        if (release == 0x38) alt_pressed   = false;

        return;  /* Release — ignore for now */
    }

    /* Track modifier keys on press */
    if (scancode == 0x2A || scancode == 0x36) { shift_pressed = true; return; }
    if (scancode == 0x1D) { ctrl_pressed  = true; return; }
    if (scancode == 0x38) { alt_pressed   = true; return; }

    /* Ignore other non-printable keys */
    if (scancode >= 128) return;

    /* Convert to ASCII */
    ascii = shift_pressed ? scancode_shift[scancode] : scancode_ascii[scancode];
    if (ascii == 0) return;  /* Unmapped key */

    /* Add to circular buffer */
    uint16_t next = (kbd_head + 1) & KBD_MASK;
    if (next != kbd_tail) {          /* Buffer not full */
        kbd_buffer[kbd_head] = ascii;
        kbd_head = next;
    }
}

/* ==================== Init ==================== */

void keyboard_init(void) {
    log_entry_t e = log_begin("PS/2 keyboard");

    /* Wait for keyboard controller to be ready */
    while (inb(KBD_STATUS) & KBD_STATUS_IN_FULL) { cpu_pause(); }

    /* Flush any pending data in output buffer */
    while (inb(KBD_STATUS) & KBD_STATUS_OUT_FULL) {
        (void)inb(KBD_DATA);
    }

    log_end(&e, true);
}

/* ==================== IRQ Handler ==================== */

void keyboard_irq(void) {
    /* Read scancode from data port */
    uint8_t scancode = inb(KBD_DATA);
    keyboard_handle_scancode(scancode);
}

/* ==================== Public API ==================== */

bool keyboard_has_data(void) {
    return kbd_head != kbd_tail;
}

bool keyboard_getchar(char *c) {
    if (kbd_head == kbd_tail) return false;

    *c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) & KBD_MASK;
    return true;
}

char keyboard_readchar(void) {
    char c;
    while (!keyboard_getchar(&c)) { halt(); }
    return c;
}