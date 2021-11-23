#include "map.hpp"

#include <condition_variable>

namespace {

struct Mutex {
    bool locked;
    std::mutex mutex;

    void lock() {
        mutex.lock();
        locked = true;
    }

    void unlock() {
        locked = false;
        mutex.unlock();
    }
};

Map<void *, Mutex> mtx_map;
Map<void *, std::condition_variable_any> cv_map;

}  // namespace

extern "C" {
void init_spinlock(struct SpinLock *lock, const char *name [[maybe_unused]]) {
    mtx_map.try_add(lock);
}

void acquire_spinlock(struct SpinLock *lock) {
    mtx_map[lock].lock();
}

void release_spinlock(struct SpinLock *lock) {
    mtx_map[lock].unlock();
}

bool holding_spinlock(struct SpinLock *lock) {
    return mtx_map[lock].locked;
}

void init_sleeplock(struct SleepLock *lock, const char *name [[maybe_unused]]) {
    mtx_map.try_add(lock);
}

void acquire_sleeplock(struct SleepLock *lock) {
    mtx_map[lock].lock();
}

void release_sleeplock(struct SleepLock *lock) {
    mtx_map[lock].unlock();
}

void _fs_test_sleep(void *chan, struct SpinLock *lock) {
    cv_map.atomic_apply(chan, [lock](auto &cv) { cv.wait(mtx_map[lock].mutex); });
}

void _fs_test_wakeup(void *chan) {
    cv_map.atomic_apply(chan, [](auto &cv) { cv.notify_all(); });
}
}
