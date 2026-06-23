#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <stdbool.h>
#include "attributes.h"

/* ==================== IDT Entry (8 bytes) ==================== */
typedef struct {
    uint16_t offset_lo;    /* Handler address [15:0]   */
    uint16_t selector;     /* Code segment (0x08)      */
    uint8_t  ist;          /* Interrupt Stack Table (0 for 32-bit) */
    uint8_t  flags;        /* Present, DPL, gate type  */
    uint16_t offset_hi;    /* Handler address [31:16]  */
} PACKED idt_entry_t;

/* ==================== IDTR (6 bytes, passed to lidt) ==================== */
typedef struct {
    uint16_t limit;        /* sizeof(IDT) - 1 = 2047   */
    uint32_t base;         /* Linear address of IDT     */
} PACKED idtr_t;

/* ==================== CPU register state (pushed by common ISR) ==================== */
typedef struct {
    /* PUSHA order: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (low addr → high addr) */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;          /* Original ESP before PUSHA */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t vector;       /* Pushed by ISR stub */
    uint32_t error_code;   /* Pushed by CPU (or 0 by stub) */

    uint32_t eip;          /* Pushed by CPU on interrupt */
    uint32_t cs;
    uint32_t eflags;
} PACKED registers_t;

/* ==================== IDT Flags ==================== */
#define IDT_FLAG_PRESENT     0x80
#define IDT_FLAG_DPL0        0x00
#define IDT_FLAG_DPL3        0x60
#define IDT_FLAG_INT_GATE    0x0E    /* 32-bit interrupt gate */
#define IDT_FLAG_TRAP_GATE   0x0F    /* 32-bit trap gate */

#define IDT_FLAG_INT32_DPL0  (IDT_FLAG_PRESENT | IDT_FLAG_DPL0 | IDT_FLAG_INT_GATE)   /* 0x8E */
#define IDT_FLAG_INT32_DPL3  (IDT_FLAG_PRESENT | IDT_FLAG_DPL3 | IDT_FLAG_INT_GATE)   /* 0xEE */
#define IDT_FLAG_TRAP_DPL0   (IDT_FLAG_PRESENT | IDT_FLAG_DPL0 | IDT_FLAG_TRAP_GATE)  /* 0x8F */

/* ==================== PIC I/O Ports ==================== */
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define PIC1_OFFSET 0x20   /* IRQ0-7 → INT 32-39 */
#define PIC2_OFFSET 0x28   /* IRQ8-15 → INT 40-47 */

/* ==================== Exception Vector Numbers ==================== */
#define EXC_DE   0
#define EXC_DB   1
#define EXC_NMI  2
#define EXC_BP   3
#define EXC_OF   4
#define EXC_BR   5
#define EXC_UD   6
#define EXC_NM   7
#define EXC_DF   8
#define EXC_TS   10
#define EXC_NP   11
#define EXC_SS   12
#define EXC_GP   13
#define EXC_PF   14
#define EXC_MF   16
#define EXC_AC   17
#define EXC_MC   18
#define EXC_XM   19

/* ==================== Public API ==================== */
void idt_init(void);
void pic_remap(void);

/* Exposed for testing */
extern const char * const exc_names[32];

/* C handlers called from assembly stubs */
void exception_handler(registers_t *r);
void irq_handler(registers_t *r);

#endif /* IDT_H */