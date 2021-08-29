#pragma once

#ifndef _AARCH64_ARCH_H_
#define _AARCH64_ARCH_H_

#include <common/types.h>

static inline size_t cpuid() {
    uint64_t id;
    asm volatile("mrs %[x], mpidr_el1" : [x] "=r"(id));
    return id & 0xff;
}

// instruct compiler not to reorder instructions around the fence.
static inline void compiler_fence() {
    asm volatile("" ::: "memory");
}

static inline uint64_t get_timer_freq() {
    uint64_t result;
    asm volatile("mrs %[freq], cntfrq_el0" : [freq] "=r"(result));
    return result;
}

static inline uint64_t get_timestamp() {
    uint64_t result;
    compiler_fence();
    asm volatile("mrs %[cnt], cntpct_el0" : [cnt] "=r"(result));
    compiler_fence();
    return result;
}

// instruction synchronization barrier.
static inline void arch_isb() {
    asm volatile("isb" ::: "memory");
}

// data synchronization barrier.
static inline void arch_dsb_sy() {
    asm volatile("dsb sy" ::: "memory");
}

// for `device_get/put_*`, there's no need to protect them with architectual
// barriers, since they are intended to access device memory regions. These
// regions are already marked as nGnRnE in `kernel_pt`.

static inline void device_put_uint32(uint64_t addr, uint32_t value) {
    compiler_fence();
    *(volatile uint32_t *)addr = value;
    compiler_fence();
}

static inline uint32_t device_get_uint32(uint64_t addr) {
    compiler_fence();
    uint32_t value = *(volatile uint32_t *)addr;
    compiler_fence();
    return value;
}

void delay_us(uint64_t n);

#endif
