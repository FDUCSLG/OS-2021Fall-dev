#include <common/spinlock.h>

void init_spinlock(SpinLock *lock) {
    lock->locked = 0;
}

void acquire_spinlock(SpinLock *lock) {
    while (lock->locked || __atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {}
}

void release_spinlock(SpinLock *lock) {
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}
