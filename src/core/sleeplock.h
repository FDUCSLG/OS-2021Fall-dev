#pragma once

#include <common/defines.h>

// a workaround for fs test.
#ifndef __cplusplus

#include <core/proc.h>

ALWAYS_INLINE void _sleep(void *chan, SpinLock *lock) {
    sleep(chan, lock);
}

ALWAYS_INLINE void _wakeup(void *chan) {
    wakeup(chan);
}

#else

// these will be provided by mock.
void _sleep(void *chan, SpinLock *lock);
void _wakeup(void *chan);

#endif

typedef struct {
    SpinLock lock;
    bool locked;
} SleepLock;

void init_sleeplock(SleepLock *lock, const char *name);
void acquire_sleeplock(SleepLock *lock);
void release_sleeplock(SleepLock *lock);
