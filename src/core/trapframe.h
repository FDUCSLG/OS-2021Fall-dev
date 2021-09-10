#pragma once

#include <common/defines.h>

typedef struct {
    u64 spsr, elr, sp, tpidr;
    u64 x[31];
    u64 _padding;

    // TODO: this is a dirty hack, since musl's `memset` requires only SIMD register `q0`.
    u64 q0[2];
} Trapframe;
