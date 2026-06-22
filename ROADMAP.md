# Nova — ROADMAP

> **Elite Kernel Architecture & Development Plan**
> Performance-first, SMP-native, 64-bit only, GPU-accelerated
> Target: A modern OS that fully exploits all hardware capabilities

---
## Table of Contents

1. [Philosophy & Design Goals](#1-philosophy--design-goals)
2. [Architecture Overview](#2-architecture-overview)
3. [Phase 0 — Foundation (COMPLETE)](#3-phase-0--foundation)
4. [Phase 1 — Interrupt Infrastructure](#4-phase-1--interrupt-infrastructure)
5. [Phase 2 — Memory Management](#5-phase-2--memory-management)
6. [Phase 3 — Heap & Dynamic Allocation](#6-phase-3--heap--dynamic-allocation)
7. [Phase 4 — SMP & Multicore](#7-phase-4--smp--multicore)
8. [Phase 5 — Scheduler & Concurrency](#8-phase-5--scheduler--concurrency)
9. [Phase 6 — 64-bit Transition](#9-phase-6--64-bit-transition)
10. [Phase 7 — Userspace & Syscalls](#10-phase-7--userspace--syscalls)
11. [Phase 8 — Storage & Filesystems](#11-phase-8--storage--filesystems)
12. [Phase 9 — GPU Drivers](#12-phase-9--gpu-drivers)
13. [Phase 10 — Advanced Performance](#13-phase-10--advanced-performance)
14. [Performance Engineering Bible](#14-performance-engineering-bible)
15. [Build & CI Infrastructure](#15-build--ci-infrastructure)
16. [Testing & Validation Matrix](#16-testing--validation-matrix)
17. [Appendix: x86-64 Architecture Reference](#17-appendix-x86-64-architecture-reference)

---

## 1. Philosophy & Design Goals

### Core Tenets

```
PERFORMANCE   → Every cycle counts. No wasted instructions.
                Cache-aware data structures. Lock-free paths.
                Zero-cost abstractions. Predictable latency.

SCALABILITY   → Designed for 128+ cores from day one.
                Per-CPU data. NUMA-aware allocation.
                Wait-free IPC. Distributed interrupt handling.

SAFETY        → Memory safety through isolation, not luck.
                Strong typing at kernel interfaces.
                Formal verification of critical paths.
                No undefined behavior, ever.

MODERNITY     → 64-bit only. No legacy 16/32-bit fallbacks.
                UEFI native. ACPI 6.0+. PCIe 5.0+.
                AVX-512, AMX, APICv, SVM/VMX.
```

### Design Decisions Summary

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Bit-width** | 64-bit only | 32-bit is legacy. Full register file, >4GB RAM |
| **Bootloader** | Limine v12+ | Modern protocol, SMP-ready, native ELF64 |
| **Interrupt model** | x2APIC only | No PIC legacy. Lowest latency, MSI support |
| **Scheduling** | Per-CPU queues + work stealing | O(1) local, balanced global |
| **Memory map** | 4-level → 5-level paging | Forward-compatible with future hardware |
| **GPU** | Native kernel driver (not UEFI GOP) | Direct performance control, no abstraction overhead |
| **Drivers** | User-space drivers (UPD) | Crash isolation, hot-reload, no kernel rebuild |
| **Synchronization** | Lock-free first, spinlocks second | Wait-free paths for critical operations |

### Performance Targets

| Metric | Target | Method |
|--------|--------|--------|
| Interrupt latency | < 1 µs | APIC timer measurement |
| Context switch (same core) | < 2 µs | RDTSCP timestamps |
| Syscall overhead | < 100 ns | RDTSCP before/after |
| Page fault (TLB hit) | < 500 ns | Hardware PMC counters |
| kmalloc(64) | < 20 ns | RDTSC benchmark |
| IPC (same core, 64 bytes) | < 500 ns | Ring buffer latency test |
| Scheduler fairness | < 5% variance | Runtime measurement |
| GPU command submit | < 10 µs | GPU timestamp queries |
| Lock contention | < 1% CPU time | Lockstat profiling |
| Memory allocation (slab) | O(1), < 100 ns | Microbenchmark |

---

## 2. Architecture Overview

### Kernel Model: Hybrid Microkernel

```
                    USERSPACE
  ┌──────────────────────────────────────┐
  │  App A     App B     App C    ...   │
  └────┬─────────┬─────────┬───────────┘
       │         │         │
       ├─────────┴─────────┴───────────┤
       │       SYSTEM CALL LAYER        │
       ├───────────────────────────────┤
       │         KERNEL SPACE            │
  ┌────┴─────┐  ┌────┴─────┐             │
  │  PROCESS  │  │  MEMORY  │             │
  │  SCHED    │  │  PMM/VMM │             │
  │  IPC      │  │  SLAB    │             │
  └────┬─────┘  └────┬─────┘             │
  ┌────┴─────┐  ┌────┴─────┐             │
  │  DEVICES  │  │  VFS/FS  │             │
  │  PCI/ACPI │  │  FAT/EXT │             │
  │  GPU/NVMe │  │  BLOCK   │             │
  └────┬─────┘  └──────────┘             │
  ┌────┴─────┐                           │
  │  SMP/APIC │                           │
  │  CPU MGMT │                           │
  └──────────┘                           │
  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  │
  │ APIC │ │ HPET │ │ PCI  │ │ ACPI │  │
  └──────┘ └──────┘ └──────┘ └──────┘  │
└──────────────────────────────────────┘


---

## 3. Phase 0 — Foundation  ✅ COMPLETE

### Status: Nova v0.2 booting in QEMU

| Feature | Status | Notes |
|---------|--------|-------|
| GRUB multiboot2 boot | ✅ Done | boot.asm with multiboot2 header |
| 32-bit protected mode | ✅ Done | Code segment: 0x08, Data: 0x10 |
| VGA text mode (80×25) | ✅ Done | Colored tags, cursor hidden |
| Serial COM1 (115200 8N1) | ✅ Done | Debug output, no interrupts needed |
| GDT (5 entries) | ✅ Done | null, Kcode, Kdata, Ucode, Udata |
| Boot log system | ✅ Done | [OK] blue, [..] grey, [FAIL] red |
| Makefile build | ✅ Done | make run → QEMU |
| README + docs | ✅ Done | Windows/Linux setup |
| GitHub repo | ✅ Done | ivan-cavero/nova |

### Known Gaps (to fix in next phases)
- [ ] No interrupt handling → triple fault on IRQ
- [ ] No memory management (identity mapped)
- [ ] 32-bit only (RAM limit 4GB)
- [ ] Single core
- [ ] No heap (no kmalloc)
- [ ] VGA text only (no framebuffer rendering)


---

## 4. Phase 1 — Interrupt Infrastructure

**Difficulty: Medium | Est. LOC: 250 | Time: 8-12 hours**

### Objectives
- [ ] Install IDT with 256 entry vectors
- [ ] Handle CPU exceptions (#DE, #GP, #PF, #DF)
- [ ] Remap PIC (IRQ0-15 → INT 32-47)
- [ ] PIT timer at 1000 Hz
- [ ] PS/2 keyboard driver
- [ ] Boot log output for each milestone

### 4.1 IDT Structure
```c
typedef struct {
    uint16_t offset_lo;   // Handler address [15:0]
    uint16_t selector;    // Code segment (0x08)
    uint8_t  ist;         // Interrupt Stack Table offset
    uint8_t  flags;       // Present, DPL, gate type
    uint16_t offset_mid;  // Handler address [31:16]
    uint32_t offset_hi;   // Handler address [63:32]
    uint32_t reserved;    // Zero
} PACKED idt_entry_t;     // Total: 16 bytes per entry
```

### 4.2 Exception Handlers Checklist
- [ ] `exc_divide_error` — #DE (vector 0)
- [ ] `exc_debug` — #DB (v1)
- [ ] `exc_nmi` — #NMI (v2)
- [ ] `exc_breakpoint` — #BP (v3)
- [ ] `exc_overflow` — #OF (v4)
- [ ] `exc_bound_range` — #BR (v5)
- [ ] `exc_invalid_opcode` — #UD (v6)
- [ ] `exc_device_not_avail` — #NM (v7)
- [ ] `exc_double_fault` — #DF (v8, ERROR CODE)
- [ ] `exc_invalid_tss` — #TS (v10, EC)
- [ ] `exc_segment_not_present` — #NP (v11, EC)
- [ ] `exc_stack_segment` — #SS (v12, EC)
- [ ] `exc_general_protection` — #GP (v13, EC)
- [ ] `exc_page_fault` — #PF (v14, EC)
- [ ] `exc_floating_point` — #MF (v16)
- [ ] `exc_alignment_check` — #AC (v17, EC)
- [ ] `exc_machine_check` — #MC (v18)
- [ ] `exc_simd` — #XM (v19)

### 4.3 Error Code Decoding
Exception handlers with error codes push an extra value:
- **#PF error code**: bit 0 = P (present), bit 1 = W (write), bit 2 = U (user), bit 3 = RSVD, bit 4 = I (instruction fetch)
- **#GP error code**: segment selector index or 0
- **#DF error code**: always 0

### 4.4 PIC Remap
```c
outb(0x20, 0x11);  // ICW1: master, edge, cascade
outb(0xA0, 0x11);  // ICW1: slave
outb(0x21, 0x20);  // ICW2: master base = 32
outb(0xA1, 0x28);  // ICW2: slave base = 40
outb(0x21, 0x04);  // ICW3: slave connected to IRQ2
outb(0xA1, 0x02);  // ICW3: slave cascade
outb(0x21, 0x01);  // ICW4: x86 mode
outb(0xA1, 0x01);
outb(0x21, 0xFF);  // Mask all IRQ
outb(0xA1, 0xFF);
```

### 4.5 PIT Timer (1000 Hz)
```
Channel 0, mode 2 (rate generator)
Divisor = 1193182 / 1000 = 1193
outb(0x43, 0x36);           // CW: channel 0, lobyte/hibyte, mode 2
outb(0x40, 1193 & 0xFF);    // low byte
outb(0x40, (1193 >> 8) & 0xFF); // high byte
IRQ0 unmasked → handler fires 1000/sec
```

### 4.6 PS/2 Keyboard
- [ ] Read status from port 0x64 (bit 0 = output buffer full)
- [ ] Read scancode from port 0x60
- [ ] Convert scancode → ASCII via lookup table
- [ ] Circular buffer: `kbd_buffer[256]`, head/tail indices
- [ ] `kbd_getchar()`: wait for char
- [ ] `kbd_readline(buf, max)`: read until Enter

### Performance Benchmarks
| Metric | Method | Target |
|--------|--------|--------|
| IDT load → first interrupt | RDTSC in entry asm | < 50 cycles |
| Exception handler (no EC) | Trigger #BP, measure | < 100 cycles |
| PIT IRQ handler | 1000 ticks in 1s | ±10 ticks |
| Keypress latency | IRQ → char in buffer | < 50 µs |

### Success Criteria
```
[..]  Loading IDT at 0x1000...
[ OK]  IDT: 256 gates, handler at 0x100000
[..]  Remapping PIC...
[ OK]  PIC: master 32-39, slave 40-47
[..]  PIT timer at 1000 Hz...
[ OK]  PIT: divisor 1193, mode 2
[ OK]  Timer ticks: 1000/s
[..]  Enabling keyboard...
[ OK]  PS/2 keyboard: scancode set 1
[ OK]  IRQ1 mapped, kbd buffer 256 bytes
```


---

## 5. Phase 2 — Physical & Virtual Memory Manager

**Difficulty: Very Hard | Est. LOC: 700 | Time: 20-30 hours**

### Objectives
- [ ] Physical memory manager (PMM): bitmap/stack allocator
- [ ] 4-level page tables (PML4 → PDP → PD → PT)
- [ ] `vmm_map()` / `vmm_unmap()` / `vmm_get_phys()`
- [ ] Large pages (2MB, 1GB) for kernel and device DMA
- [ ] Page fault handler with error code decoding
- [ ] TLB management (flush on map/unmap, shootdown on SMP)

### Why 64-bit Paging?
Without paging, every memory access goes directly to RAM. Paging gives us:
- **Isolation**: each process has its own address space
- **Virtual memory**: can map more than physical RAM
- **Demand paging**: pages loaded on first access
- **Copy-on-write**: efficient fork()
- **Memory mapping**: files → virtual addresses

### 5.1 Physical Memory Manager

**Strategy**: Free-stack allocator (O(1) alloc/free, cache-hot)

```c
#define PAGE_SIZE 4096

typedef struct {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t *stack;      // Array of free page frame numbers
    uint64_t stack_top;   // Stack pointer
} pmm_t;

void *pmm_alloc(void) {
    if (pmm.stack_top == 0) return NULL;  // OOM
    uint64_t pfn = pmm.stack[--pmm.stack_top];
    pmm.free_pages--;
    return (void *)(pfn * PAGE_SIZE);
}

void pmm_free(void *addr) {
    uint64_t pfn = (uint64_t)addr / PAGE_SIZE;
    pmm.stack[pmm.stack_top++] = pfn;
    pmm.free_pages++;
}
```

**Checklist**:
- [ ] Parse GRUB memory map (e820 entries with type/addr/length)
- [ ] Mark reserved areas: kernel image, VGA memory, BIOS area
- [ ] Build free stack from usable regions
- [ ] `pmm_alloc()` returns zeroed page
- [ ] `pmm_free()` returns page to stack
- [ ] `pmm_alloc_contiguous(n)` for DMA buffers
- [ ] OOM tracking + recovery

### 5.2 4-Level Paging Setup

**Virtual address breakdown (48-bit, canonical)**:
```
47      39 38      30 29      21 20       12 11       0
┌─────────┬──────────┬──────────┬───────────┬──────────┐
│  PML4   │   PDP    │    PD    │    PT     │  OFFSET  │
│  (9b)   │   (9b)   │   (9b)   │   (9b)    │  (12b)   │
└─────────┴──────────┴──────────┴───────────┴──────────┘
 512 entries  512 ent    512 ent    512 ent    → 4096 bytes
```

**Checklist**:
- [ ] PML4 allocated and zeroed
- [ ] Recursive mapping: PML4[511] = phys_of_PML4
- [ ] Identity map: first 4MB (kernel entry before paging on)
- [ ] `vmm_map(phys, virt, flags, pt_level)`:
  - Walk PML4 → PDP → PD → PT (create tables as needed)
  - Set PTE flags (present, writable, user, nx, pat)
- [ ] `vmm_unmap(virt)`: clear PTE, invalidate TLB
- [ ] `vmm_get_phys(virt)`: walk tables, return PA

### 5.3 Large Pages (Performance!)

```c
// 2MB page: PD entry has PS=1 (page size)
// Benefits: half TLB entries → more coverage, faster walks
if (size >= 0x200000 && (virt & 0x1FFFFF) == 0) {
    // Use 2MB page: PT level skipped
    pd_entry = phys | PG_PRESENT | PG_WRITABLE | PG_PS;
}

// 1GB page: PDP entry has PS=1
if (size >= 0x40000000 && (virt & 0x3FFFFFFF) == 0) {
    // Use 1GB page: PD and PT skipped
    pdp_entry = phys | PG_PRESENT | PG_WRITABLE | PG_PS;
}
```

### Success Criteria
```
[..]  Initializing PMM...
[ OK]  PMM: 16320 free pages (63.75 MB)
[..]  Setting up 4-level paging...
[ OK]  Page tables: PML4 @ 0x7000
[ OK]  Recursive map: PML4[511] = 0x7000
[ OK]  Identity map: 0-4MB (kernel + stack)
[ OK]  Paging enabled (CR0.PG = 1)
[..]  Mapping 2MB page at 0x100000...
[ OK]  2MB page: kernel region (PD PS=1)
[ OK]  TLB flushed, all translations active
```


---

## 6. Phase 3 — Heap & Slab Allocator

**Difficulty: Medium-Hard | Est. LOC: 400 | Time: 10-15 hours**

### Objectives
- [ ] Slab allocator for sizes 16-4096 bytes
- [ ] Buddy allocator for page-sized allocations
- [ ] `kmalloc(size)` / `kfree(ptr)` / `krealloc(ptr, new_size)`
- [ ] Per-CPU slab caches for SMP (planned, not built yet)

### Slab Design
```c
typedef struct {
    void     *page;           // Backing page (phys)
    uint16_t  object_size;    // Size of each object
    uint16_t  objects_per_page;
    uint16_t  free_count;
    uint16_t  first_free_idx; // Index of first free object
    struct slab *next;        // Next slab in list
} slab_t;

// Free list is stored WITHIN freed objects (no extra memory)
void *slab_alloc(slab_t *s) {
    void *obj = (void *)s->page + s->first_free_idx * s->object_size;
    s->first_free_idx = *(uint16_t *)obj;  // Read next free index from the object itself
    s->free_count--;
    return obj;
}
```

### Slab Sizes
| Index | Size | Objects per 4KB page | Waste |
|-------|------|---------------------|-------|
| 0 | 16 | 256 | 0 |
| 1 | 32 | 128 | 0 |
| 2 | 64 | 64 | 0 |
| 3 | 128 | 32 | 0 |
| 4 | 256 | 16 | 0 |
| 5 | 512 | 8 | 0 |
| 6 | 1024 | 4 | 0 |
| 7 | 2048 | 2 | 0 |
| 8 | 4096 | 1 | 0 |

### Performance Targets
| Operation | Cycles | Target ns (3 GHz) |
|-----------|--------|-------------------|
| kmalloc(64) | < 60 | < 20 ns |
| kfree(ptr) | < 30 | < 10 ns |
| kmalloc(3000) → buddy | < 100 | < 33 ns |
| 100k alloc-free cycle | < 10M | < 3.3 ms |

---

## 7. Phase 4 — SMP & Multicore Startup

**Difficulty: Very Hard | Est. LOC: 800 | Time: 25-35 hours**

### 7.1 CPU Discovery via ACPI MADT
```c
// MADT (Multiple APIC Description Table) header
typedef struct {
    char     signature[4];   // "APIC"
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    // ... followed by variable-length entries (each 8 bytes):
    // Entry type 0: Local APIC (CPU)
    //   ACPI processor ID, APIC ID, flags
    // Entry type 1: I/O APIC
    //   APIC ID, base address, global system interrupt base
    // Entry type 2: Interrupt source override
} PACKED madt_t;
```

### 7.2 AP Startup Protocol
```
BSP                              AP (waiting in real mode)
 │                                    │
 ├─ Read ACPI MADT                    │
 ├─ Build CPU list                    │
 ├─ Allocate AP stacks                │
 ├─ Write trampoline to 0x8000        │
 │                                    │
 ├─ Send INIT IPI ─────────────────►   CPU reset
 │         (ICR: 0x000C4500)        │
 │         wait 10ms                 │
 │                                    │
 ├─ Send SIPI ────────────────────►   Jump to trampoline
 │    (ICR: 0x000C4608, vec=0x08)  │   │
 │         wait 200µs               │   ├── Set GDT (lgdt)
 │                                    │   ├── Set CR0 (PE=1)
 │    Check if AP online             │   ├── Far jump: 32-bit
 │                                    │   ├── Load page tables (CR3)
 │                                    │   ├── Enable paging (CR0.PG=1)
 │                                    │   ├── Write EFER (LME=1)
 │                                    │   ├── Far jump: 64-bit
 │                                    │   ├── Set GS base (per-CPU)
 │                                    │   ├── Call ap_main()
 │                                    │   │
 │                                    │   ├── Init local APIC
 │                               ──►  │   ├── "CPU N online"
 │    Receive IPI confirmation       │   ├── Enable interrupts
 │    CPU marked ONLINE              │   └── Idle loop (hlt)
```

### 7.3 Per-CPU Structure
```c
typedef struct {
    uint32_t apic_id;         // Local APIC ID
    uint32_t cpu_index;       // 0 = BSP, 1+ = APs
    void    *stack_base;      // Top of kernel stack
    void    *idle_task;       // Idle task for this CPU
    uint64_t local_ticks;     // APIC timer count
    uint64_t ipi_count;       // IPIs received
    
    // Scheduler
    runqueue_t  *runqueue;
    task_t      *current_task;
    
    // Memory
    slab_cache_t slab[9];     // Per-CPU slab partials
    uint64_t allocs_local;    // Allocations from local node
} PACKED cpu_t;

// Access current CPU via GS segment (RDGSBASE in x86-64)
static inline cpu_t *this_cpu(void) {
    cpu_t *cpu;
    __asm__("mov %%gs:0, %0" : "=r"(cpu));
    return cpu;
}
```

### 7.4 Lock-Free Primitives (Performance!)
```c
// x86 atomic operations — single instruction, no lock needed
#define atomic_inc(p)   __sync_fetch_and_add(p, 1)
#define atomic_dec(p)   __sync_fetch_and_sub(p, 1)
#define atomic_add(p,v) __sync_fetch_and_add(p, v)
#define atomic_cas(p,o,n) __sync_bool_compare_and_swap(p, o, n)
#define atomic_xchg(p,v) __sync_lock_test_and_set(p, v)

// Spinlock — 2 instructions
typedef int spinlock_t;
#define spin_lock(l)   while (atomic_xchg(l, 1)) { cpu_pause(); }
#define spin_unlock(l) atomic_xchg(l, 0)
```

### Checklist
- [ ] ACPI RSDP → RSDT/XSDT → MADT parsing
- [ ] Local APIC detection + enable
- [ ] I/O APIC enumeration
- [ ] AP trampoline assembly (real → 32 → 64 bit)
- [ ] All APs start and report online
- [ ] `this_cpu()` returns correct pointer
- [ ] IPI send + receive (test with "ping" each CPU)
- [ ] Per-CPU data: slab, runqueue, idle task

### Success Criteria
```
[..]  Enumerating CPUs...
[ OK]  MADT: 4 CPU entries found
[..]  Starting AP 1 (APIC ID 2)...
[ OK]  CPU 1 online (APIC ID 2)
[..]  Starting AP 2 (APIC ID 3)...
[ OK]  CPU 2 online (APIC ID 3)
[..]  Starting AP 3 (APIC ID 4)...
[ OK]  CPU 3 online (APIC ID 4)
[ OK]  4 CPUs active: BSP @ APIC 0 + 3 APs
[ OK]  IPI broadcast: 3/3 responses in 12 µs
```

---

## 8. Phase 5 — Scheduler & Concurrency

**Difficulty: Hard | Est. LOC: 700 | Time: 15-25 hours**

### Objectives
- [ ] Task control block (TCB) with state machine
- [ ] Context switch in 64-bit assembly (push/pop regs)
- [ ] Per-CPU run queues (O(1) enqueue/dequeue)
- [ ] Priority levels (0-255, real-time to idle)
- [ ] Work stealing from neighbor CPUs
- [ ] Tickless idle (HLT, wake via IPI)

### Scheduler Architecture
```
CPU 0                    CPU 1                    CPU 2
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│ Runqueue     │    │ Runqueue     │    │ Runqueue     │
│ [0]: ── RT   │    │ [0]:         │    │ [0]: ── RT   │
│ [1]: ──────  │    │ [1]: ──────  │    │ [1]:         │
│ [2]: ──────  │    │ [2]: ── N    │    │ [2]: ──────  │
│ [3]: ── B    │    │ [3]: ── B    │    │ [3]: ──── I  │
│ Idle (hlt)   │    │ Idle (hlt)   │    │ Idle (hlt)   │
└──────┬───────┘    └──────┬───────┘    └──────┬───────┘
       │                   │                   │
       └───────────────────┴───────────────────┘
             Work stealing: idle CPU steals from
             busiest neighbor (round-robin)
```

### Context Switch (assembly)
```asm
switch_to:
    ; Save current task registers
    mov [rdi + TASK_RAX], rax
    mov [rdi + TASK_RBX], rbx
    mov [rdi + TASK_RCX], rcx
    ; ... save all GPRs + XMM registers ...
    mov [rdi + TASK_RSP], rsp
    mov [rdi + TASK_RBP], rbp
    
    ; Restore next task registers
    mov rsp, [rsi + TASK_RSP]
    mov rax, [rsi + TASK_RAX]
    mov rbx, [rsi + TASK_RBX]
    ; ... restore all GPRs ...
    
    ; Restore CR3 (switch address space if different)
    mov rax, [rsi + TASK_CR3]
    mov cr3, rax
    
    ; Return to where next task was preempted
    ret
```

### Priority & Timeslice
| Priority | Class | Timeslice | Boost |
|----------|-------|-----------|-------|
| 0-31 | Real-time | 100ms | Never expires |
| 32-63 | Interactive | 30ms | +20 on wake |
| 64-127 | Normal | 20ms | +10 on wake |
| 128-191 | Background | 10ms | None |
| 192-255 | Idle | 1ms | None |

### Performance Targets
| Operation | Same CPU | Different CPU |
|-----------|----------|---------------|
| Context switch | < 1 µs | < 3 µs |
| Enqueue task | O(1) | O(1) |
| Dequeue (schedule) | O(1) bitmap scan | O(1) |
| Work steal | N/A | < 5 µs |
| Wake from idle (IPI) | N/A | < 2 µs |


---

## 9. Phase 6 — 64-bit Long Mode Migration

**Difficulty: Medium | Est. LOC: 200 | Time: 5-10 hours**

### Why 64-bit?
- **Registers**: 16 GPRs (vs 8), each 64-bit (vs 32)
- **Memory**: >4GB RAM (required for GPU VRAM mapping)
- **Features**: SYSCALL/SYSRET, RDTSCP, INVPCID, RDFSGSBASE
- **OS support**: All modern apps expect 64-bit
- **Hardware**: Every x86 CPU sold since ~2010 is 64-bit

### Approach: Switch to Limine Bootloader
Limine loads the kernel directly in 64-bit long mode. No assembly trampoline needed.

- [ ] Add Limine header to boot.asm (`.limine_reqs` section)
- [ ] Link ELF64 binary (`OUTPUT_FORMAT(elf64-x86-64)`)
- [ ] Replace GRUB config with Limine config (`limine.cfg`)
- [ ] Kernel entry point: 64-bit, called in long mode

### What changes in code
| Component | 32-bit | 64-bit |
|-----------|--------|--------|
| Makefile | `-m32` → `gcc -m64` | `-m64` with `-mno-red-zone` |
| Pointers | uint32_t (4GB limit) | uint64_t |
| Calling convention | cdecl (stack args) | System V AMD64 (rdi, rsi, rdx...) |
| Syscall | INT 0x80 (slow) | SYSCALL/SYSRET (fast: ~50ns) |
| GDT entries | 32-bit code (D=1) | 64-bit code (L=1) |
| Stack | 4KB default | 16KB default |
| VGA access | Same (0xB8000) | Same (0xB8000) |

### Bootstrap Changes
```asm
; boot.asm (Limine protocol, 64-bit)
bits 64
section .text
global _start
_start:
    mov rsp, stack_top
    cld
    ; rdi = bootloader magic
    ; rsi = bootloader response
    call kernel_main_64
```

### Toolchain
```makefile
CC      = gcc
CFLAGS  = -ffreestanding -nostdlib -m64 -mno-red-zone \
          -march=x86-64 -O3 -I.
LDFLAGS = -m elf_x86_64 -T linker.ld -nostdlib
ASFLAGS = -f elf64
```

---

## 10. Phase 7 — Userspace & System Calls

**Difficulty: Very Hard | Est. LOC: 900 | Time: 25-40 hours**

### 10.1 SYSCALL/SYSRET (Fast Path)
```asm
; Kernel entry for syscall (MSR LSTAR)
syscall_entry:
    swapgs                    ; GS ← kernel per-CPU
    mov gs:[CPU_USER_RSP], rsp  ; Save user RSP
    mov rsp, gs:[CPU_KERN_RSP]  ; Load kernel stack
    push gs:[CPU_USER_SS]
    push gs:[CPU_USER_RSP]
    push r11                   ; RFLAGS (saved by SYSCALL)
    push r10                   ; RIP (saved by SYSCALL)
    push rax                   ; Syscall number
    ; ... save remaining registers ...
    
    ; Dispatch
    cmp rax, MAX_SYSCALL
    ja  bad_syscall
    call [syscall_table + rax*8]
    
    ; Return
    mov rsp, gs:[CPU_USER_RSP]
    swapgs
    sysretq                    ; Fast return to user
; Total: ~60-80 ns
```

### 10.2 System Calls (Planned)
```
#   Name        Description
0   sys_log     Write to kernel log
1   sys_exit    Terminate process
2   sys_write   Write to fd
3   sys_read    Read from fd
4   sys_open    Open file/device
5   sys_close   Close fd
6   sys_mmap    Map memory
7   sys_sbrk    Extend heap
8   sys_fork    Create child process
9   sys_execve  Execute binary
10  sys_waitpid Wait for child
11  sys_getpid  Get process ID
12  sys_nanosleep Sleep for µs
13  sys_sched_yield Yield CPU
14  sys_kill    Send signal
15  sys_mkdir   Create directory
```

### 10.3 ELF Binary Loader
- [ ] Parse ELF64 header: magic (0x7F 'ELF'), type (EXEC/DYN), entry
- [ ] Parse Program Headers: PT_LOAD, PT_INTERP, PT_DYNAMIC
- [ ] Allocate memory for each PT_LOAD segment
- [ ] Copy segment data from binary
- [ ] Zero BSS sections
- [ ] Set up initial stack (argc, argv, envp)
- [ ] Jump to entry point in ring 3 via IRETQ

```c
typedef struct {
    unsigned char e_ident[16];  // Magic + class + data + version
    uint16_t e_type;            // ET_EXEC = 2, ET_DYN = 3
    uint16_t e_machine;         // EM_X86_64 = 62
    uint32_t e_version;
    uint64_t e_entry;           // Entry point virtual address
    uint64_t e_phoff;           // Program header offset
    uint64_t e_shoff;           // Section header offset
    uint32_t e_flags;
    uint16_t e_ehsize;          // ELF header size
    uint16_t e_phentsize;       // Program header entry size
    uint16_t e_phnum;           // Program header count
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} PACKED elf64_header_t;
```

---

## 11. Phase 8 — Storage & Filesystems

**Difficulty: Hard | Est. LOC: 1200 | Time: 30-45 hours**

### 11.1 ATA PIO Driver
- [ ] Detect ATA drives on primary/secondary bus
- [ ] Read identify data (model, LBA support, sectors)
- [ ] `ata_read(lba, count, buffer)` — 28-bit LBA
- [ ] `ata_write(lba, count, buffer)`

### 11.2 FAT32 Filesystem
- [ ] Parse MBR: find partition type 0x0B/0x0C (FAT32)
- [ ] Parse FAT32 BPB: sector size, cluster size, FAT size
- [ ] Walk directory tree: root → subdirectories
- [ ] Read file by cluster chain
- [ ] Cache: buffer recently accessed sectors

### 11.3 VFS Layer
```c
typedef struct vfs_node {
    char     name[256];
    uint32_t flags;       // File, directory, device
    uint64_t size;
    struct vfs_node *parent;
    struct vfs_node *children;   // Directory listing
    
    // Operations (filled by filesystem driver)
    uint64_t (*read)(struct vfs_node *, uint64_t off, void *buf, uint64_t n);
    uint64_t (*write)(struct vfs_node *, uint64_t off, void *buf, uint64_t n);
    void     (*close)(struct vfs_node *);
    struct vfs_node *(*finddir)(struct vfs_node *, const char *name);
} vfs_node_t;
```


---

## 12. Phase 9 — GPU & Graphics Drivers

**Difficulty: EXTREMELY Hard | Est. LOC: 3000+ | Time: 100+ hours**

### Why Native GPU Support?
Most hobby OS use VGA text mode or UEFI GOP framebuffer. Nova will **control the GPU directly** for maximum performance and modern features.

### GPU Abstraction Layer
```c
typedef struct gpu_device {
    char name[64];
    uint16_t vendor_id;     // 0x10DE NVIDIA, 0x1002 AMD, 0x8086 Intel
    uint16_t device_id;
    
    // BARs (PCI Base Address Registers)
    uint64_t bar0;          // MMIO (register access)
    uint64_t bar1;          // VRAM (framebuffer, textures)
    uint64_t bar2;          // Additional (NVIDIA: port, AMD: doorbell)
    uint64_t vram_size;
    
    // Operations
    int (*init)(struct gpu_device *self);
    int (*set_mode)(struct gpu_device *self, int width, int height, int bpp);
    int (*present)(struct gpu_device *self, struct gpu_buffer *buf);
    int (*submit)(struct gpu_device *self, struct gpu_command *cmd);
    
    // Memory management
    struct gpu_bo *(*alloc_bo)(struct gpu_device *self, size_t size);
    void (*free_bo)(struct gpu_device *self, struct gpu_bo *bo);
    
    // Interrupts
    void (*irq_handler)(struct gpu_device *self);
} gpu_device_t;

typedef struct gpu_buffer {
    void    *vram_addr;     // GPU-visible address
    void    *cpu_addr;      // CPU-mapped address (via WC)
    size_t   size;
    uint64_t pitch;         // Bytes per row
    int      width, height;
} gpu_buffer_t;
```

### 12.1 PCI GPU Discovery
- [ ] Scan PCI bus 0-255 for VGA controller (class 0x03, subclass 0x00/0x01)
- [ ] Read Vendor ID:
  - 0x10DE = NVIDIA Corporation
  - 0x1002 = Advanced Micro Devices (AMD)
  - 0x8086 = Intel Corporation
- [ ] Enable MMIO + Bus Master in PCI command register
- [ ] Map BAR0 (register MMIO) and BAR1/2 (VRAM) in kernel address space

### 12.2 Simple Framebuffer (Must Work First)
```c
// Get framebuffer from Limine bootloader
limine_framebuffer *fb = limine_get_framebuffer();

// Map to kernel space
uint32_t *fb_ptr = vmm_map(fb->address, fb->address, 
                           fb->pitch * fb->height, PG_WC);

// Draw pixel
void gpu_pixel(int x, int y, uint32_t rgba) {
    uint32_t *p = &fb_ptr[y * (fb->pitch/4) + x];
    *p = rgba;
}

// Draw rect (batch for performance)
void gpu_rect(int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            fb_ptr[row * (fb->pitch/4) + col] = color;
    // OPTIMIZE: Use SSE/AVX for row fills
}
```

**Checklist**:
- [ ] Limine framebuffer info: address, pitch, bpp, size
- [ ] Map framebuffer with write-combining (PAT: Page Attribute Table)
- [ ] `gpu_pixel(x, y, color)` — single pixel
- [ ] `gpu_rect(x, y, w, h, color)` — filled rectangle
- [ ] `gpu_bitblt(src, dst)` — blit with alpha
- [ ] Double buffer: draw to back, `gpu_present()` flips
- [ ] Text rendering: bitmap font (8×16 per char)

### 12.3 Advanced 2D (Post-Framebuffer)
- [ ] **Hardware cursor** (via GPU cursor register if available)
- [ ] **Hardware acceleration**: GPU-specific 2D blit engine
- [ ] **Multi-monitor**: detect connectors (VGA, DVI, HDMI, DP)
- [ ] **VSync**: synchronize with display refresh
- [ ] **Gamma correction**: lookup table for color accuracy

### 12.4 NVIDIA Driver Architecture
```
┌──────────────────────────────────────────────────┐
│                  Nova Kernel                       │
│  ┌────────────────────────────────────────────┐   │
│  │           NVIDIA GPU Driver                 │   │
│  │                                            │   │
│  │  1. PCI Discovery + BAR mapping            │   │
│  │  2. GSP firmware loading (nvidia-gsp.bin)  │   │
│  │  3. GPU System Processor initialization     │   │
│  │  4. Channel creation + command submission   │   │
│  │  5. Memory management (VRAM allocator)       │   │
│  │  6. Display controller (head, mode, vblank) │   │
│  │  7. Interrupt handling (MSI)                │   │
│  └────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────┘
```

**NVIDIA-specific challenges**:
1. **Firmware**: NVIDIA GPUs from Turing (RTX 2000, 2018) require GSP firmware. The open-source `nvidia-gsp.bin` must be loaded.
2. **Documentation**: NVIDIA has partial public docs. Reverse-engineering from the open kernel module (nvidia-open, released 2022, mature by 2026) is the path.
3. **NVF0/NVF1 registers**: Different GPU families have different MMIO layouts.

**Implementation plan**:
```
├── gpu/nvidia/nvidia.h        # NVIDIA-specific constants
├── gpu/nvidia/nvidia_pci.c    # PCI discovery + BARs + init
├── gpu/nvidia/nvidia_gsp.c    # GSP firmware loading + RM API
├── gpu/nvidia/nvidia_fb.c     # Framebuffer + mode setting
├── gpu/nvidia/nvidia_mem.c    # VRAM allocator
└── gpu/nvidia/nvidia_cmd.c    # Channel + command submission
```

### 12.5 AMD Driver Architecture
```c
// AMD has the best open documentation
// Registers via BAR0 MMIO, command via PM4 packet format

// PM4 packet (AMD command format)
typedef struct {
    uint32_t header;    // Type + count
    uint32_t body[];
} pm4_packet_t;

// Example: draw command
pm4_packet_t pkt = {
    .header = PM4_TYPE3 | PM4_IT_OP(IT_DRAW_INDEX_AUTO) | (3 << 16),
    .body[0] = num_vertices,
    .body[1] = VGT_DI_PRIMTYPE_TRIANGLE,
    .body[2] = 0,   // flags
};
```

### 12.6 Intel Driver Architecture
Intel i915 is fully open-source. Gen9+ (Skylake, 2015) through Xe (Arc, 2022) are well-documented.

```
GPU Engine        Command Streamer
    │                   │
    ├── Render (3D)     Ring buffer (legacy) or GuC (modern)
    ├── Blitter (2D)    └─── Submit commands → GPU executes
    ├── Video (VDEnc)
    └── Compute (CS)
```

### Performance Targets
| Metric | Framebuffer | Hardware 2D | 3D |
|--------|-------------|-------------|-----|
| Pixel fill | 100M px/s | 1B px/s | 10B+ px/s |
| Rect fill (1920×1080) | 8 ms | 0.1 ms | 0.01 ms |
| Present (60Hz) | 16.6ms budget | VSync | VSync |
| GPU submit latency | N/A | < 100 µs | < 10 µs |

---

## 13. Phase 10 — Advanced Performance Features

### 13.1 Lock-Free Data Structures
- [ ] Lock-free SPSC ring buffer (for inter-core IPC)
- [ ] Lock-free MPSC queue (for interrupt → task handoff)
- [ ] Lock-free MPMC queue (full Michael-Scott)
- [ ] RCU (Read-Copy-Update): O(1) reads, deferred frees

### 13.2 Cache Optimization
```c
// Align hot data to cache lines (64 bytes)
#define cache_aligned __attribute__((aligned(64)))

// Avoid false sharing: never put hot globals on same cache line
uint64_t cache_aligned cpu0_counter;
uint64_t cache_aligned cpu1_counter;

// Prefetch
__builtin_prefetch(ptr, 0, 3);    // Read, high temporal
__builtin_prefetch(next_ptr, 0, 0); // Read, non-temporal

// Non-temporal stores (bypass cache for large writes)
_mm_stream_si64((long long *)addr, value);
```

### 13.3 PMU (Performance Monitoring Counters)
```c
// Enable PMC for kernel-wide profiling
uint64_t cycles, instructions, cache_misses;

// Read fixed-purpose counters
cycles = __rdpmc(0);          // Fixed CTLC0: cycles
instructions = __rdpmc(1);   // Fixed CTLC1: instructions retired
cache_misses = __rdpmc(2);   // Fixed CTLC2: LLC misses
```

### 13.4 SIMD in Kernel
- [ ] CR4.OSFXSR set → SSE/SSE2 available
- [ ] `memset_sse(void *dst, int c, size_t n)` — 16 bytes at a time
- [ ] `memcpy_avx(void *dst, void *src, size_t n)` — 32 bytes at a time
- [ ] Save/restore XMM regs in context switch (fxsave64/fxrstor64)

### 13.5 NUMA-Aware Policies
- [ ] Allocate memory on local node first
- [ ] Schedule tasks on home node (migrate if imbalance)
- [ ] Interrupt affinity: IRQ → closest core


---

## 14. Performance Engineering Bible

### Hot Path Rules
```
1. INTERRUPT HANDLERS  (< 100 cycles)
   - No memory allocation
   - No function calls (inline everything)
   - No locks (use lock-free queues for handoff)
   - Mark hot: __attribute__((hot))
   - Prefetch the handler data

2. SYSCALLS  (< 500 cycles)
   - Copy minimal data to/from user
   - Validate arguments early (fail fast)
   - Prefer register ABI (System V AMD64)
   - No double-fetch from user memory (TOCTOU!)

3. SCHEDULER  (< 200 cycles, timer tick)
   - O(1) bitmap scan for next task
   - Per-CPU data (no cross-core access)
   - Batch TLB invalidations
```

### Critical Path Profile
```
Component                  Max budget    Instrumentation
──────────────────────────────────────────────────────────
Syscall entry              30 ns         RDTSCP
Argument validation        20 ns
Dispatch + handler body    100-500 ns    varies
Copy to user               5-50 ns       varies by size
Syscall return             20 ns
Total:                     ~200-600 ns

Scheduler tick             200 ns        RDTSC
Pick next task             100 ns        bitmap scan
Context switch (save)      100 ns        push regs
Context switch (restore)   100 ns        pop regs
TLB flush (if CR3 change)   20 ns        mov cr3
Total:                      ~520 ns

Interrupt entry            50 ns        push + jump
Handler body               50-1000 ns    varies
EOI                        10 ns         write APIC
Total:                      ~100-1100 ns
```

### Compiler Optimization Flags
```makefile
# Maximum performance
CFLAGS += -O3 -flto -march=native
CFLAGS += -fomit-frame-pointer -fno-stack-protector
CFLAGS += -mno-red-zone -fno-exceptions
CFLAGS += -mavx2 -mbmi2 -mlzcnt -mpopcnt

# Safety flags (debug build)
CFLAGS_DEBUG += -O0 -g -fsanitize=undefined
CFLAGS_DEBUG += -fstack-protector-strong
CFLAGS_DEBUG += -DDEBUG -DVERBOSE_LOG
```

---

## 15. Build & CI Infrastructure

### Cross-Compiler Build (Production)
```bash
# x86_64-elf-gcc (needed for proper freestanding kernel)
git clone https://github.com/misterfoxy/osdev-cross
cd osdev-cross && ./build.sh
# Installed at /usr/local/cross/bin/x86_64-elf-gcc
```

### CI Pipeline (GitHub Actions)
```yaml
name: Nova Build & Test
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt install build-essential nasm make xorriso grub-pc-bin qemu-system-x86
      - run: make clean && make
      - run: make iso
      - name: Boot test
        run: timeout 10 qemu-system-x86_64 -cdrom myKernel.iso -serial stdio -display none 2>&1 | tee serial.log
      - run: grep "System ready" serial.log
      - run: grep "\[ OK \]" serial.log
```

### Common Commands
```bash
make          # Build kernel
make run      # Build + QEMU
make debug    # Build + QEMU + GDB
make iso      # Generate ISO
make clean    # Remove build artifacts

# Advanced QEMU
make run SMP=4     # Test SMP on 4 cores
make run MEM=4G    # Test with 4GB RAM
make run KVM=1     # KVM acceleration (Linux only)
```

---

## 16. Testing & Validation Matrix

### Automated Tests
| Component | Test Case | Validation |
|-----------|-----------|------------|
| Boot | Serial output check | "[ OK ]" markers all present |
| IDT | Trigger int 0x00 via inline | #DE handler called |
| PIC | Read ISR after IRQ | Correct vector returned |
| PIT | Count 1000 ticks | Elapsed = 1.0s ± 10ms |
| Keyboard | Inject scancode via QMP | Char appears in buffer |
| PMM | alloc/free 10k pages | No leak, OOM works |
| VMM | map, unmap, get_phys | Virtual ↔ Physical correct |
| Slab | kmalloc/kfree cycle | No corruption |
| SMP | All CPUs reply to IPI | N/4 online messages |
| Scheduler | 10 threads round-robin | Each runs ~equal time |
| Framebuffer | Fill screen with color | Pixels match expected |

### Stress Tests
```bash
# Burn-in: 1000 boot cycles
for i in $(seq 1 1000); do
    timeout 5 qemu-system-x86_64 -cdrom nova.iso -display none -serial file:serial.log
    if ! grep -q "System ready" serial.log; then
        echo "FAIL at boot $i"
        exit 1
    fi
done

# Memory pressure
qemu-system-x86_64 -cdrom nova.iso -m 128M -serial stdio

# CPU stress (128 cores)
qemu-system-x86_64 -cdrom nova.iso -smp 128 -serial stdio -display none

# GPU stress (framebuffer fill)
qemu-system-x86_64 -cdrom nova.iso -vga std -serial stdio
```

---

## 17. Appendix: x86-64 Architecture Reference

### Important MSRs
| MSR | Address | Bits | Function |
|-----|---------|------|----------|
| IA32_APIC_BASE | 0x1B | [11] APIC enable, [8] x2APIC | Local APIC location |
| IA32_EFER | 0xC0000080 | [8] LME, [0] SCE | Long mode + syscall enable |
| STAR | 0xC0000081 | [47:32] kernel CS | SYSCALL CS/SS base |
| LSTAR | 0xC0000082 | Full 64-bit | SYSCALL entry RIP |
| SF_MASK | 0xC0000084 | [9] IF mask | Disable IRQ on syscall |
| KERNEL_GS_BASE | 0xC0000102 | Full 64-bit | Per-CPU struct base |

### Page Fault Error Code
```
Bit 0 (P)   : 0 = non-present page, 1 = protection violation
Bit 1 (W)   : 0 = read, 1 = write
Bit 2 (U)   : 0 = kernel mode, 1 = user mode
Bit 3 (RSVD): Reserved bit violation in paging structure
Bit 4 (ID)  : Instruction fetch (NX violation)
Bit 5 (PK)  : Protection key violation
Bit 6 (SS)  : Shadow stack access
```

### Interrupt Vector Layout
```
0-19     CPU Exceptions (divide, GPF, page fault, etc.)
20-31    Reserved
32-47    ISA IRQs (after PIC remap)
48-255   Free: IPIs, software interrupts, MSI
  48-63   IPI vectors (TLB shootdown, reschedule, etc.)
  64-255  Device MSI/MSI-X / user-defined
```

### Memory Map (QEMU default, 64MB)
```
0x00000000 - 0x000004FF   Real Mode IVT + BDA
0x00000500 - 0x00007BFF   BIOS data area
0x00007C00 - 0x00007DFF   Boot sector (GRUB stage 1)
0x00007E00 - 0x0007FFFF   GRUB stage 1.5 + 2
0x00080000 - 0x0009FBFF   Available (kernel stack, etc.)
0x0009FC00 - 0x0009FFFF   EBDA (Extended BIOS Data Area)
0x000A0000 - 0x000BFFFF   VGA video memory
0x000B8000 - 0x000BFFFF   VGA text mode buffer
0x000E0000 - 0x000FFFFF   BIOS ROM
0x00100000 - 0x03FFFFFF   System RAM (kernel loaded at 1MB)
```

---

## Master Checklist: All Phases

**Phase 1 — IDT + Timer + Keyboard**
- [ ] IDT loaded at 0x1000 (256 × 16 bytes = 4KB)
- [ ] All 32 exception handlers installed
- [ ] PIC remapped: master 32-39, slave 40-47
- [ ] PIT at 1000 Hz, ticks counter
- [ ] Keyboard driver: scancode → ASCII, circular buffer
- [ ] Boot log shows all milestones

**Phase 2 — Memory Management**
- [ ] PMM: free-stack allocator (O(1))
- [ ] 4-level page tables (PML4 → PDP → PD → PT)
- [ ] vmm_map / vmm_unmap / vmm_get_phys
- [ ] 2MB large pages for kernel
- [ ] Recursive page table mapping

**Phase 3 — Heap**
- [ ] Slab allocator (sizes 16-4096)
- [ ] Buddy allocator for > 4KB
- [ ] kmalloc / kfree
- [ ] Per-CPU slab partials

**Phase 4 — SMP**
- [ ] ACPI MADT parsed
- [ ] AP startup via SIPI
- [ ] All CPUs online
- [ ] Per-CPU data via GS segment
- [ ] Spinlocks, mutexes

**Phase 5 — Scheduler**
- [ ] Task control block
- [ ] Context switch (64-bit asm)
- [ ] Per-CPU run queues
- [ ] Priority levels (0-255)
- [ ] Work stealing
- [ ] Idle task (HLT)

**Phase 6 — 64-bit**
- [ ] Limine boot protocol
- [ ] ELF64 binary
- [ ] Full 64-bit address space
- [ ] 64-bit GDT + segments
- [ ] SYSCALL/SYSRET ready

**Phase 7 — Userspace**
- [ ] System call table (= 16 handlers)
- [ ] ELF64 loader
- [ ] Ring 3 processes
- [ ] fork/exec/wait

**Phase 8 — Filesystem**
- [ ] ATA PIO driver
- [ ] FAT32 read/write
- [ ] VFS layer
- [ ] Page cache

**Phase 9 — GPU**
- [ ] Framebuffer (from Limine)
- [ ] Pixel/rect/text drawing
- [ ] Double buffering + VSync
- [ ] PCI GPU discovery
- [ ] NVIDIA: GSP init + basic commands
- [ ] AMD: PM4 packet submission
- [ ] Intel: Ring buffer submission
- [ ] Multi-monitor support

**Phase 10 — Advanced Performance**
- [ ] Lock-free SPSC ring buffer
- [ ] RCU for read-mostly data
- [ ] Perf counters (PMC)
- [ ] SIMD memcpy/memset
- [ ] NUMA-aware allocation
- [ ] Cache-line aligned hot data

---

## Timeline & Effort

| Phase | Est. Lines | Est. Hours | Knowledge Prerequisites |
|-------|-----------|-----------|------------------------|
| P0 (✅) | 500 | Done | Done |
| P1 IDT | 250 | 8-12 | C, ASM, x86 arch |
| P2 MMU | 700 | 20-30 | Paging, page tables |
| P3 Heap | 400 | 10-15 | Paging (solid) |
| P4 SMP | 800 | 25-35 | ACPI, APIC, IPIs |
| P5 Sched | 700 | 15-25 | Context switch, queues |
| P6 64-bit | 200 | 5-10 | x86-64, Limine |
| P7 Userspace | 900 | 25-40 | ELF, ring 3, syscalls |
| P8 FS | 1200 | 30-45 | ATA, FAT32, VFS |
| P9 GPU | 3000+ | 100+ | PCI, GPU arch, DMA |
| P10 Perf | 500+ | 20-30 | Lock-free, PMC, SIMD |

**Total core kernel**: ~5,950 lines / ~150-200 hours
**With GPU**: ~9,000+ lines / ~250-350 hours

---

> **Final word**: This roadmap is ambitious. Real hobby OS projects (ToaruOS, skiftOS, uniOS) took years. Pick the next phase that excites you most and **ship it**. Each working [OK] in the boot log is a victory.


---

## 18. Phase 11 — AI-Native Operating System

> "The OS should be self-documenting, self-verifying, and AI-accessible by design."

**Difficulty: Medium (AI integration) + Hard (OS architecture) | Est. LOC: 1500+ | Time: 40-60 hours**

### Design Principle - SECURITY WARNING
**MCP at the kernel level is DANGEROUS.** Having an AI agent directly execute tools inside the kernel is a catastrophic security risk — prompt injection, parser bugs, or compromised AI clients would give full kernel control.

**Correct architecture**: MCP runs at the **OS (userspace) level** as a trusted system daemon (`mcpd`). It communicates with the kernel through the **same safe syscall interface** that any application uses. The kernel does not know about MCP — it just exports standard syscalls.

### 18.1 MCP Daemon (Userspace System Service)

The MCP daemon (`mcpd`) runs as a **userspace system process** (ring 3, not ring 0). It translates MCP tool calls into **safe syscalls** and applies authentication, rate limiting, and audit logging. It exposes system capabilities to AI agents via the Model Context Protocol (MCP) — the open standard for LLM-tool integration (adopted by Anthropic, OpenAI, VS Code, JetBrains, and others, with the 2026 roadmap focusing on transport scalability and agent communication).

**Available MCP Tools** (listening at `/var/run/mcpd.sock`):

```json
{
  "tools": [
      "description": "List all running processes/tasks" },
    { "name": "nova.get_cpu_usage",
      "description": "Get per-CPU utilization" },
    { "name": "nova.get_logs",
      "description": "Retrieve kernel log messages",
      "inputSchema": { "level": "string", "since_ms": "uint64" } },
    { "name": "nova.profile_start/stop",
      "description": "Start/stop kernel profiling" },
    { "name": "nova.compile_check",
      "description": "Verify kernel compiles with changes" }
  ],
  "resources": [
    { "uri": "nova://cpu/usage",
      "description": "Per-CPU usage telemetry" },
    { "uri": "nova://memory/pages",
      "description": "Page allocation breakdown" },
    { "uri": "nova://scheduler/tasks",
      "description": "Active task list and states" },
    { "uri": "nova://drivers/gpu/status",
      "description": "GPU driver status + metrics" }
  ]
}
```

**Implementation**:
- [ ] MCP daemon: UNIX domain socket (AF_UNIX) in userspace
- [ ] MCP protocol frame parser (JSON-RPC 2.0)
- [ ] MCP tool registration: `mcpd --register-tool name handler`
- [ ] Resource endpoint: `mcp_serve_resource(uri, callback)`
- [ ] Security: JWT auth, rate-limiting, audit log
- [ ] Integration: VS Code / Cursor / Claude Desktop connect to mcpd

```c
// Example: registering an MCP tool in kernel
static mcp_tool_t tool_get_cpu_usage = {
    .name = "nova.get_cpu_usage",
    .description = "Get per-CPU utilization percentage",
    .handler = mcp_handler_get_cpu_usage,
    .input_schema = "{}",  // no args
    .auth_required = true,
};
mcp_register_tool(&tool_get_cpu_usage);

// Handler writes response into MCP buffer
static void mcp_handler_get_cpu_usage(mcp_req_t *req) {
    mcp_write_json(req, "{cpu: [23, 45, 12, 67]}");
}
```

---

### 18.2 Security Verification Agent

An automated AI agent that reviews every commit for security vulnerabilities:

**Checks performed on each PR**:
- [ ] **Buffer overflow detection**: scan `kmalloc`/`memcpy` patterns
- [ ] **Use-after-free**: track `kfree(ptr)` → use of `ptr`
- [ ] **Race conditions**: detect shared state without locks
- [ ] **Integer overflow**: arithmetic on unchecked `size_t`
- [ ] **TOCTOU**: double-fetch from user-space memory
- [ ] **Format string**: `printk(msg)` where msg = user-controlled
- [ ] **Uninitialized variables**: stack variables used before init
- [ ] **Null pointer dereference**: missing NULL checks on kmalloc

**CI Integration** (`.github/workflows/security.yml`):
```yaml
name: Security Audit
on: [pull_request]
jobs:
  audit:
    runs-on: ubuntu-latest
    steps:
      - run: make  # build first
      - run: nova-security-check ./kernel/  # AI analysis
      - uses: actions/github-script@v7
        with:
          script: |
            // Post results as PR comment
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              body: results
            })
```

---

### 18.3 Auto-Documentation Generator

Every function in the kernel is annotated with structured comments and automatically documented:

```c
/**
 * @brief Allocate a zeroed page from physical memory manager
 *
 * @return void* Virtual address of page, NULL on OOM
 *
 * @performance O(1), < 10 ns
 * @lock   Must NOT hold any lock (can trigger page table update)
 * @smp    Per-CPU free stack, no cross-core access
 * @test   pmm_alloc_10000_pages()
 * @see    pmm_free(), pmm_alloc_contiguous()
 */
void *pmm_alloc(void) { ... }
```

**Pipeline**:
1. `make docs` → parse all `@brief`, `@param`, `@return` tags
2. AI generates: explanation in plain English, call graph, performance notes
3. Output: `docs/` directory with HTML/Markdown
4. Deploy to `ivan-cavero.github.io/nova/` via GitHub Pages

---

### 18.4 Dev Agent (AI Pair Programmer for Nova)

An AI agent specifically trained on Nova source code that helps with:

| Task | How AI helps |
|------|--------------|
| **Code generation** | Suggest implementation for Phase N from roadmap |
| **Debug analysis** | Read kernel log → identify bug → suggest fix |
| **Performance** | Profile hotspots → suggest optimization |
| **Porting** | Suggest ARM64 equivalents of x86-64 code |
| **Refactoring** | Identify dead code, simplify |
| **Test writing** | Generate test cases from function signatures |

The Dev Agent can be used via:
- **MCP client** (VS Code, Cursor, Claude Desktop) → connect to Nova MCP server
- **CLI tool**: `nova-ai "implement round-robin scheduler"`
- **GitHub Action**: auto-suggest fixes on failed builds

---

### 18.5 Automatic Issue Creation from Roadmap

```bash
# scripts/roadmap-to-issues.sh
#
# Parses ROADMAP.md, finds unchecked checkboxes, creates GitHub issues
#
# ./roadmap-to-issues.sh --phase "Phase 1"
# Creates:
#   Issue "P1: Install IDT with 256 entry vectors"
#     label: phase-1, difficulty: medium
#   Issue "P1: Handle #PF page fault with CR2 decode"
#     label: phase-1, depends: IDT-installed
#   Issue "P1: PIT timer at 1000 Hz"
#     label: phase-1, depends: PIC-remap
#   Issue "P1: PS/2 keyboard driver"
#     label: phase-1, depends: PIC-remap
```

**Features**:
- [ ] Parse ROADMAP.md checkboxes
- [ ] Create GitHub issues with labels, descriptions, priorities
- [ ] Track dependencies between phases
- [ ] Update issue status when checkbox is marked
- [ ] Generate milestone with estimated hours
- [ ] Post progress summary weekly via AI agent

---

### 18.6 MCP Client in Userspace

Nova ships with a native MCP client that connects to any MCP server:

```bash
# Inside Nova shell:
mcp-connect /var/mcp/nova.sock
# Interactive mode: ask questions about kernel state
mcp> how much memory is free?
  → [nova.get_memory_stats] 54 MB free / 64 MB total
mcp> is the GPU driver loaded?
  → [nova.gpu_status] NVIDIA RTX 4060, VRAM 8GB, engine idle
```

This turns Nova into a **self-aware operating system** — it can introspect itself, explain its own state, and even debug itself with AI assistance.

---

### 18.7 Checklist

- [ ] Kernel MCP server: transport + protocol + auth
- [ ] MCP resource: `nova://cpu/usage` (streaming)
- [ ] MCP resource: `nova://memory/pages`
- [ ] MCP resource: `nova://scheduler/tasks`
- [ ] MCP resource: `nova://drivers/gpu/status`
- [ ] Security agent: static analysis pipeline
- [ ] Security agent: runtime verification (canary)
- [ ] Auto-doc generator: `make docs`
- [ ] Dev agent: `nova-ai` CLI tool
- [ ] Roadmap → issues script
- [ ] MCP client in userspace: `mcp-connect`
- [ ] GitHub Pages documentation site

### Timeline

| Sub-phase | Est. hours | Depends on |
|-----------|-----------|------------|
| MCP transport (UNIX socket) | 10 | Syscalls (P7) |
| MCP tools for kernel state | 15 | MCP transport |
| Security agent pipeline | 8 | CI setup |
| Auto-documentation | 6 | Security agent |
| Dev agent CLI | 10 | MCP tools |
| Roadmap → issues | 4 | Dev agent |
| Userspace MCP client | 8 | MCP tools |
| **Total** | **~61 hours** | |

