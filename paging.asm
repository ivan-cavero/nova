; Paging enable trampoline — Nova kernel Phase 2
; CR3 must be set before calling this.
; This function must be in a SEPARATE .asm file to ensure
; the instruction after 'mov cr0, PG-enable' is correctly
; fetched with the new paging mode.

global paging_enable

section .text
bits 32

paging_enable:
    ; Argument 1: physical address of page directory (CR3 value)
    mov eax, [esp + 4]

    ; Set CR3 = page directory physical address
    mov cr3, eax

    ; Enable Page Size Extension (CR4.PSE, bit 4) — REQUIRED for 4MB pages
    ; Without this, PDE.PS is ignored and 4MB pages don't work
    mov eax, cr4
    or eax, 0x10
    mov cr4, eax

    ; Read current CR0
    mov eax, cr0

    ; Set PG bit (bit 31)
    or eax, 0x80000000

    ; Write CR0 — paging ENABLED after this instruction
    mov cr0, eax

    ; Short jump to flush prefetch queue (required after CR0.PG change)
    jmp .flush
.flush:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits