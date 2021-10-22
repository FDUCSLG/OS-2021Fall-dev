#pragma once

#include <common/defines.h>
#include <core/cpu.h>

typedef struct {
    volatile bool locked;
    struct cpu *cpu;
    char *name;
} SpinLock;

void init_spinlock(SpinLock *lock, char *);

bool try_acquire_spinlock(SpinLock *lock);
void acquire_spinlock(SpinLock *lock);
void release_spinlock(SpinLock *lock);
void wait_spinlock(SpinLock *lock);
bool holding_spinlock(SpinLock *lock);
