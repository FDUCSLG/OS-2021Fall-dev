#pragma once

#include <core/proc.h>

typedef struct {
    SpinLock lock;
    bool locked;
} SleepLock;

void init_sleeplock(SleepLock *lock, const char *name);
void acquire_sleeplock(SleepLock *lock);
void release_sleeplock(SleepLock *lock);
