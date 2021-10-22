#include <aarch64/intrinsic.h>
#include <common/spinlock.h>

void init_spinlock(SpinLock *lock) {
    lock->locked = 0;
    lock->cpu = 0;
}

bool try_acquire_spinlock(SpinLock *lock) {
    if (!lock->locked && !__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE)) {
        lock->cpu = thiscpu();
        return true;
    } else {
        return false;
    }
}

void acquire_spinlock(SpinLock *lock) {
    if (holding_spinlock(lock)) {
        PANIC("acquire: lock already held\n");
    }
    while (!try_acquire_spinlock(lock)) {}
}

void release_spinlock(SpinLock *lock) {
    if (!holding_spinlock(lock)) {
        printf("%p %p\n", thiscpu(), lock->cpu);
        PANIC("release: lock not held\n");
    }
	lock->cpu = 0;
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

void wait_spinlock(SpinLock *lock) {
    acquire_spinlock(lock);
    release_spinlock(lock);
}

bool holding_spinlock(SpinLock *lock) {
    return lock->locked && lock->cpu == thiscpu();
}