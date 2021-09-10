#pragma once

#include "defines.h"
#include "proc.h"
#include "container.h"
#include "sched.h"
#include "spinlock.h"
#include <aarch64/intrinsic.h>
struct cpu {
    struct scheduler *scheduler;
    struct sched_obj *sched_obj;
    SpinLock lock;
};

extern struct cpu cpu[NCPU];

static inline struct cpu *thiscpu()
{
    return &cpu[cpuid()];
}

static inline void init_cpu()
{
    thiscpu()->scheduler = root_scheduler;
    thiscpu()->sched_obj = root_scheduler;
    init_spinlock(&thiscpu()->lock);
}