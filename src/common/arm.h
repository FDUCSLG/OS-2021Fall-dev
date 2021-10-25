#ifndef INC_ARM_H
#define INC_ARM_H

#include <common/defines.h>

static inline void
delay(i32 count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n":
                 "=r"(count): [count]"0"(count) : "cc");
}

/* Wait N microsec. */
static inline void
delayus(u32 n)
{
    u64 f, t, r;
    /* Get the current counter frequency */
    asm volatile ("mrs %[freq], cntfrq_el0" : [freq]"=r"(f));
    /* Read the current counter. */
    asm volatile ("mrs %[cnt], cntpct_el0" : [cnt]"=r"(t));
    /* Calculate expire value for counter */
    t += f / 1000000 * n;
    do {
        asm volatile ("mrs %[cnt], cntpct_el0" : [cnt]"=r"(r));
    } while (r < t);
}

static inline u64
timestamp()
{
    u64 t;
    asm volatile ("mrs %[cnt], cntpct_el0" : [cnt]"=r"(t));
    return t;
}

static inline void
put32(u64 p, u32 x)
{
    *(volatile u32 *)p = x;
}

static inline u32
get32(u64 p)
{
    return *(volatile u32 *)p;
}

/* Unmask DAIF to start interrupt. */
static inline void
sti()
{
    asm volatile("msr daif, %[x]" : : [x]"r"(0));
}

/* Mask DAIF to close interrupt. */
static inline void
cli()
{
    asm volatile("msr daif, %[x]" : : [x]"r"(0xF << 6));
}

/* Brute-force data and instruction synchronization barrier. */
static inline void
disb()
{
    asm volatile("dsb sy; isb");
}

/* Data cache clean and invalidate by virtual address to point of coherency. */
static inline void
dccivac(void *p, int n)
{
    while (n--)
        asm volatile("dc civac, %[x]" : : [x]"r"(p + n));
}

/* Read Exception Syndrome Register (EL1). */
static inline u64
resr()
{
    u64 r;
    asm volatile("mrs %[x], esr_el1" : [x]"=r"(r));
    return r;
}

/* Read Exception Link Register (EL1). */
static inline u64
relr()
{
    u64 r;
    asm volatile("mrs %[x], elr_el1" : [x]"=r"(r));
    return r;
}

/* Load Exception Syndrome Register (EL1). */
static inline void
lesr(u64 r)
{
    asm volatile("msr esr_el1, %[x]" : : [x]"r"(r));
}

/* Load vector base (virtual) address register (EL1). */
static inline void
lvbar(void *p)
{
    disb();
    asm volatile("msr vbar_el1, %[x]" : : [x]"r"(p));
    disb();
}

/* Load Translation Table Base Register 0 (EL1). */
static inline void
lttbr0(u64 p)
{
    asm volatile("msr ttbr0_el1, %[x]" : : [x]"r"(p));
    disb();
    asm volatile("tlbi vmalle1");
    disb();
}

/* Load Translation Table Base Register 1 (EL1). */
static inline void
lttbr1(u64 p)
{
    asm volatile("msr ttbr1_el1, %[x]" : : [x]"r"(p));
    disb();
    asm volatile("tlbi vmalle1");
    disb();
}

static inline int
cpuid()
{
    i64 id;
    asm volatile("mrs %[x], mpidr_el1" : [x]"=r"(id));
    return id & 0xFF;
}

#endif
