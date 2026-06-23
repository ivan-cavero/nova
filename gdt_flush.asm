; GDT flush — loads GDTR and reloads all segment registers
; Called once during kernel init. Interrupts must be disabled (GRUB guarantee).
;
; Performance: lgdt ~10 cycles, 6 mov segment regs ~5 cycles each,
;              far jmp ~15-20 cycles. Total: ~55 cycles, one-time cost.

global gdt_flush

section .text
bits 32

gdt_flush:
    ; Argument: pointer to GDTR structure (6 bytes: 2 limit + 4 base)
    mov eax, [esp + 4]

    ; Load the GDT register — single CPU instruction
    lgdt [eax]

    ; Reload data segment registers with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to reload code segment with kernel code selector (0x08)
    ; Required: lgdt does NOT update CS, only the GDTR cache.
    ; Without this, the next interrupt or exception would use stale CS.
    jmp 0x08:.flush
.flush:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits