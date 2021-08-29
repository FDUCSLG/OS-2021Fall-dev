#pragma once

#ifndef _COMMON_SPINLOCK_H_
#define _COMMON_SPINLOCK_H_

#include <common/types.h>

typedef struct {
    volatile bool locked;
} SpinLock;

void init_spinlock(SpinLock *lock);

bool try_acquire_spinlock(SpinLock *lock);
void acquire_spinlock(SpinLock *lock);
void release_spinlock(SpinLock *lock);
void wait_spinlock(SpinLock *lock);

#endif
