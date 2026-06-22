#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

/* Compiler attribute shortcuts */
#define PACKED      __attribute__((packed))
#define ALIGNED(x)  __attribute__((aligned(x)))
#define USED        __attribute__((used))
#define UNUSED      __attribute__((unused))
#define NORETURN    __attribute__((noreturn))
#define SECTION(s)  __attribute__((section(s)))
#define INLINE      __attribute__((always_inline)) inline
#define HOT         __attribute__((hot))
#define COLD        __attribute__((cold))
#define PURE        __attribute__((pure))
#define CONST_FN    __attribute__((const))

/* Branch prediction hints */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Compiler memory barrier */
#define BARRIER()   __asm__ volatile ("" ::: "memory")

/* Spin-wait pause (more efficient than NOP for spinlocks) */
#define cpu_pause() __asm__ volatile ("pause" ::: "memory")

/* CPU instructions */
static inline void halt(void) {
    __asm__ volatile ("hlt");
}

static inline void cli(void) {
    __asm__ volatile ("cli");
}

static inline void sti(void) {
    __asm__ volatile ("sti");
}

#endif /* ATTRIBUTES_H */