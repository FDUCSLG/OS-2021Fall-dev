#pragma once

#include <common/defines.h>

typedef struct {
    volatile bool locked;
} SpinLock;

void init_spinlock(SpinLock *lock);

bool try_acquire_spinlock(SpinLock *lock);
void acquire_spinlock(SpinLock *lock);
void release_spinlock(SpinLock *lock);
void wait_spinlock(SpinLock *lock);
