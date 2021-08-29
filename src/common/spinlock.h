#pragma once

#ifndef _COMMON_SPINLOCK_H_
#define _COMMON_SPINLOCK_H_

typedef struct {
    volatile int locked;
} SpinLock;

void init_spinlock(SpinLock *lock);
void acquire_spinlock(SpinLock *lock);
void release_spinlock(SpinLock *lock);

#endif
