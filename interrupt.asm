; Interrupt Service Routines — Nova kernel Phase 1
; Each stub pushes vector + error code and jumps to common handler.
;
; Stack layout on entry:
;   CPU pushed: EFLAGS, CS, EIP  (all interrupts)
;   CPU pushed: error code       (exceptions 8,10,11,12,13,14,17,30)
;   Our stub:   dummy 0          (if CPU didn't push EC)
;   Our stub:   vector number
;   Common:     pusha (8 regs)
;   Common:     push esp → C handler gets registers_t*

; ==================== Extern C Handlers ====================
extern exception_handler
extern irq_handler

; ==================== ISR Stub Macros ====================

; Stub for exceptions WITHOUT CPU error code
; Pushes dummy 0 (for uniform stack frame) then vector
%macro ISR_NOERR 1
global isr%1
isr%1:
    push byte 0
    push byte %1
    jmp isr_common
%endmacro

; Stub for exceptions WITH CPU error code
; CPU already pushed the error code; just push vector
%macro ISR_ERR 1
global isr%1
isr%1:
    push byte %1
    jmp isr_common
%endmacro

; ==================== Exception Stubs (Vectors 0-31) ====================
ISR_NOERR 0     ; #DE — Divide Error
ISR_NOERR 1     ; #DB — Debug
ISR_NOERR 2     ; #NMI — Non-maskable Interrupt
ISR_NOERR 3     ; #BP — Breakpoint
ISR_NOERR 4     ; #OF — Overflow
ISR_NOERR 5     ; #BR — Bound Range
ISR_NOERR 6     ; #UD — Invalid Opcode
ISR_NOERR 7     ; #NM — Device Not Available
ISR_ERR   8     ; #DF — Double Fault
ISR_NOERR 9     ; Coprocessor Segment Overrun (reserved on i686+)
ISR_ERR   10    ; #TS — Invalid TSS
ISR_ERR   11    ; #NP — Segment Not Present
ISR_ERR   12    ; #SS — Stack Segment Fault
ISR_ERR   13    ; #GP — General Protection Fault
ISR_ERR   14    ; #PF — Page Fault
ISR_NOERR 15    ; Reserved (x87 FPU error on old CPUs)
ISR_NOERR 16    ; #MF — x87 FPU Floating-Point Error
ISR_ERR   17    ; #AC — Alignment Check
ISR_NOERR 18    ; #MC — Machine Check
ISR_NOERR 19    ; #XM — SIMD Floating-Point
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30    ; #SX — Security Exception
ISR_NOERR 31

; ==================== Array of Exception Stub Pointers ====================
global isr_stubs
isr_stubs:
%assign i 0
%rep 32
    dd isr %+ i
%assign i i+1
%endrep

; ==================== Common Handler (Exceptions) ====================
section .text
bits 32

isr_common:
    pusha                           ; Save all general registers

    mov ax, 0x10                    ; Kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                        ; Pass pointer to registers_t
    call exception_handler
    add esp, 4                      ; Clean up argument

    popa                            ; Restore general registers
    add esp, 8                      ; Pop vector + error code
    iret                            ; Return: pops EIP, CS, EFLAGS

; ==================== IRQ Stubs (Vectors 32-47) ====================

%macro IRQ 2
global irq%1
irq%1:
    push byte 0
    push byte %2
    jmp irq_common
%endmacro

IRQ 0, 32   ; PIT Timer
IRQ 1, 33   ; PS/2 Keyboard
IRQ 2, 34   ; Cascade (slave PIC)
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy Disk
IRQ 7, 39   ; LPT1
IRQ 8, 40   ; CMOS RTC
IRQ 9, 41   ; Free (ACPI)
IRQ 10, 42  ; Free
IRQ 11, 43  ; Free
IRQ 12, 44  ; PS/2 Mouse
IRQ 13, 45  ; FPU / Coprocessor
IRQ 14, 46  ; Primary ATA
IRQ 15, 47  ; Secondary ATA

; ==================== Array of IRQ Stub Pointers ====================
global irq_stubs
irq_stubs:
%assign i 0
%rep 16
    dd irq %+ i
%assign i i+1
%endrep

; ==================== IRQ Common Handler ====================
irq_common:
    pusha

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    popa
    add esp, 8
    iret

; ==================== IDT Load ====================
global lidt_load
lidt_load:
    mov eax, [esp + 4]              ; Argument: pointer to IDTR
    lidt [eax]                      ; Load IDT register (~10 cycles, once)
    ret

section .note.GNU-stack noalloc noexec nowrite progbits